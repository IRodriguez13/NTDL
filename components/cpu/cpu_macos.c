/**
 * @file cpu_macos.c
 * @brief Implementación de CPU para macOS usando IOKit y sysctl
 */

#ifdef __APPLE__

#include "cpu_interface.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL cpu_initialized = FALSE;
static char cpu_model_cache[256] = {0};
static char cpu_vendor_cache[64] = {0};
static int physical_cores_cache = 0;
static int logical_cores_cache = 0;
static processor_info_array_t cpu_info_prev = NULL;
static mach_msg_type_number_t cpu_info_prev_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene información básica del CPU usando sysctl
 */
static int get_cpu_basic_info(void) {
    size_t size;
    
    // Obtener modelo del CPU
    size = sizeof(cpu_model_cache);
    if (sysctlbyname("machdep.cpu.brand_string", cpu_model_cache, &size, NULL, 0) != 0) {
        strncpy(cpu_model_cache, "Unknown CPU", sizeof(cpu_model_cache) - 1);
    }
    
    // Obtener vendor del CPU
    size = sizeof(cpu_vendor_cache);
    if (sysctlbyname("machdep.cpu.vendor", cpu_vendor_cache, &size, NULL, 0) != 0) {
        strncpy(cpu_vendor_cache, "Unknown", sizeof(cpu_vendor_cache) - 1);
    }
    
    // Obtener número de núcleos físicos
    size = sizeof(physical_cores_cache);
    if (sysctlbyname("hw.physicalcpu", &physical_cores_cache, &size, NULL, 0) != 0) {
        physical_cores_cache = 1;
    }
    
    // Obtener número de núcleos lógicos
    size = sizeof(logical_cores_cache);
    if (sysctlbyname("hw.logicalcpu", &logical_cores_cache, &size, NULL, 0) != 0) {
        logical_cores_cache = physical_cores_cache;
    }
    
    return 0;
}

/**
 * @brief Obtiene la frecuencia del CPU
 */
static float get_cpu_frequency(void) {
    uint64_t frequency = 0;
    size_t size = sizeof(frequency);
    
    // Intentar obtener frecuencia actual
    if (sysctlbyname("hw.cpufrequency", &frequency, &size, NULL, 0) == 0) {
        return (float)(frequency / 1000000.0); // Convertir a MHz
    }
    
    // Si no está disponible, intentar frecuencia máxima
    if (sysctlbyname("hw.cpufrequency_max", &frequency, &size, NULL, 0) == 0) {
        return (float)(frequency / 1000000.0);
    }
    
    // Fallback: usar información del modelo para estimar
    if (strstr(cpu_model_cache, "M1")) {
        return 3200.0f; // M1 típico
    } else if (strstr(cpu_model_cache, "M2")) {
        return 3500.0f; // M2 típico
    }
    
    return -1.0f;
}

/**
 * @brief Obtiene el uso del CPU usando host_processor_info
 */
static float get_cpu_usage(void) {
    processor_info_array_t cpu_info;
    mach_msg_type_number_t cpu_info_count;
    natural_t cpu_count;
    
    kern_return_t kr = host_processor_info(mach_host_self(),
                                          PROCESSOR_CPU_LOAD_INFO,
                                          &cpu_count,
                                          &cpu_info,
                                          &cpu_info_count);
    
    if (kr != KERN_SUCCESS) {
        return -1.0f;
    }
    
    float usage = 0.0f;
    
    if (cpu_info_prev != NULL) {
        processor_cpu_load_info_t cpu_load = (processor_cpu_load_info_t)cpu_info;
        processor_cpu_load_info_t cpu_load_prev = (processor_cpu_load_info_t)cpu_info_prev;
        
        unsigned int total_ticks = 0;
        unsigned int idle_ticks = 0;
        
        for (natural_t i = 0; i < cpu_count; i++) {
            unsigned int user_delta = cpu_load[i].cpu_ticks[CPU_STATE_USER] - 
                                     cpu_load_prev[i].cpu_ticks[CPU_STATE_USER];
            unsigned int system_delta = cpu_load[i].cpu_ticks[CPU_STATE_SYSTEM] - 
                                       cpu_load_prev[i].cpu_ticks[CPU_STATE_SYSTEM];
            unsigned int nice_delta = cpu_load[i].cpu_ticks[CPU_STATE_NICE] - 
                                     cpu_load_prev[i].cpu_ticks[CPU_STATE_NICE];
            unsigned int idle_delta = cpu_load[i].cpu_ticks[CPU_STATE_IDLE] - 
                                     cpu_load_prev[i].cpu_ticks[CPU_STATE_IDLE];
            
            total_ticks += user_delta + system_delta + nice_delta + idle_delta;
            idle_ticks += idle_delta;
        }
        
        if (total_ticks > 0) {
            usage = ((float)(total_ticks - idle_ticks) / total_ticks) * 100.0f;
        }
    }
    
    // Liberar información anterior y guardar la actual
    if (cpu_info_prev != NULL) {
        vm_deallocate(mach_task_self(), (vm_address_t)cpu_info_prev, 
                     cpu_info_prev_count * sizeof(natural_t));
    }
    
    cpu_info_prev = cpu_info;
    cpu_info_prev_count = cpu_info_count;
    
    return usage;
}

