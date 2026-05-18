#include <windows.h>
#include <winsvc.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

typedef struct _DLL_COUNT {
    LPWSTR lpPath;
    DWORD dwCount;
} DLL_COUNT, * PDLL_COUNT;

LPWSTR NormalizePath(LPCWSTR lpPath)
{
    SIZE_T cchLength = wcslen(lpPath);
    LPWSTR lpNormalized = malloc((cchLength + 1) * sizeof(WCHAR));

    if (!lpNormalized) {
        return NULL;
    }

    for (SIZE_T i = 0; i < cchLength; i++) {
        lpNormalized[i] = towlower(lpPath[i]);
    }

    lpNormalized[cchLength] = L'\0';
    return lpNormalized;
}

BOOL FindPath(LPWSTR* lppList, SIZE_T cCount, LPCWSTR lpPath)
{
    for (SIZE_T i = 0; i < cCount; i++) {
        if (wcscmp(lppList[i], lpPath) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL AddSeenPath(LPWSTR** lpppList, SIZE_T* pcCount, SIZE_T* pcCapacity, LPCWSTR lpPath)
{
    if (*pcCount == *pcCapacity) {
        SIZE_T cNewCapacity = (*pcCapacity == 0) ? 16 : (*pcCapacity * 2);
        LPWSTR* lppResized = realloc(*lpppList, cNewCapacity * sizeof(LPWSTR));

        if (!lppResized) {
            return FALSE;
        }

        *lpppList = lppResized;
        *pcCapacity = cNewCapacity;
    }

    LPWSTR lpCopiedPath = _wcsdup(lpPath);

    if (!lpCopiedPath) {
        return FALSE;
    }

    (*lpppList)[*pcCount] = lpCopiedPath;
    (*pcCount)++;

    return TRUE;
}

BOOL AddDllCount(PDLL_COUNT* lppList, SIZE_T* pcCount, SIZE_T* pcCapacity, LPCWSTR lpPath)
{
    for (SIZE_T i = 0; i < *pcCount; i++) {
        if (wcscmp((*lppList)[i].lpPath, lpPath) == 0) {
            (*lppList)[i].dwCount++;
            return TRUE;
        }
    }

    if (*pcCount == *pcCapacity) {
        SIZE_T cNewCapacity = (*pcCapacity == 0) ? 32 : (*pcCapacity * 2);
        SIZE_T cOldCapacity = *pcCapacity;
        PDLL_COUNT lpResized = NULL;

        if (*pcCapacity == 0) {
            lpResized = calloc(cNewCapacity, sizeof(DLL_COUNT));
        } else {
            lpResized = realloc(*lppList, cNewCapacity * sizeof(DLL_COUNT));
        }

        if (!lpResized) {
            return FALSE;
        }

        if (cOldCapacity < cNewCapacity) {
            memset(lpResized + cOldCapacity, 0, (cNewCapacity - cOldCapacity) * sizeof(DLL_COUNT));
        }

        *lppList = lpResized;
        *pcCapacity = cNewCapacity;
    }

    (*lppList)[*pcCount].lpPath = NULL;
    (*lppList)[*pcCount].dwCount = 0;

    (*lppList)[*pcCount].lpPath = _wcsdup(lpPath);

    if (!(*lppList)[*pcCount].lpPath) {
        return FALSE;
    }

    (*lppList)[*pcCount].dwCount = 1;
    (*pcCount)++;

    return TRUE;
}

INT wmain(VOID)
{
    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (!hSCM) {
        fwprintf(stderr, L"OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }

    DWORD cbBytesNeeded = 0;
    DWORD dwServiceCount = 0;
    DWORD dwResumeHandle = 0;

    if (!EnumServicesStatusExW(
        hSCM,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        NULL,
        0,
        &cbBytesNeeded,
        &dwServiceCount,
        &dwResumeHandle,
        NULL
    )) {
        DWORD dwError = GetLastError();

        if (dwError != ERROR_MORE_DATA) {
            fwprintf(stderr, L"EnumServicesStatusEx sizing failed: %lu\n", dwError);
            CloseServiceHandle(hSCM);
            return 1;
        }
    } else {
        fwprintf(stderr, L"EnumServicesStatusEx sizing failed: expected ERROR_MORE_DATA.\n");
        CloseServiceHandle(hSCM);
        return 1;
    }

    LPBYTE lpBuffer = malloc(cbBytesNeeded);

    if (!lpBuffer) {
        fwprintf(stderr, L"Memory allocation failed.\n");
        CloseServiceHandle(hSCM);
        return 1;
    }

    if (!EnumServicesStatusExW(
        hSCM,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        lpBuffer,
        cbBytesNeeded,
        &cbBytesNeeded,
        &dwServiceCount,
        &dwResumeHandle,
        NULL
    )) {
        fwprintf(stderr, L"EnumServicesStatusEx failed: %lu\n", GetLastError());
        free(lpBuffer);
        CloseServiceHandle(hSCM);
        return 1;
    }

    ENUM_SERVICE_STATUS_PROCESSW* lpServices = (ENUM_SERVICE_STATUS_PROCESSW*)lpBuffer;

    PDLL_COUNT lpDllCounts = NULL;
    SIZE_T cDllCount = 0;
    SIZE_T cDllCapacity = 0;

    wprintf(L"RUNNING WINDOWS SERVICES (%lu found)\n", dwServiceCount);
    wprintf(L"%-50ls %-10ls %ls\n", L"Service Name", L"PID", L"Display Name");

    for (DWORD i = 0; i < dwServiceCount; i++) {
        DWORD dwPid = lpServices[i].ServiceStatusProcess.dwProcessId;

        wprintf(L"%-50ls %-10lu %ls\n",
            lpServices[i].lpServiceName,
            dwPid,
            lpServices[i].lpDisplayName
        );

        if (dwPid == 0) {
            continue;
        }

        HANDLE hSnapshot = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            dwPid
        );

        if (hSnapshot == INVALID_HANDLE_VALUE) {
            wprintf(L"Could not scan PID %lu, error: %lu\n", dwPid, GetLastError());
            continue;
        }

        MODULEENTRY32W ModuleEntry;
        ModuleEntry.dwSize = sizeof(ModuleEntry);

        LPWSTR* lppSeenPaths = NULL;
        SIZE_T cSeenCount = 0;
        SIZE_T cSeenCapacity = 0;

        if (Module32FirstW(hSnapshot, &ModuleEntry)) {
            do {
                LPWSTR lpNormalizedPath = NormalizePath(ModuleEntry.szExePath);

                if (!lpNormalizedPath) {
                    continue;
                }

                if (!FindPath(lppSeenPaths, cSeenCount, lpNormalizedPath)) {
                    if (AddSeenPath(&lppSeenPaths, &cSeenCount, &cSeenCapacity, lpNormalizedPath)) {
                        AddDllCount(&lpDllCounts, &cDllCount, &cDllCapacity, lpNormalizedPath);
                    }
                }

                free(lpNormalizedPath);

            } while (Module32NextW(hSnapshot, &ModuleEntry));
        }

        for (SIZE_T j = 0; j < cSeenCount; j++) {
            free(lppSeenPaths[j]);
        }

        free(lppSeenPaths);
        CloseHandle(hSnapshot);
    }

    wprintf(L"\nDLLs loaded by more than one active service:\n");

    if (lpDllCounts) {
        for (SIZE_T i = 0; i < cDllCount; i++) {
            if (lpDllCounts[i].lpPath && lpDllCounts[i].dwCount > 1) {
                wprintf(L"%ls (count: %lu)\n",
                    lpDllCounts[i].lpPath,
                    lpDllCounts[i].dwCount
                );
            }

            free(lpDllCounts[i].lpPath);
        }

        free(lpDllCounts);
    }
    free(lpBuffer);
    CloseServiceHandle(hSCM);

    return 0;
}