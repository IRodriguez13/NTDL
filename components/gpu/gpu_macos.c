/**
 * @file gpu_macos.c
 * @brief Implementación de GPU para macOS usando Metal y IOKit
 */

#ifdef __APPLE__

#include "gpu_interface.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL gpu_initialized = FALSE;
static GpuInfo gpu_list[MAX_GPUS];
static int gpu_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Convierte CFString a C string
 */
static void cfstring_to_cstring(CFStringRef cfstr, char *buffer, size_t buffer_size)
{
    if (cfstr && buffer && buffer_size > 0)
    {
        CFStringGetCString(cfstr, buffer, buffer_size, kCFStringEncodingUTF8);
    }
    else if (buffer && buffer_size > 0)
    {
        buffer[0] = '\0';
    }
}

/**
 * @brief Obtiene valor numérico de una propiedad CFNumber
 */
static int64_t get_cf_number_value(CFNumberRef number)
{
    int64_t value = 0;
    if (number && CFGetTypeID(number) == CFNumberGetTypeID())
    {
        CFNumberGetValue(number, kCFNumberSInt64Type, &value);
    }
    return value;
}

/**
 * @brief Enumera GPUs usando IOKit
 */
static int enumerate_gpus_iokit(void)
{
    io_iterator_t iterator;
    io_service_t service;

    // Buscar servicios de display
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                    IOServiceMatching("IOPCIDevice"),
                                                    &iterator);

    if (kr != KERN_SUCCESS)
    {
        return -1;
    }

    gpu_count = 0;

    while ((service = IOIteratorNext(iterator)) != 0 && gpu_count < MAX_GPUS)
    {
        CFMutableDictionaryRef properties = NULL;

        kr = IORegistryEntryCreateCFProperties(service, &properties,
                                               kCFAllocatorDefault, kNilOptions);

        if (kr == KERN_SUCCESS && properties)
        {
            // Verificar si es un dispositivo gráfico
            CFStringRef class_code = CFDictionaryGetValue(properties, CFSTR("class-code"));
            CFStringRef device_type = CFDictionaryGetValue(properties, CFSTR("device_type"));

            BOOL is_gpu = FALSE;

            // Verificar por class-code (0x030000 = VGA compatible controller)
            if (class_code && CFGetTypeID(class_code) == CFDataGetTypeID())
            {
                CFDataRef data = (CFDataRef)class_code;
                if (CFDataGetLength(data) >= 4)
                {
                    const UInt8 *bytes = CFDataGetBytePtr(data);
                    UInt32 code = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
                    if ((code & 0xFF0000) == 0x030000)
                    { // Display controller
                        is_gpu = TRUE;
                    }
                }
            }

            // Verificar por device_type
            if (!is_gpu && device_type && CFGetTypeID(device_type) == CFStringGetTypeID())
            {
                char type_str[256];
                cfstring_to_cstring(device_type, type_str, sizeof(type_str));
                if (strstr(type_str, "display") || strstr(type_str, "graphics"))
                {
                    is_gpu = TRUE;
                }
            }

            if (is_gpu)
            {
                GpuInfo *gpu = &gpu_list[gpu_count];
                memset(gpu, 0, sizeof(GpuInfo));

                // Nombre del dispositivo
                CFStringRef name = CFDictionaryGetValue(properties, CFSTR("model"));
                if (!name)
                {
                    name = CFDictionaryGetValue(properties, CFSTR("name"));
                }
                if (name)
                {
                    cfstring_to_cstring(name, gpu->name, sizeof(gpu->name));
                }
                else
                {
                    strncpy(gpu->name, "Unknown GPU", sizeof(gpu->name) - 1);
                }

                // Vendor ID para determinar fabricante
                CFNumberRef vendor_id = CFDictionaryGetValue(properties, CFSTR("vendor-id"));
                if (vendor_id)
                {
                    int64_t vid = get_cf_number_value(vendor_id);
                    switch (vid)
                    {
                    case 0x10DE: // NVIDIA
                        strncpy(gpu->vendor, "NVIDIA", sizeof(gpu->vendor) - 1);
                        break;
                    case 0x1002: // AMD
                        strncpy(gpu->vendor, "AMD", sizeof(gpu->vendor) - 1);
                        break;
                    case 0x8086: // Intel
                        strncpy(gpu->vendor, "Intel", sizeof(gpu->vendor) - 1);
                        break;
                    case 0x106B: // Apple
                        strncpy(gpu->vendor, "Apple", sizeof(gpu->vendor) - 1);
                        break;
                    default:
                        strncpy(gpu->vendor, "Unknown", sizeof(gpu->vendor) - 1);
                        break;
                    }
                }

                // Memoria VRAM (si está disponible)
                CFNumberRef vram_size = CFDictionaryGetValue(properties, CFSTR("VRAM,totalsize"));
                if (!vram_size)
                {
                    vram_size = CFDictionaryGetValue(properties, CFSTR("ATY,memsize"));
                }
                if (vram_size)
                {
                    int64_t vram_bytes = get_cf_number_value(vram_size);
                    gpu->memory_total_mb = (float)(vram_bytes / (1024 * 1024));
                    gpu->memory_free_mb = gpu->memory_total_mb; // Aproximación
                }

                // Información adicional (valores por defecto)
                gpu->memory_used_mb = 0.0f;
                gpu->usage_percent = 0.0f; // Difícil de obtener en macOS sin herramientas específicas
                gpu->temperature = -1.0f;
                gpu->fan_speed_percent = -1.0f;
                gpu->power_usage_watts = -1.0f;
                gpu->clock_core_mhz = -1.0f;
                gpu->clock_memory_mhz = -1.0f;

                // Versión del driver (aproximada)
                strncpy(gpu->driver_version, "macOS Built-in", sizeof(gpu->driver_version) - 1);

                gpu_count++;
            }

            CFRelease(properties);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iterator);

    // Si no encontramos GPUs por IOKit, intentar detectar GPUs integradas de Apple
    if (gpu_count == 0)
    {
        // Detectar Apple Silicon GPUs
        size_t size = 0;
        if (sysctlbyname("hw.optional.arm64", NULL, &size, NULL, 0) == 0)
        {
            // Es Apple Silicon, agregar GPU integrada
            GpuInfo *gpu = &gpu_list[gpu_count];
            memset(gpu, 0, sizeof(GpuInfo));

            strncpy(gpu->name, "Apple Integrated GPU", sizeof(gpu->name) - 1);
            strncpy(gpu->vendor, "Apple", sizeof(gpu->vendor) - 1);
            strncpy(gpu->driver_version, "macOS Built-in", sizeof(gpu->driver_version) - 1);

            // Valores estimados para Apple Silicon
            gpu->memory_total_mb = 0.0f; // Memoria unificada
            gpu->memory_used_mb = 0.0f;
            gpu->memory_free_mb = 0.0f;
            gpu->usage_percent = 0.0f;
            gpu->temperature = -1.0f;
            gpu->fan_speed_percent = -1.0f;
            gpu->power_usage_watts = -1.0f;
            gpu->clock_core_mhz = -1.0f;
            gpu->clock_memory_mhz = -1.0f;

            gpu_count++;
        }
    }

    return gpu_count > 0 ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int gpu_init(void)
{
    if (gpu_initialized)
    {
        return 0;
    }

    // Limpiar lista de GPUs
    memset(gpu_list, 0, sizeof(gpu_list));
    gpu_count = 0;

    // Enumerar GPUs usando IOKit
    if (enumerate_gpus_iokit() != 0)
    {
        return -1;
    }

    gpu_initialized = TRUE;
    return 0;
}

void gpu_cleanup(void)
{
    gpu_initialized = FALSE;
    gpu_count = 0;
}

int gpu_get_count(void)
{
    return gpu_initialized ? gpu_count : -1;
}

int gpu_get_info(int gpu_id, GpuInfo *info)
{
    if (!info || !gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count)
    {
        return -1;
    }

    memcpy(info, &gpu_list[gpu_id], sizeof(GpuInfo));
    return 0;
}

int gpu_get_metrics(GpuMetrics *metrics)
{
    if (!metrics || !gpu_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(GpuMetrics));

    metrics->num_gpus = gpu_count;

    // Copiar información de todas las GPUs
    for (int i = 0; i < gpu_count && i < MAX_GPUS; i++)
    {
        memcpy(&metrics->gpus[i], &gpu_list[i], sizeof(GpuInfo));
    }

    return 0;
}

float gpu_get_usage_percent(int gpu_id)
{
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count)
    {
        return -1.0f;
    }

    // El uso de GPU en macOS requiere herramientas específicas o Metal Performance Shaders
    return gpu_list[gpu_id].usage_percent;
}

float gpu_get_temperature(int gpu_id)
{
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count)
    {
        return -1.0f;
    }

    // La temperatura de GPU en macOS requiere acceso a sensores específicos
    return gpu_list[gpu_id].temperature;
}

float gpu_get_memory_usage_percent(int gpu_id)
{
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count)
    {
        return -1.0f;
    }

    GpuInfo *gpu = &gpu_list[gpu_id];
    if (gpu->memory_total_mb <= 0)
    {
        return -1.0f;
    }

    return (gpu->memory_used_mb / gpu->memory_total_mb) * 100.0f;
}

int gpu_get_name(int gpu_id, char *buffer, size_t size)
{
    if (!buffer || size == 0 || !gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count)
    {
        return -1;
    }

    strncpy(buffer, gpu_list[gpu_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

#endif /* __APPLE__ */