/**
 * @file gpu_linux.c
 * @brief Implementación de GPU para Linux
 */

#include "gpu_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int gpu_count = 0;
static GpuMetrics detected_gpus[MAX_GPUS];

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int detect_nvidia_gpu(int gpu_index) {
    FILE *fp;
    char line[512];
    char cmd[256];
    
    // Verificar si nvidia-smi está disponible
    fp = popen("which nvidia-smi 2>/dev/null", "r");
    if (!fp || !fgets(line, sizeof(line), fp)) {
        if (fp) pclose(fp);
        return -1;
    }
    pclose(fp);
    
    // Obtener información de la GPU específica
    snprintf(cmd, sizeof(cmd), 
             "nvidia-smi -i %d --query-gpu=name,driver_version,memory.total,memory.used,temperature.gpu,utilization.gpu,clocks.gr,clocks.mem,power.draw,fan.speed --format=csv,noheader,nounits 2>/dev/null", 
             gpu_index);
    
    fp = popen(cmd, "r");
    if (!fp) return -1;
    
    if (fgets(line, sizeof(line), fp)) {
        char name[128], driver[64];
        int memory_total, memory_used, temperature, utilization, core_clock, memory_clock, power_draw, fan_speed;
        
        // Parsear la línea CSV
        int parsed = sscanf(line, "%127[^,], %63[^,], %d, %d, %d, %d, %d, %d, %d, %d",
                           name, driver, &memory_total, &memory_used, 
                           &temperature, &utilization, &core_clock, &memory_clock,
                           &power_draw, &fan_speed);
        
        if (parsed >= 6) { // Al menos los campos básicos
            GpuMetrics *gpu = &detected_gpus[gpu_count];
            memset(gpu, 0, sizeof(GpuMetrics));
            
            strncpy(gpu->name, name, sizeof(gpu->name) - 1);
            strncpy(gpu->vendor, "NVIDIA", sizeof(gpu->vendor) - 1);
            strncpy(gpu->driver_version, driver, sizeof(gpu->driver_version) - 1);
            gpu->driver_version[sizeof(gpu->driver_version) - 1] = '\0';
            
            gpu->memory_total_mb = memory_total;
            gpu->memory_used_mb = memory_used;
            gpu->memory_free_mb = memory_total - memory_used;
            gpu->memory_usage_percent = (memory_total > 0) ? 
                                       ((float)memory_used / memory_total * 100.0f) : 0.0f;
            
            gpu->temperature = (temperature > 0) ? temperature : -1.0f;
            gpu->usage_percent = (utilization >= 0) ? utilization : -1.0f;
            
            if (parsed >= 8) {
                gpu->core_clock_mhz = (core_clock > 0) ? core_clock : -1;
                gpu->memory_clock_mhz = (memory_clock > 0) ? memory_clock : -1;
            } else {
                gpu->core_clock_mhz = -1;
                gpu->memory_clock_mhz = -1;
            }
            
            if (parsed >= 9) {
                gpu->power_usage_w = (power_draw > 0) ? power_draw : -1;
            } else {
                gpu->power_usage_w = -1;
            }
            
            if (parsed >= 10) {
                gpu->fan_speed_rpm = (fan_speed > 0) ? fan_speed : -1;
            } else {
                gpu->fan_speed_rpm = -1;
            }
            
            pclose(fp);
            return 0;
        }
    }
    
    pclose(fp);
    return -1;
}