/**
 * @brief Obtiene la temperatura del CPU usando IOKit
 */
static float get_cpu_temperature(void) {
    io_iterator_t iterator;
    io_object_t service;
    float temperature = -1.0f;
    
    // Buscar sensores de temperatura
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                   IOServiceMatching("IOHWSensor"),
                                                   &iterator);
    
    if (kr != KERN_SUCCESS) {
        return -1.0f;
    }
    
    while ((service = IOIteratorNext(iterator)) != 0) {
        CFStringRef name = IORegistryEntryCreateCFProperty(service,
                                                          CFSTR("sensor-key"),
                                                          kCFAllocatorDefault,
                                                          0);
        
        if (name) {
            char sensor_name[256];
            CFStringGetCString(name, sensor_name, sizeof(sensor_name), kCFStringEncodingUTF8);
            
            // Buscar sensores de CPU
            if (strstr(sensor_name, "CPU") || strstr(sensor_name, "cpu")) {
                CFNumberRef temp_ref = IORegistryEntryCreateCFProperty(service,
                                                                      CFSTR("current-value"),
                                                                      kCFAllocatorDefault,
                                                                      0);
                
                if (temp_ref) {
                    SInt32 temp_value;
                    if (CFNumberGetValue(temp_ref, kCFNumberSInt32Type, &temp_value)) {
                        // La temperatura suele venir en formato fixed-point
                        temperature = temp_value / 65536.0f;
                        CFRelease(temp_ref);
                        CFRelease(name);
                        IOObjectRelease(service);
                        break;
                    }
                    CFRelease(temp_ref);
                }
            }
            CFRelease(name);
        }
        IOObjectRelease(service);
    }
    
    IOObjectRelease(iterator);
    return temperature;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int cpu_init(void) {
    if (cpu_initialized) {
        return 0;
    }
    
    // Obtener información básica del CPU
    if (get_cpu_basic_info() != 0) {
        return -1;
    }
    
    // Inicializar medición de uso (primera llamada para establecer baseline)
    get_cpu_usage();
    
    cpu_initialized = TRUE;
    return 0;
}

void cpu_cleanup(void) {
    if (cpu_info_prev != NULL) {
        vm_deallocate(mach_task_self(), (vm_address_t)cpu_info_prev, 
                     cpu_info_prev_count * sizeof(natural_t));
        cpu_info_prev = NULL;
        cpu_info_prev_count = 0;
    }
    cpu_initialized = FALSE;
}

int cpu_get_metrics(CpuMetrics *metrics) {
    if (!metrics || !cpu_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(CpuMetrics));
    
    // Copiar información básica
    strncpy(metrics->model, cpu_model_cache, sizeof(metrics->model) - 1);
    strncpy(metrics->vendor, cpu_vendor_cache, sizeof(metrics->vendor) - 1);
    
    metrics->physical_cores = physical_cores_cache;
    metrics->logical_cores = logical_cores_cache;
    metrics->usage_percent = cpu_get_usage_percent();
    metrics->temperature = cpu_get_temperature();
    metrics->frequency_mhz = cpu_get_frequency_mhz();
    
    // Información por núcleo (simplificada para macOS)
    metrics->num_cores = logical_cores_cache;
    for (int i = 0; i < metrics->num_cores && i < MAX_CPU_CORES; i++) {
        metrics->cores[i].core_id = i;
        metrics->cores[i].usage_percent = metrics->usage_percent; // Aproximación
        metrics->cores[i].frequency_mhz = metrics->frequency_mhz;
        metrics->cores[i].temperature = metrics->temperature;
    }
    
    return 0;
}

int cpu_get_model(char *buffer, size_t size) {
    if (!buffer || size == 0 || !cpu_initialized) {
        return -1;
    }
    
    strncpy(buffer, cpu_model_cache, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float cpu_get_usage_percent(void) {
    if (!cpu_initialized) {
        return -1.0f;
    }
    
    return get_cpu_usage();
}

float cpu_get_temperature(void) {
    if (!cpu_initialized) {
        return -1.0f;
    }
    
    return get_cpu_temperature();
}

float cpu_get_frequency_mhz(void) {
    if (!cpu_initialized) {
        return -1.0f;
    }
    
    return get_cpu_frequency();
}

int cpu_get_core_info(int core_id, CoreInfo *core_info) {
    if (!core_info || core_id < 0 || core_id >= logical_cores_cache || !cpu_initialized) {
        return -1;
    }
    
    // Información simplificada por núcleo
    core_info->core_id = core_id;
    core_info->usage_percent = cpu_get_usage_percent(); // Aproximación
    core_info->frequency_mhz = cpu_get_frequency_mhz();
    core_info->temperature = cpu_get_temperature();
    
    return 0;
}

int cpu_get_physical_cores(void) {
    return cpu_initialized ? physical_cores_cache : -1;
}

int cpu_get_logical_cores(void) {
    return cpu_initialized ? logical_cores_cache : -1;
}

#endif /* __APPLE__ */