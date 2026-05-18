#include <stdio.h>
#include <windows.h>

VOID PrintSubKeys(HKEY key) {
    CHAR szClass[MAX_PATH] = "";
    DWORD chClass = MAX_PATH;

    DWORD cSubKeys = 0;
    DWORD cValues = 0;
    DWORD cbSecurityDescriptor = 0;
    FILETIME ftLastWriteTime;

    CHAR szSubKeyName[MAX_PATH] = "";
    DWORD cchSubKeyNameSize = MAX_PATH;

    LSTATUS lsKeyStatus = RegQueryInfoKeyA(
        key,
        szClass,
        &chClass,
        NULL,
        &cSubKeys,
        NULL,
        NULL,
        &cValues,
        NULL,
        NULL,
        &cbSecurityDescriptor,
        &ftLastWriteTime
    );

    if (lsKeyStatus == ERROR_SUCCESS && cSubKeys > 0) {
        LSTATUS lsSubKeyStatus;
        for (DWORD i = 0; i < cSubKeys; i++) {
            cchSubKeyNameSize = MAX_PATH;

            lsSubKeyStatus = RegEnumKeyExA(
                key,
                i,
                szSubKeyName,
                &cchSubKeyNameSize,
                NULL,
                NULL,
                NULL,
                NULL
            );

            if (lsSubKeyStatus == ERROR_SUCCESS) {
                printf("%s\n", szSubKeyName);
            }
        }
    }
}

int main() {
    HKEY hkQueriedKey = 0;
    LPCSTR szSubKey = "SOFTWARE";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        szSubKey,
        0,
        KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
        &hkQueriedKey)
        == ERROR_SUCCESS
        ) {
        PrintSubKeys(hkQueriedKey);
        RegCloseKey(hkQueriedKey);
    }
    return 0;
}