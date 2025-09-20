/**
 * @file advanced_windows.c
 * @brief Implementación de funciones avanzadas para Windows usando Registry y WMI
 */

#ifdef _WIN32

#include "advanced_interface.h"
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL advanced_initialized = FALSE;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene información del sistema usando WMI
 */
static int get_system_info_wmi(SystemInfo *sys_info)
{
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *class_object = NULL;
    ULONG returned = 0;
    VARIANT variant;

    // Inicializar COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

    // Crear locator WMI
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID *)&locator);
    if (FAILED(hr))
    {
        CoUninitialize();
        return -1;
    }

    // Conectar al namespace WMI
    hr = locator->lpVtbl->ConnectServer(
        locator,
        L"ROOT\\CIMV2",
        NULL, NULL, 0, NULL, 0, 0,
        &services);
    if (FAILED(hr))
    {
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Configurar seguridad
    hr = CoSetProxyBlanket(
        (IUnknown *)services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);
    if (FAILED(hr))
    {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Obtener información del sistema operativo
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Caption, Version, BuildNumber, OSArchitecture FROM Win32_OperatingSystem",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    if (SUCCEEDED(hr))
    {
        hr = enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned);
        if (returned > 0)
        {
            // Nombre del OS
            VariantInit(&variant);
            hr = class_object->lpVtbl->Get(class_object, L"Caption", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->os_name, sizeof(sys_info->os_name), NULL, NULL);
            }
            VariantClear(&variant);

            // Versión del OS
            hr = class_object->lpVtbl->Get(class_object, L"Version", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->os_version, sizeof(sys_info->os_version), NULL, NULL);
            }
            VariantClear(&variant);

            // Build number
            hr = class_object->lpVtbl->Get(class_object, L"BuildNumber", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->os_build, sizeof(sys_info->os_build), NULL, NULL);
            }
            VariantClear(&variant);

            // Arquitectura
            hr = class_object->lpVtbl->Get(class_object, L"OSArchitecture", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->architecture, sizeof(sys_info->architecture), NULL, NULL);
            }
            VariantClear(&variant);

            class_object->lpVtbl->Release(class_object);
        }
        enumerator->lpVtbl->Release(enumerator);
    }

    // Obtener información del motherboard
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Manufacturer, Product FROM Win32_BaseBoard",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    if (SUCCEEDED(hr))
    {
        hr = enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned);
        if (returned > 0)
        {
            // Fabricante del motherboard
            VariantInit(&variant);
            hr = class_object->lpVtbl->Get(class_object, L"Manufacturer", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->motherboard_vendor, sizeof(sys_info->motherboard_vendor), NULL, NULL);
            }
            VariantClear(&variant);

            // Modelo del motherboard
            hr = class_object->lpVtbl->Get(class_object, L"Product", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->motherboard_model, sizeof(sys_info->motherboard_model), NULL, NULL);
            }
            VariantClear(&variant);

            class_object->lpVtbl->Release(class_object);
        }
        enumerator->lpVtbl->Release(enumerator);
    }

    // Obtener información del BIOS
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Manufacturer, SMBIOSBIOSVersion, ReleaseDate FROM Win32_BIOS",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    if (SUCCEEDED(hr))
    {
        hr = enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned);
        if (returned > 0)
        {
            // Fabricante del BIOS
            VariantInit(&variant);
            hr = class_object->lpVtbl->Get(class_object, L"Manufacturer", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->bios_vendor, sizeof(sys_info->bios_vendor), NULL, NULL);
            }
            VariantClear(&variant);

            // Versión del BIOS
            hr = class_object->lpVtbl->Get(class_object, L"SMBIOSBIOSVersion", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->bios_version, sizeof(sys_info->bios_version), NULL, NULL);
            }
            VariantClear(&variant);

            // Fecha del BIOS
            hr = class_object->lpVtbl->Get(class_object, L"ReleaseDate", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sys_info->bios_date, sizeof(sys_info->bios_date), NULL, NULL);
            }
            VariantClear(&variant);

            class_object->lpVtbl->Release(class_object);
        }
        enumerator->lpVtbl->Release(enumerator);
    }

    // Limpiar recursos
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();

    return 0;
}

/**
 * @brief Obtiene información de procesos del sistema
 */
static int get_process_info(ProcessInfo *proc_info)
{
    SYSTEM_INFO sys_info;
    FILETIME idle_time, kernel_time, user_time;
    ULARGE_INTEGER idle, kernel, user;

    // Obtener información del sistema
    GetSystemInfo(&sys_info);
    proc_info->num_processors = sys_info.dwNumberOfProcessors;

    // Obtener tiempos del sistema
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time))
    {
        idle.LowPart = idle_time.dwLowDateTime;
        idle.HighPart = idle_time.dwHighDateTime;
        kernel.LowPart = kernel_time.dwLowDateTime;
        kernel.HighPart = kernel_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;

        // Calcular load average aproximado (simplificado)
        ULONGLONG total_time = kernel.QuadPart + user.QuadPart;
        ULONGLONG active_time = total_time - idle.QuadPart;

        if (total_time > 0)
        {
            proc_info->load_average_1min = (float)(active_time * 100.0 / total_time);
            proc_info->load_average_5min = proc_info->load_average_1min;  // Aproximación
            proc_info->load_average_15min = proc_info->load_average_1min; // Aproximación
        }
    }

    // Obtener número de procesos (aproximado usando Performance Data)
    PERFORMANCE_INFORMATION perf_info;
    perf_info.cb = sizeof(PERFORMANCE_INFORMATION);

    if (GetPerformanceInfo(&perf_info, sizeof(PERFORMANCE_INFORMATION)))
    {
        proc_info->num_processes = perf_info.ProcessCount;
        proc_info->num_threads = perf_info.ThreadCount;
        proc_info->num_handles = perf_info.HandleCount;
    }

    return 0;
}

