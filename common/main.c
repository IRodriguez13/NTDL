// main.c
#include <stdio.h>
#include "common.h"

int main()
{
    // Creamos el struct CPU
    CpuInfo *info = alloc_cpu_info();
    if (!info)
    {
        perror("No se pudo reservar memoria para CpuInfo");
        return 1;
    }

    // Opcional: rellenar algunos datos para prueba
    snprintf(info->vendor, CPU_MODEL_LEN, "Intel");
    snprintf(info->model, CPU_MODEL_LEN, "i7-8700K");
    info->cores = 6;
    info->mhz = 3600.0;

    // Mostramos info
    print_cpu_info(info);

    // Liberamos memoria
    free_cpu_info(info);

    return 0;
}