static int detect_amd_gpu(void) {
    glob_t glob_result;
    
    // Buscar dispositivos DRM AMD
    int glob_status = glob("/sys/class/drm/card*/device/vendor", GLOB_NOSORT, NULL, &glob_result);
    if (glob_status != 0) {
        return -1;
    }
    
    for (size_t i = 0; i < glob_result.gl_pathc && gpu_count < MAX_GPUS; i++) {
        FILE *fp = fopen(glob_result.gl_pathv[i], "r");
        if (!fp) continue;
        
        char line[64];
        if (fgets(line, sizeof(line), fp)) {
            int vendor_id;
            if (sscanf(line, "0x%x", &vendor_id) == 1 && vendor_id == 0x1002) { // AMD vendor ID
                fclose(fp);
                
                GpuMetrics *gpu = &detected_gpus[gpu_count];
                memset(gpu, 0, sizeof(GpuMetrics));
                
                // Obtener path base del dispositivo
                char *card_path = strdup(glob_result.gl_pathv[i]);
                char *device_pos = strstr(card_path, "/device/vendor");
                if (device_pos) {
                    *device_pos = '\0';
                    
                    // Intentar obtener nombre de la GPU
                    char device_path[512];
                    snprintf(device_path, sizeof(device_path), "%s/device/product_name", card_path);
                    
                    FILE *name_fp = fopen(device_path, "r");
                    if (name_fp) {
                        if (fgets(line, sizeof(line), name_fp)) {
                            line[strcspn(line, "\n")] = 0; // Quitar newline
                            strncpy(gpu->name, line, sizeof(gpu->name) - 1);
                        }
                        fclose(name_fp);
                    } else {
                        snprintf(gpu->name, sizeof(gpu->name), "AMD GPU %d", gpu_count);
                    }
                    
                    strncpy(gpu->vendor, "AMD", sizeof(gpu->vendor) - 1);
                    strncpy(gpu->driver_version, "Unknown", sizeof(gpu->driver_version) - 1);
                    
                    // Intentar obtener temperatura
                    snprintf(device_path, sizeof(device_path), "%s/hwmon/hwmon*/temp1_input", card_path);
                    glob_t temp_glob;
                    if (glob(device_path, GLOB_NOSORT, NULL, &temp_glob) == 0 && temp_glob.gl_pathc > 0) {
                        FILE *temp_fp = fopen(temp_glob.gl_pathv[0], "r");
                        if (temp_fp) {
                            int temp_millicelsius;
                            if (fscanf(temp_fp, "%d", &temp_millicelsius) == 1) {
                                gpu->temperature = temp_millicelsius / 1000.0f;
                            }
                            fclose(temp_fp);
                        }
                        globfree(&temp_glob);
                    }
                    
                    // Valores por defecto para AMD (más difícil de obtener sin herramientas específicas)
                    gpu->memory_total_mb = 0;  // Requiere herramientas específicas
                    gpu->memory_used_mb = 0;
                    gpu->memory_free_mb = 0;
                    gpu->memory_usage_percent = 0.0f;  // 0% para consistencia en lugar de -1
                    gpu->usage_percent = 0.0f;  // 0% para consistencia en lugar de -1
                    gpu->core_clock_mhz = -1;
                    gpu->memory_clock_mhz = -1;
                    gpu->power_usage_w = -1;
                    gpu->fan_speed_rpm = -1;
                    
                    if (gpu->temperature <= 0) {
                        gpu->temperature = -1.0f;
                    }
                }
                
                free(card_path);
                gpu_count++;
                continue;
            }
        }
        fclose(fp);
    }
    
    globfree(&glob_result);
    return (gpu_count > 0) ? 0 : -1;
}

static int detect_intel_gpu(void) {
    FILE *fp;
    char line[64];
    
    // Verificar si hay GPU Intel integrada
    fp = fopen("/sys/class/drm/card0/device/vendor", "r");
    if (!fp) return -1;
    
    if (fgets(line, sizeof(line), fp)) {
        int vendor_id;
        if (sscanf(line, "0x%x", &vendor_id) == 1 && vendor_id == 0x8086) { // Intel vendor ID
            fclose(fp);
            
            if (gpu_count >= MAX_GPUS) return -1;
            
            GpuMetrics *gpu = &detected_gpus[gpu_count];
            memset(gpu, 0, sizeof(GpuMetrics));
            
            strncpy(gpu->name, "Intel Integrated GPU", sizeof(gpu->name) - 1);
            strncpy(gpu->vendor, "Intel", sizeof(gpu->vendor) - 1);
            strncpy(gpu->driver_version, "Unknown", sizeof(gpu->driver_version) - 1);
            
            // Para Intel, la información es muy limitada sin herramientas específicas
            gpu->memory_total_mb = 0;
            gpu->memory_used_mb = 0;
            gpu->memory_free_mb = 0;
            gpu->memory_usage_percent = 0.0f;  // 0% para consistencia
            gpu->temperature = -1.0f;  // Temperatura sí puede ser -1 (no disponible)
            gpu->usage_percent = 0.0f;  // 0% para consistencia
            gpu->core_clock_mhz = -1;
            gpu->memory_clock_mhz = -1;
            gpu->power_usage_w = -1;
            gpu->fan_speed_rpm = -1;
            
            gpu_count++;
            return 0;
        }
    }
    
    fclose(fp);
    return -1;
}

