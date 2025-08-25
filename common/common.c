#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

void print_cpu_info(const CpuInfo *info)
{

    printf("==== CPU Info ====\n");
    printf("Vendor:   %s\n", info->vendor ? info->vendor : "Unknown");
    printf("Model:    %s\n", info->model ? info->model : "Unknown");
    printf("Cores:    %d\n", info->cores);
    printf("Freq MHz: %.2f\n", info->mhz);
    printf("==================\n");
}

CpuInfo *alloc_cpu_info()
{
    CpuInfo *info = (CpuInfo *)malloc(sizeof(CpuInfo));

    if (info == NULL)
    {
        perror("NULL pointer");
        return NULL;
    }

    memset(info, 0, sizeof(CpuInfo));

    // Llenar información del CPU según la plataforma
    char cpu_model[CPU_MODEL_LEN];

    // Obtener modelo de CPU
    if (get_cpu_model(cpu_model, CPU_MODEL_LEN) == 0)
    {
        info->model = strdup(cpu_model);
    }
    else
    {
        info->model = strdup("Unknown");
    }

// Obtener información adicional según la plataforma
#ifdef _WIN32
    // En Windows, usar la función específica
    char vendor[64] = {0};
    int cores = 0;
    double frequency = 0.0;

    if (get_cpu_info_windows(vendor, cpu_model, &cores, &frequency) == 0)
    {
        info->vendor = strdup(vendor);
        info->cores = cores;
        info->mhz = frequency;
    }
    else
    {
        info->vendor = strdup("Unknown");
        info->cores = 0;
        info->mhz = 0.0;
    }
#else
    // En Linux, obtener información básica
    info->vendor = strdup("Linux CPU");

    // Obtener número de núcleos desde /proc/cpuinfo
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        int core_count = 0;
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "processor", 9) == 0)
            {
                core_count++;
            }
        }
        fclose(f);
        info->cores = core_count;
    }
    else
    {
        info->cores = 0;
    }

    // Obtener frecuencia desde /proc/cpuinfo
    f = fopen("/proc/cpuinfo", "r");
    if (f)
    {
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, "cpu MHz", 7) == 0)
            {
                char *colon = strchr(line, ':');
                if (colon)
                {
                    info->mhz = atof(colon + 2);
                    break;
                }
            }
        }
        fclose(f);
    }
    else
    {
        info->mhz = 0.0;
    }
#endif

    return info;
}

void free_cpu_info(CpuInfo *info)
{
    if (info == NULL)
    {
        perror("NULL pointer");
        return;
    }

    free(info->vendor);
    free(info->model);
    free(info);
}


