// main.c
#include <stdio.h>
#include "common.h"

int main()
{
    // Creamos el struct CPU
    CpuInfo *info = alloc_cpu_info();
    if (!info)
    {
        Kerror("No se pudo reservar memoria para CpuInfo");
        return 1;
    }
    // Mostramos info
    print_cpu_info(info);

    // Liberamos memoria
    free_cpu_info(info);

    // ================= DISK INFO =================

    return 0;
}
