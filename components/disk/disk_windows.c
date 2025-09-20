/**
 * @file disk_windows.c
 * @brief Implementación de discos para Windows usando WMI y APIs del sistema
 */

#ifdef _WIN32

#include "disk_interface.h"
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL disk_initialized = FALSE;
static DiskInfo disk_list[MAX_DISKS];
static int disk_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene información de discos usando WMI
 */
static int get_disk_info_wmi(void)
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

    // Ejecutar consulta WQL para discos físicos
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT DeviceID, Model, Size, MediaType FROM Win32_DiskDrive",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);
    if (FAILED(hr))
    {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    disk_count = 0;

    // Procesar resultados
    while (SUCCEEDED(enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned)) &&
           returned != 0 && disk_count < MAX_DISKS)
    {

        DiskInfo *disk = &disk_list[disk_count];
        memset(disk, 0, sizeof(DiskInfo));

        // Obtener DeviceID
        VariantInit(&variant);
        hr = class_object->lpVtbl->Get(class_object, L"DeviceID", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
        {
            WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                disk->device, sizeof(disk->device), NULL, NULL);
        }
        VariantClear(&variant);

        // Obtener modelo
        hr = class_object->lpVtbl->Get(class_object, L"Model", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
        {
            WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                disk->model, sizeof(disk->model), NULL, NULL);
        }
        VariantClear(&variant);

        // Obtener tamaño
        hr = class_object->lpVtbl->Get(class_object, L"Size", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
        {
            // Convertir string a número
            char size_str[64];
            WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                size_str, sizeof(size_str), NULL, NULL);
            unsigned long long size_bytes = _strtoui64(size_str, NULL, 10);
            disk->size_gb = (float)(size_bytes / (1024.0 * 1024.0 * 1024.0));
        }
        VariantClear(&variant);

        // Obtener tipo de medio
        hr = class_object->lpVtbl->Get(class_object, L"MediaType", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
        {
            char media_type[128];
            WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                media_type, sizeof(media_type), NULL, NULL);

            // Determinar tipo de disco
            if (strstr(media_type, "Fixed") || strstr(media_type, "hard"))
            {
                // Asumir HDD por defecto, SSD requiere detección adicional
                strncpy(disk->type, "HDD", sizeof(disk->type) - 1);
            }
            else
            {
                strncpy(disk->type, "Unknown", sizeof(disk->type) - 1);
            }
        }
        else
        {
            strncpy(disk->type, "Unknown", sizeof(disk->type) - 1);
        }
        VariantClear(&variant);

        // Información adicional (valores por defecto)
        disk->temperature = -1.0f; // Requiere SMART
        disk->usage_percent = 0.0f;
        disk->health_percent = 100.0f; // Asumir saludable
        disk->power_on_hours = 0;
        disk->read_speed_mbps = -1.0f;
        disk->write_speed_mbps = -1.0f;

        class_object->lpVtbl->Release(class_object);
        class_object = NULL;
        disk_count++;
    }

    // Limpiar recursos
    if (class_object)
        class_object->lpVtbl->Release(class_object);
    enumerator->lpVtbl->Release(enumerator);
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();

    return disk_count > 0 ? 0 : -1;
}

/**
 * @brief Obtiene información de uso de discos lógicos
 */
static int get_disk_usage_info(void)
{
    char drives[256];
    DWORD drives_len = GetLogicalDriveStrings(sizeof(drives), drives);

    if (drives_len == 0)
    {
        return -1;
    }

    char *drive = drives;
    int logical_disk_index = 0;

    while (*drive && logical_disk_index < disk_count)
    {
        ULARGE_INTEGER free_bytes, total_bytes;

        if (GetDiskFreeSpaceEx(drive, &free_bytes, &total_bytes, NULL))
        {
            // Buscar el disco físico correspondiente (simplificado)
            if (logical_disk_index < disk_count)
            {
                DiskInfo *disk = &disk_list[logical_disk_index];

                float total_gb = (float)(total_bytes.QuadPart / (1024.0 * 1024.0 * 1024.0));
                float free_gb = (float)(free_bytes.QuadPart / (1024.0 * 1024.0 * 1024.0));
                float used_gb = total_gb - free_gb;

                if (total_gb > 0)
                {
                    disk->usage_percent = (used_gb / total_gb) * 100.0f;
                }

                // Actualizar información del punto de montaje
                strncpy(disk->mount_point, drive, sizeof(disk->mount_point) - 1);
            }
        }

        // Avanzar al siguiente drive
        drive += strlen(drive) + 1;
        logical_disk_index++;
    }

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int disk_init(void)
{
    if (disk_initialized)
    {
        return 0;
    }

    // Limpiar lista de discos
    memset(disk_list, 0, sizeof(disk_list));
    disk_count = 0;

    // Obtener información de discos usando WMI
    if (get_disk_info_wmi() != 0)
    {
        return -1;
    }

    // Obtener información de uso
    get_disk_usage_info();

    disk_initialized = TRUE;
    return 0;
}

void disk_cleanup(void)
{
    disk_initialized = FALSE;
    disk_count = 0;
}

int disk_get_count(void)
{
    return disk_initialized ? disk_count : -1;
}

int disk_get_info(int disk_id, DiskInfo *info)
{
    if (!info || !disk_initialized || disk_id < 0 || disk_id >= disk_count)
    {
        return -1;
    }

    memcpy(info, &disk_list[disk_id], sizeof(DiskInfo));
    return 0;
}

int disk_get_metrics(DiskMetrics *metrics)
{
    if (!metrics || !disk_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(DiskMetrics));

    metrics->num_disks = disk_count;

    // Copiar información de todos los discos
    for (int i = 0; i < disk_count && i < MAX_DISKS; i++)
    {
        memcpy(&metrics->disks[i], &disk_list[i], sizeof(DiskInfo));
    }

    return 0;
}

float disk_get_usage_percent(int disk_id)
{
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count)
    {
        return -1.0f;
    }

    return disk_list[disk_id].usage_percent;
}

float disk_get_temperature(int disk_id)
{
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count)
    {
        return -1.0f;
    }

    // La temperatura SMART en Windows requiere APIs específicas o herramientas externas
    return disk_list[disk_id].temperature;
}

int disk_get_name(int disk_id, char *buffer, size_t size)
{
    if (!buffer || size == 0 || !disk_initialized || disk_id < 0 || disk_id >= disk_count)
    {
        return -1;
    }

    strncpy(buffer, disk_list[disk_id].model, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float disk_get_health_percent(int disk_id)
{
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count)
    {
        return -1.0f;
    }

    // La salud SMART en Windows requiere APIs específicas
    return disk_list[disk_id].health_percent;
}

#endif /* _WIN32 */