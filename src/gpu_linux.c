// src/gpu_linux.c
#include "gpu_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>

// Función para detectar GPUs NVIDIA
int detect_nvidia_gpu(GpuInfo *info)
{
    FILE *fp;
    char line[256];

    // Verificar si nvidia-smi está disponible
    fp = popen("which nvidia-smi 2>/dev/null", "r");
    if (!fp || !fgets(line, sizeof(line), fp))
    {
        if (fp)
            pclose(fp);
        return -1;
    }
    pclose(fp);

    // Obtener información básica de la GPU
    fp = popen("nvidia-smi --query-gpu=name,driver_version,memory.total,memory.used,temperature.gpu,utilization.gpu,clocks.gr,clocks.mem --format=csv,noheader,nounits 2>/dev/null", "r");
    
    if (!fp)
        return -1;

    if (fgets(line, sizeof(line), fp))
    {
        char name[128], driver[64];
        int memory_total, memory_used, temperature, utilization, core_clock, memory_clock;

        if (sscanf(line, "%127[^,], %63[^,], %d, %d, %d, %d, %d, %d",
                   name, driver, &memory_total, &memory_used,
                   &temperature, &utilization, &core_clock, &memory_clock) == 8)
        {

            strncpy(info->name, name, sizeof(info->name) - 1);
            strncpy(info->vendor, "NVIDIA", sizeof(info->vendor) - 1);
            strncpy(info->driver_version, driver, sizeof(info->driver_version) - 1);
            info->memory_total_mb = memory_total;
            info->memory_used_mb = memory_used;
            info->temperature = temperature;
            info->utilization_percent = (float)utilization;
            info->core_clock_mhz = core_clock;
            info->memory_clock_mhz = memory_clock;

            pclose(fp);
            return 0;
        }
    }

    pclose(fp);
    return -1;
}

// Función para detectar GPUs AMD
int detect_amd_gpu(GpuInfo *info)
{
    FILE *fp;
    char line[256];
    glob_t glob_result;

    // Buscar dispositivos DRM AMD
    int glob_status = glob("/sys/class/drm/card*/device/vendor", GLOB_NOSORT, NULL, &glob_result);
    if (glob_status != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < glob_result.gl_pathc; i++)
    {
        fp = fopen(glob_result.gl_pathv[i], "r");
        if (!fp)
            continue;

        if (fgets(line, sizeof(line), fp))
        {
            int vendor_id;
            if (sscanf(line, "0x%x", &vendor_id) == 1 && vendor_id == 0x1002)
            { // AMD vendor ID
                fclose(fp);

                // Obtener nombre de la GPU
                char device_path[512];
                char *card_path = strdup(glob_result.gl_pathv[i]);
                char *device_pos = strstr(card_path, "/device/vendor");
                if (device_pos)
                {
                    *device_pos = '\0';
                    snprintf(device_path, sizeof(device_path), "%s/device/product_name", card_path);

                    FILE *name_fp = fopen(device_path, "r");
                    if (name_fp)
                    {
                        if (fgets(line, sizeof(line), name_fp))
                        {
                            line[strcspn(line, "\n")] = 0; // Quitar newline
                            strncpy(info->name, line, sizeof(info->name) - 1);
                        }
                        fclose(name_fp);
                    }
                    else
                    {
                        strncpy(info->name, "AMD GPU", sizeof(info->name) - 1);
                    }

                    strncpy(info->vendor, "AMD", sizeof(info->vendor) - 1);

                    // Obtener temperatura (si está disponible)
                    snprintf(device_path, sizeof(device_path), "%s/hwmon/hwmon*/temp1_input", card_path);
                    glob_t temp_glob;
                    if (glob(device_path, GLOB_NOSORT, NULL, &temp_glob) == 0 && temp_glob.gl_pathc > 0)
                    {
                        FILE *temp_fp = fopen(temp_glob.gl_pathv[0], "r");
                        if (temp_fp)
                        {
                            int temp_millicelsius;
                            if (fscanf(temp_fp, "%d", &temp_millicelsius) == 1)
                            {
                                info->temperature = temp_millicelsius / 1000;
                            }
                            fclose(temp_fp);
                        }
                        globfree(&temp_glob);
                    }

                    // Valores por defecto para AMD (más difícil de obtener)
                    info->memory_total_mb = -1; // Requiere herramientas específicas
                    info->memory_used_mb = -1;
                    info->utilization_percent = -1.0f;
                    info->core_clock_mhz = -1;
                    info->memory_clock_mhz = -1;
                    strncpy(info->driver_version, "Unknown", sizeof(info->driver_version) - 1);
                }

                free(card_path);
                globfree(&glob_result);
                return 0;
            }
        }
        fclose(fp);
    }

    globfree(&glob_result);
    return -1;
}

