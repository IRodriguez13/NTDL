#include "../metrics.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <wbemidl.h>
#include <comdef.h>
#include <winternl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <regstr.h>
#include <ntddk.h>

// Declaraciones de funciones auxiliares
void get_windows_version(SystemStatus *status);
void get_cpu_detailed_info(SystemStatus *status);
void get_memory_detailed_info(SystemStatus *status);
void get_disk_info_windows(SystemStatus *status);
void get_network_info_windows(SystemStatus *status);
void get_gpu_info_windows(SystemStatus *status);
void get_sensors_info_windows(SystemStatus *status);
void get_motherboard_info_windows(SystemStatus *status);
void get_system_load_windows(SystemStatus *status);
void get_process_info_windows(SystemStatus *status);
void get_uptime_info_windows(SystemStatus *status);

void collect_metrics(SystemStatus *status)
{
    // Información básica del sistema
    strcpy(status->os_name, "Windows");

    // Obtener versión de Windows
    get_windows_version(status);

    // Obtener hostname
    DWORD hostname_size = sizeof(status->hostname);
    GetComputerNameA(status->hostname, &hostname_size);

    // Información básica de CPU
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    status->cpu_cores = sysinfo.dwNumberOfProcessors;
    status->cpu_threads = status->cpu_cores; // Simplificado por ahora

    // Obtener modelo de CPU desde el registro
    HKEY hKey;
    DWORD size = sizeof(status->cpu_model);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL,
                        (LPBYTE)status->cpu_model, &size);
        RegCloseKey(hKey);
    }
    else
    {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    // Información detallada de CPU
    get_cpu_detailed_info(status);

    // Información básica de memoria
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    status->ram_total_kb = mem.ullTotalPhys / 1024;
    status->ram_free_kb = mem.ullAvailPhys / 1024;
    status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
    status->ram_usage_percent = (float)status->ram_used_kb / status->ram_total_kb * 100.0f;

    // Información detallada de memoria
    get_memory_detailed_info(status);

    // Información de discos
    get_disk_info_windows(status);

    // Información de red
    get_network_info_windows(status);

    // Información de GPU
    get_gpu_info_windows(status);

    // Información de sensores
    get_sensors_info_windows(status);

    // Información de motherboard
    get_motherboard_info_windows(status);

    // Información de carga del sistema
    get_system_load_windows(status);

    // Información de procesos
    get_process_info_windows(status);

    // Timestamp y tiempo de actividad
    status->timestamp = time(NULL);
    get_uptime_info_windows(status);
}

void get_windows_version(SystemStatus *status)
{
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((OSVERSIONINFO *)&osvi))
    {
        snprintf(status->os_version, sizeof(status->os_version),
                 "%lu.%lu.%lu", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
        snprintf(status->kernel_version, sizeof(status->kernel_version),
                 "NT %lu.%lu", osvi.dwMajorVersion, osvi.dwMinorVersion);
    }
    else
    {
        strcpy(status->os_version, "Unknown");
        strcpy(status->kernel_version, "Unknown");
    }
}