/**
 * @brief Obtiene uptime del sistema
 */
static int get_uptime_info(UptimeInfo *uptime_info)
{
    // Obtener uptime en milisegundos
    ULONGLONG uptime_ms = GetTickCount64();

    uptime_info->uptime_seconds = uptime_ms / 1000;
    uptime_info->uptime_minutes = uptime_info->uptime_seconds / 60;
    uptime_info->uptime_hours = uptime_info->uptime_minutes / 60;
    uptime_info->uptime_days = uptime_info->uptime_hours / 24;

    // Obtener tiempo de boot (aproximado)
    FILETIME boot_time;
    SYSTEMTIME boot_system_time;
    ULARGE_INTEGER boot_time_ull;

    GetSystemTimeAsFileTime(&boot_time);
    boot_time_ull.LowPart = boot_time.dwLowDateTime;
    boot_time_ull.HighPart = boot_time.dwHighDateTime;

    // Restar uptime para obtener tiempo de boot
    boot_time_ull.QuadPart -= uptime_ms * 10000; // Convertir ms a 100ns intervals

    boot_time.dwLowDateTime = boot_time_ull.LowPart;
    boot_time.dwHighDateTime = boot_time_ull.HighPart;

    if (FileTimeToSystemTime(&boot_time, &boot_system_time))
    {
        snprintf(uptime_info->boot_time, sizeof(uptime_info->boot_time),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 boot_system_time.wYear, boot_system_time.wMonth, boot_system_time.wDay,
                 boot_system_time.wHour, boot_system_time.wMinute, boot_system_time.wSecond);
    }

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int advanced_init(void)
{
    advanced_initialized = TRUE;
    return 0;
}

void advanced_cleanup(void)
{
    advanced_initialized = FALSE;
}

int advanced_get_system_info(SystemInfo *info)
{
    if (!info || !advanced_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(info, 0, sizeof(SystemInfo));

    // Obtener información usando WMI
    if (get_system_info_wmi(info) != 0)
    {
        // Si WMI falla, usar valores por defecto
        strncpy(info->os_name, "Windows", sizeof(info->os_name) - 1);
        strncpy(info->os_version, "Unknown", sizeof(info->os_version) - 1);
        strncpy(info->architecture, "x64", sizeof(info->architecture) - 1);
    }

    // Obtener hostname
    DWORD hostname_size = sizeof(info->hostname);
    GetComputerName(info->hostname, &hostname_size);

    // Obtener username
    DWORD username_size = sizeof(info->username);
    GetUserName(info->username, &username_size);

    return 0;
}

int advanced_get_process_info(ProcessInfo *info)
{
    if (!info || !advanced_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(info, 0, sizeof(ProcessInfo));

    return get_process_info(info);
}

int advanced_get_uptime_info(UptimeInfo *info)
{
    if (!info || !advanced_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(info, 0, sizeof(UptimeInfo));

    return get_uptime_info(info);
}

int advanced_get_metrics(AdvancedMetrics *metrics)
{
    if (!metrics || !advanced_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(AdvancedMetrics));

    // Obtener todas las métricas
    advanced_get_system_info(&metrics->system);
    advanced_get_process_info(&metrics->processes);
    advanced_get_uptime_info(&metrics->uptime);

    return 0;
}

float advanced_get_load_average_1min(void)
{
    if (!advanced_initialized)
    {
        return -1.0f;
    }

    ProcessInfo proc_info;
    if (get_process_info(&proc_info) == 0)
    {
        return proc_info.load_average_1min;
    }

    return -1.0f;
}

int advanced_get_process_count(void)
{
    if (!advanced_initialized)
    {
        return -1;
    }

    ProcessInfo proc_info;
    if (get_process_info(&proc_info) == 0)
    {
        return proc_info.num_processes;
    }

    return -1;
}

long advanced_get_uptime_seconds(void)
{
    if (!advanced_initialized)
    {
        return -1;
    }

    return (long)(GetTickCount64() / 1000);
}

int advanced_get_hostname(char *buffer, size_t size)
{
    if (!buffer || size == 0 || !advanced_initialized)
    {
        return -1;
    }

    DWORD buffer_size = (DWORD)size;
    if (GetComputerName(buffer, &buffer_size))
    {
        return 0;
    }

    return -1;
}

#endif /* _WIN32 */