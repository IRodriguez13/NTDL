#include "../metrics.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tchar.h>

void collect_metrics(SystemStatus *status) {
    strcpy(status->os_name, "Windows");

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    status->cpu_cores = sysinfo.dwNumberOfProcessors;

    HKEY hKey;
    DWORD size = sizeof(status->cpu_model);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL,
            (LPBYTE)status->cpu_model, &size);
        RegCloseKey(hKey);
    } else {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    status->ram_total_kb = mem.ullTotalPhys / 1024;
    status->ram_free_kb = mem.ullAvailPhys / 1024;

    strcpy(status->disk_model, "Generic Disk");
    status->disk_health = -1;
    status->cpu_usage_percent = 0.0f; // Simplificado
}
#endif