void get_cpu_detailed_info(SystemStatus *status)
{
    // Usar Performance Counters para obtener uso de CPU
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;

    if (PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS)
    {
        if (PdhAddCounter(hQuery, "\\Processor(_Total)\\% Processor Time", 0, &hCounter) == ERROR_SUCCESS)
        {
            PdhCollectQueryData(hQuery);
            Sleep(100); // Esperar para obtener datos válidos
            PdhCollectQueryData(hQuery);

            PDH_FMT_COUNTERVALUE counterVal;
            if (PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
            {
                status->cpu_usage_percent = (float)(100.0 - counterVal.doubleValue);
            }
        }
        PdhCloseQuery(hQuery);
    }

    // Información por núcleo (simplificada)
    status->num_cores = status->cpu_cores;
    for (int i = 0; i < status->cpu_cores && i < MAX_CORES; i++)
    {
        status->cores[i].core_id = i;
        status->cores[i].temperature = -1.0f;                       // Requiere drivers específicos
        status->cores[i].frequency = -1.0f;                         // Requiere WMI o drivers específicos
        status->cores[i].voltage = -1.0f;                           // Requiere drivers específicos
        status->cores[i].usage_percent = status->cpu_usage_percent; // Simplificado
        status->cores[i].max_frequency = -1.0f;
        status->cores[i].min_frequency = -1.0f;
        status->cores[i].load = 0;
    }

    status->cpu_temperature = -1.0f;
    status->cpu_frequency = -1.0f;
}

void get_memory_detailed_info(SystemStatus *status)
{
    // Inicializar información detallada de RAM
    strcpy(status->detailed_ram.type, "Unknown");
    status->detailed_ram.frequency_mhz = 0;
    status->detailed_ram.num_sticks = 0;
    status->detailed_ram.l1_cache_kb = 0;
    status->detailed_ram.l2_cache_kb = 0;
    status->detailed_ram.l3_cache_kb = 0;

    // Información básica de memoria desde Performance Counters
    status->detailed_ram.total_kb = status->ram_total_kb;
    status->detailed_ram.used_kb = status->ram_used_kb;
    status->detailed_ram.free_kb = status->ram_free_kb;
    status->detailed_ram.cached_kb = 0; // Requiere APIs específicas
    status->detailed_ram.swap_total_kb = 0;
    status->detailed_ram.swap_used_kb = 0;
}

void get_disk_info_windows(SystemStatus *status)
{
    status->num_disks = 0;

    // Obtener drives lógicos
    DWORD drives = GetLogicalDrives();
    char drive_letter = 'A';

    for (int i = 0; i < 26 && status->num_disks < MAX_DISKS; i++)
    {
        if (drives & (1 << i))
        {
            char drive_path[4] = {drive_letter + i, ':', '\\', '\0'};
            UINT drive_type = GetDriveTypeA(drive_path);

            if (drive_type == DRIVE_FIXED)
            { // Solo discos fijos
                DiskInfo *disk = &status->disks[status->num_disks];

                snprintf(disk->name, sizeof(disk->name), "Drive_%c", drive_letter + i);
                snprintf(disk->device_path, sizeof(disk->device_path), "%c:", drive_letter + i);
                strcpy(disk->protocol, "Unknown");
                strcpy(disk->model, "Windows Disk");
                strcpy(disk->serial, "Unknown");

                // Obtener espacio del disco
                ULARGE_INTEGER free_bytes, total_bytes;
                if (GetDiskFreeSpaceExA(drive_path, &free_bytes, &total_bytes, NULL))
                {
                    disk->total_kb = total_bytes.QuadPart / 1024;
                    disk->free_kb = free_bytes.QuadPart / 1024;
                    disk->used_kb = disk->total_kb - disk->free_kb;
                    disk->usage_percent = (float)disk->used_kb / disk->total_kb * 100.0f;
                }
                else
                {
                    disk->total_kb = 0;
                    disk->free_kb = 0;
                    disk->used_kb = 0;
                    disk->usage_percent = 0.0f;
                }

                disk->temperature = -1.0f;
                disk->sectors_read = 0;
                disk->sectors_written = 0;
                disk->read_errors = 0;
                disk->write_errors = 0;
                disk->power_on_hours = 0;
                disk->power_cycles = 0;
                disk->smart_status = 0;
                disk->health = 100;

                strcpy(disk->mount_point, drive_path);
                strcpy(disk->filesystem, "NTFS"); // Simplificado

                status->num_disks++;
            }
        }
    }
}

void get_network_info_windows(SystemStatus *status)
{
    status->num_network_interfaces = 0;
    // Implementación básica - requiere WinSock2 y API específicas
    // Por simplicidad, inicializamos con valores por defecto
}

void get_gpu_info_windows(SystemStatus *status)
{
    // Información básica de GPU
    strcpy(status->gpu_model, "Unknown GPU");
    strcpy(status->gpu_driver, "Unknown");
    strcpy(status->gpu_vendor, "Unknown");
    status->gpu_memory_total_mb = 0;
    status->gpu_memory_used_mb = 0;
    status->gpu_memory_free_mb = 0;
    status->gpu_usage_percent = 0.0f;
    status->gpu_temperature = -1.0f;
    status->gpu_memory_usage_percent = 0.0f;
    status->gpu_fan_speed = 0;
    status->gpu_power_usage_w = 0;
    status->gpu_clock_mhz = 0;
    status->gpu_memory_clock_mhz = 0;
}

void get_sensors_info_windows(SystemStatus *status)
{
    status->num_sensors = 0;
    // Los sensores en Windows requieren drivers específicos como SpeedFan, HWiNFO, etc.
}

void get_motherboard_info_windows(SystemStatus *status)
{
    // Validar puntero nulo
    if (!status) {
        return;
    }
    
    // Inicializar valores por defecto
    strcpy(status->motherboard.manufacturer, "Unknown");
    strcpy(status->motherboard.model, "Unknown");
    strcpy(status->motherboard.version, "Unknown");
    strcpy(status->motherboard.serial, "Unknown");
    strcpy(status->motherboard.bios_version, "Unknown");
    strcpy(status->motherboard.chipset, "Unknown");
    
    // Usar APIs centralizadas de Windows: Registry + WMI
    HKEY hKey;
    DWORD size;
    char value[256];

    // Información del sistema desde múltiples fuentes del registro
    const char* registry_paths[] = {
        "HARDWARE\\DESCRIPTION\\System\\BIOS",
        "SYSTEM\\CurrentControlSet\\Control\\SystemInformation",
        "HARDWARE\\DESCRIPTION\\System"
    };
    
    // Intentar obtener fabricante
    for (int i = 0; i < 3; i++) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, registry_paths[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            size = sizeof(value);
            if (RegQueryValueExA(hKey, "SystemManufacturer", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
                strncpy(status->motherboard.manufacturer, value, 63);
                status->motherboard.manufacturer[63] = '\0';
                RegCloseKey(hKey);
                break;
            }
            RegCloseKey(hKey);
        }
    }
    
    // Intentar obtener modelo
    for (int i = 0; i < 3; i++) {
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, registry_paths[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            size = sizeof(value);
            if (RegQueryValueExA(hKey, "SystemProductName", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
                strncpy(status->motherboard.model, value, 127);
                status->motherboard.model[127] = '\0';
                RegCloseKey(hKey);
                break;
            }
            RegCloseKey(hKey);
        }
    }
    
    // Intentar obtener versión BIOS
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(value);
        if (RegQueryValueExA(hKey, "BIOSVersion", NULL, NULL, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            strncpy(status->motherboard.bios_version, value, 63);
            status->motherboard.bios_version[63] = '\0';
        }
        RegCloseKey(hKey);
    }
}

void get_system_load_windows(SystemStatus *status)
{
    // Windows no tiene load average como Linux, usar alternativas
    status->load_average_1min = 0.0f;
    status->load_average_5min = 0.0f;
    status->load_average_15min = 0.0f;
    status->load_1min_percent = 0.0f;
    status->load_5min_percent = 0.0f;
    status->load_15min_percent = 0.0f;
}

void get_process_info_windows(SystemStatus *status)
{
    // Usar ToolHelp32 para obtener información de procesos
    status->total_processes = 0;
    status->running_processes = 0;
    status->sleeping_processes = 0;
    status->stopped_processes = 0;
    status->zombie_processes = 0;

    // Implementación básica usando Performance Counters
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;

    if (PdhOpenQuery(NULL, 0, &hQuery) == ERROR_SUCCESS)
    {
        if (PdhAddCounter(hQuery, "\\System\\Processes", 0, &hCounter) == ERROR_SUCCESS)
        {
            PdhCollectQueryData(hQuery);

            PDH_FMT_COUNTERVALUE counterVal;
            if (PdhGetFormattedCounterValue(hCounter, PDH_FMT_LONG, NULL, &counterVal) == ERROR_SUCCESS)
            {
                status->total_processes = counterVal.longValue;
                status->running_processes = counterVal.longValue; // Simplificado
            }
        }
        PdhCloseQuery(hQuery);
    }
}

void get_uptime_info_windows(SystemStatus *status)
{
    // Obtener tiempo de actividad del sistema
    ULONGLONG uptime_ms = GetTickCount64();
    status->uptime_seconds = uptime_ms / 1000;
    status->idle_time_seconds = 0; // No fácilmente disponible en Windows
}

#endif