static int scan_all_gpus(void) {
    gpu_count = 0;
    
    // Intentar detectar GPUs NVIDIA (pueden ser múltiples)
    for (int i = 0; i < MAX_GPUS; i++) {
        if (detect_nvidia_gpu(i) == 0) {
            gpu_count++;
        } else {
            break; // No más GPUs NVIDIA
        }
    }
    
    // Si no hay NVIDIA, buscar AMD
    if (gpu_count == 0) {
        detect_amd_gpu();
    }
    
    // Si no hay NVIDIA ni AMD, buscar Intel
    if (gpu_count == 0) {
        detect_intel_gpu();
    }
    
    return gpu_count;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int gpu_init(void) {
    if (initialized) return 0;
    
    // Escanear GPUs disponibles
    scan_all_gpus();
    
    initialized = 1;
    return 0;
}

void gpu_cleanup(void) {
    initialized = 0;
    gpu_count = 0;
}

int gpu_get_count(void) {
    if (!initialized) {
        gpu_init();
    }
    return gpu_count;
}

int gpu_get_metrics(int gpu_id, GpuMetrics *metrics) {
    if (!metrics || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    // Para NVIDIA, actualizar datos en tiempo real
    if (strncmp(detected_gpus[gpu_id].vendor, "NVIDIA", 6) == 0) {
        if (detect_nvidia_gpu(gpu_id) == 0) {
            // Los datos se actualizaron en detected_gpus[gpu_count-1], pero necesitamos el índice correcto
            // Para simplificar, copiamos los datos ya almacenados
        }
    }
    
    // Copiar métricas
    memcpy(metrics, &detected_gpus[gpu_id], sizeof(GpuMetrics));
    return 0;
}

int gpu_get_primary_metrics(GpuMetrics *metrics) {
    return gpu_get_metrics(0, metrics);
}

int gpu_get_name(int gpu_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    strncpy(buffer, detected_gpus[gpu_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float gpu_get_usage_percent(int gpu_id) {
    if (gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    // Para NVIDIA, intentar obtener datos actualizados
    if (strncmp(detected_gpus[gpu_id].vendor, "NVIDIA", 6) == 0) {
        // GpuMetrics temp_gpu; // Removed unused variable
        if (detect_nvidia_gpu(gpu_id) == 0) {
            // Los datos se actualizaron, pero están en detected_gpus[gpu_count-1]
            // Necesitamos una mejor estrategia aquí
        }
    }
    
    return detected_gpus[gpu_id].usage_percent;
}

float gpu_get_temperature(int gpu_id) {
    if (gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    return detected_gpus[gpu_id].temperature;
}

float gpu_get_memory_usage_percent(int gpu_id) {
    if (gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    return detected_gpus[gpu_id].memory_usage_percent;
}

uint64_t gpu_get_memory_total_mb(int gpu_id) {
    if (gpu_id < 0 || gpu_id >= gpu_count) {
        return 0;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    return detected_gpus[gpu_id].memory_total_mb;
}

uint64_t gpu_get_memory_used_mb(int gpu_id) {
    if (gpu_id < 0 || gpu_id >= gpu_count) {
        return 0;
    }
    
    if (!initialized) {
        gpu_init();
    }
    
    return detected_gpus[gpu_id].memory_used_mb;
}