#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    SC_HANDLE hSCM = OpenSCManager(
        NULL, 
        NULL, 
        SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCM) {
        fprintf(stderr, "OpenSCManager failed: %lu\n", GetLastError());
        return 1;
    }

    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;

    EnumServicesStatusExA(
        hSCM, 
        SC_ENUM_PROCESS_INFO, 
        SERVICE_WIN32, 
        SERVICE_ACTIVE,
        NULL, 
        0, 
        &bytesNeeded, 
        &serviceCount, 
        &resumeHandle, 
        NULL);

    BYTE* pBuf = (BYTE*)malloc(bytesNeeded);
    if (!pBuf) {
        fprintf(stderr, "Memory allocation failed.\n");
        CloseServiceHandle(hSCM);
        return 1;
    }

    if (!EnumServicesStatusExA(
        hSCM, 
        SC_ENUM_PROCESS_INFO, 
        SERVICE_WIN32, 
        SERVICE_ACTIVE,
        pBuf, 
        bytesNeeded, 
        &bytesNeeded, 
        &serviceCount, 
        &resumeHandle, 
        NULL))
    {
        fprintf(stderr, "EnumServicesStatusEx failed: %lu\n", GetLastError());
        free(pBuf);
        CloseServiceHandle(hSCM);
        return 1;
    }

    ENUM_SERVICE_STATUS_PROCESS* pServices = (ENUM_SERVICE_STATUS_PROCESS*)pBuf;

    printf("  RUNNING WINDOWS SERVICES (%lu found)\n", serviceCount);
    printf("%-50s %-10s %s\n", "Service Name", "PID", "Display Name");
    printf("%-50s %-10s %s\n",
        "--------------------------------------------------",
        "----------",
        "--------------------------------------------");

    for (DWORD i = 0; i < serviceCount; i++) {
        printf("%-50s %-10lu %s\n",
            pServices[i].lpServiceName,
            pServices[i].ServiceStatusProcess.dwProcessId,
            pServices[i].lpDisplayName);
    }

    free(pBuf);
    CloseServiceHandle(hSCM);
    return 0;
}