// Función para detectar GPUs Intel
int detect_intel_gpu(GpuInfo *info)
{
    FILE *fp;
    char line[256];

    // Verificar si hay GPU Intel integrada
    fp = fopen("/sys/class/drm/card0/device/vendor", "r");
    if (!fp)
        return -1;

    if (fgets(line, sizeof(line), fp))
    {
        int vendor_id;
        if (sscanf(line, "0x%x", &vendor_id) == 1 && vendor_id == 0x8086)
        { // Intel vendor ID
            fclose(fp);

            strncpy(info->name, "Intel Integrated GPU", sizeof(info->name) - 1);
            strncpy(info->vendor, "Intel", sizeof(info->vendor) - 1);
            strncpy(info->driver_version, "Unknown", sizeof(info->driver_version) - 1);

            // Para Intel, la información es limitada sin herramientas específicas
            info->memory_total_mb = -1;
            info->memory_used_mb = -1;
            info->temperature = -1;
            info->utilization_percent = -1.0f;
            info->core_clock_mhz = -1;
            info->memory_clock_mhz = -1;

            return 0;
        }
    }

    fclose(fp);
    return -1;
}

// Función principal para obtener información de GPU
GpuInfo *alloc_gpu_info()
{
    GpuInfo *info = (GpuInfo *)malloc(sizeof(GpuInfo));
    if (!info)
    {
        Kerror("malloc failed for GpuInfo");
        goto cleanup;
        return NULL;
    }

    // Inicializar estructura
    memset(info, 0, sizeof(GpuInfo));
    info->memory_total_mb = -1;
    info->memory_used_mb = -1;
    info->temperature = -1;
    info->utilization_percent = -1.0f;
    info->core_clock_mhz = -1;
    info->memory_clock_mhz = -1;

    // Intentar detectar GPUs en orden de preferencia
    if (detect_nvidia_gpu(info) == 0)
    {
        printf("DEBUG: GPU NVIDIA detectada\n");
        return info;
    }

    if (detect_amd_gpu(info) == 0)
    {
        printf("DEBUG: GPU AMD detectada\n");
        return info;
    }

    if (detect_intel_gpu(info) == 0)
    {
        printf("DEBUG: GPU Intel detectada\n");
        return info;
    }

    // Si no se detecta ninguna GPU específica
    strncpy(info->name, "Unknown GPU", sizeof(info->name) - 1);
    strncpy(info->vendor, "Unknown", sizeof(info->vendor) - 1);
    strncpy(info->driver_version, "Unknown", sizeof(info->driver_version) - 1);

    return info;

    cleanup:
        free(info);
    return NULL;
}

// Función para liberar memoria de GPU info
void free_gpu_info(GpuInfo *info)
{
    if (info)
    {
        free(info);
    }
}

// Función para imprimir información de GPU
void print_gpu_info(const GpuInfo *info)
{
    if (!info)
        return;

    printf("\n==== GPU Info ====\n");
    printf("Name:         %s\n", info->name);
    printf("Vendor:       %s\n", info->vendor);
    printf("Driver:       %s\n", info->driver_version);

    if (info->memory_total_mb > 0)
    {
        printf("VRAM Total:   %d MB\n", info->memory_total_mb);
    }
    if (info->memory_used_mb >= 0)
    {
        printf("VRAM Used:    %d MB\n", info->memory_used_mb);
    }
    if (info->temperature > 0)
    {
        printf("Temperature:  %d°C\n", info->temperature);
    }
    if (info->utilization_percent >= 0)
    {
        printf("Utilization:  %.1f%%\n", info->utilization_percent);
    }
    if (info->core_clock_mhz > 0)
    {
        printf("Core Clock:   %d MHz\n", info->core_clock_mhz);
    }
    if (info->memory_clock_mhz > 0)
    {
        printf("Memory Clock: %d MHz\n", info->memory_clock_mhz);
    }

    printf("==================\n");
}