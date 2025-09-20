#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

void print_cpu_info(const CpuInfo *info)
{
    printf("\n==== CPU Info ====\n");
    printf("Vendor:   %s\n", info->vendor ? info->vendor : "Unknown");
    printf("Model:    %s\n", info->model ? info->model : "Unknown");
    printf("Cores:    %d\n", info->cores);
    printf("Freq MHz: %.2f\n", info->mhz);
    printf("\n==================\n");
}

void free_cpu_info(CpuInfo *info)
{
    if (info == NULL)
        return;

    if (info->vendor)
    {
        free(info->vendor);
        info->vendor = NULL;
    }

    if (info->model)
    {
        free(info->model);
        info->model = NULL;
    }

    free(info);
}

CpuInfo *alloc_cpu_info()
{
    CpuInfo *info = (CpuInfo *)malloc(sizeof(CpuInfo));

    if (info == NULL)
    {
        Kerror("malloc failed for CpuInfo");
        goto cleanup;
        return NULL;
    }

    // Inicializar estructura
    memset(info, 0, sizeof(CpuInfo));
    info->vendor = NULL;
    info->model = NULL;
    info->cores = 0;
    info->mhz = 0.0;

    // Obtener modelo de CPU
    char cpu_model[CPU_MODEL_LEN];
    if (get_cpu_model(cpu_model, CPU_MODEL_LEN) == 0)
    {
        info->model = strdup(cpu_model);
    }
    else
    {
        info->model = strdup("Unknown CPU Model");
    }

    if (info->model == NULL)
    {
        fprintf(stderr, "Error: strdup failed for model\n");
        goto cleanup;
    }

// Obtener información adicional según la plataforma
#ifdef _WIN32
    // En Windows, usar la función específica
    char vendor[64] = {0};
    char model_win[CPU_MODEL_LEN] = {0};
    int cores = 0;
    double frequency = 0.0;

    if (get_cpu_info_windows(vendor, model_win, &cores, &frequency) == 0)
    {
        info->vendor = strdup(vendor);
        if (info->vendor == NULL)
        {
            fprintf(stderr, "Error: strdup failed for vendor\n");
            goto cleanup;
        }

        info->cores = cores;
        info->mhz = frequency;

        // Si el modelo de Windows es mejor, usarlo
        if (strlen(model_win) > strlen(info->model))
        {
            free(info->model);
            info->model = strdup(model_win);
            if (info->model == NULL)
            {
                fprintf(stderr, "Error: strdup failed for Windows model\n");
                goto cleanup;
            }
        }
    }
    else
    {
        info->vendor = strdup("Unknown Vendor");
        info->cores = 0;
        info->mhz = 0.0;
    }
#else
    // En Linux, obtener información desde /proc/cpuinfo
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        int core_count = 0;

        while (fgets(line, sizeof(line), f))
        {
            // Contar procesadores (núcleos lógicos)
            if (strncmp(line, "processor", 9) == 0)
            {
                core_count++;
            }

            // Obtener vendor_id
            if (strncmp(line, "vendor_id", 9) == 0)
            {
                char *colon = strchr(line, ':');
                if (colon)
                {
                    colon += 2; // Saltar ": "

                    // Limpiar espacios y salto de línea
                    char *vendor_str = colon;
                    char *newline = strchr(vendor_str, '\n');
                    if (newline)
                        *newline = '\0';

                    // Limpiar espacios al final
                    int len = strlen(vendor_str);
                    while (len > 0 && vendor_str[len - 1] == ' ')
                    {
                        vendor_str[--len] = '\0';
                    }

                    info->vendor = strdup(vendor_str);
                    if (info->vendor == NULL)
                    {
                        fprintf(stderr, "Error: strdup failed for vendor_id\n");
                    }
                }
            }

            // Obtener frecuencia de CPU
            if (strncmp(line, "cpu MHz", 7) == 0)
            {
                char *colon = strchr(line, ':');
                if (colon)
                {
                    info->mhz = atof(colon + 2);
                }
            }
        }
        fclose(f);

        info->cores = core_count;
    }
    else
    {
        fprintf(stderr, "Warning: No se pudo abrir /proc/cpuinfo\n");
        info->vendor = strdup("Unknown Vendor");
        info->cores = 0;
        info->mhz = 0.0;
    }

    // Si no pudimos obtener vendor, usar valor por defecto
    if (info->vendor == NULL)
    {
        info->vendor = strdup("Unknown Vendor");
    }
#endif

    return info;

cleanup:
    if (info)
    {
        if (info->vendor)
            free(info->vendor);
        if (info->model)
            free(info->model);
        free(info);
    }
    return NULL;
}