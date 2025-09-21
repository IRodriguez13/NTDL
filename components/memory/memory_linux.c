/**
 * @file memory_linux.c
 * @brief Implementación de memoria para Linux
 */

#include "memory_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int parse_meminfo(DetailedRamInfo *ram_info, SwapInfo *swap_info)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return -1;

    char line[256];
    uint64_t value;

    // Inicializar estructuras
    if (ram_info)
    {
        memset(ram_info, 0, sizeof(DetailedRamInfo));
    }
    if (swap_info)
    {
        memset(swap_info, 0, sizeof(SwapInfo));
    }

    while (fgets(line, sizeof(line), f))
    {
        // Información de RAM
        if (ram_info)
        {
            if (sscanf(line, "MemTotal: %lu kB", &value) == 1)
            {
                ram_info->total_kb = value;
            }
            
            if (sscanf(line, "MemFree: %lu kB", &value) == 1)
            {
                ram_info->free_kb = value;
            }
            
            if (sscanf(line, "MemAvailable: %lu kB", &value) == 1)
            {
                ram_info->available_kb = value;
            }
            
            if (sscanf(line, "Cached: %lu kB", &value) == 1)
            {
                ram_info->cached_kb = value;
            }
            
            if (sscanf(line, "Buffers: %lu kB", &value) == 1)
            {
                ram_info->buffers_kb = value;
            }

            if (sscanf(line, "Shmem: %lu kB", &value) == 1)
            {
                ram_info->shared_kb = value;
            }
        }

        // Información de Swap
        if (swap_info)
        {
            if (sscanf(line, "SwapTotal: %lu kB", &value) == 1)
            {
                swap_info->total_kb = value;
            }
            else if (sscanf(line, "SwapFree: %lu kB", &value) == 1)
            {
                swap_info->free_kb = value;
            }
        }
    }

    fclose(f);

    // Calcular valores derivados
    if (ram_info)
    {
        // Si no tenemos MemAvailable, calcularlo
        if (ram_info->available_kb == 0 && ram_info->total_kb > 0)
        {
            ram_info->available_kb = ram_info->free_kb + ram_info->cached_kb + ram_info->buffers_kb;
        }

        // Calcular memoria usada
        ram_info->used_kb = ram_info->total_kb - ram_info->available_kb;

        // Calcular porcentaje de uso
        if (ram_info->total_kb > 0)
        {
            ram_info->usage_percent = ((float)ram_info->used_kb / ram_info->total_kb) * 100.0f;
        }
    }

    if (swap_info)
    {
        // Calcular swap usado
        swap_info->used_kb = swap_info->total_kb - swap_info->free_kb;

        // Calcular porcentaje de uso de swap
        if (swap_info->total_kb > 0)
        {
            swap_info->usage_percent = ((float)swap_info->used_kb / swap_info->total_kb) * 100.0f;
        }
    }

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int memory_init(void)
{
    if (initialized)
        return 0;

    // Verificar que podemos leer /proc/meminfo
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return -1;
    fclose(f);

    initialized = 1;
    return 0;
}

void memory_cleanup(void)
{
    initialized = 0;
}

int memory_get_ram_info(DetailedRamInfo *ram_info)
{
    if (!ram_info)
        return -1;

    return parse_meminfo(ram_info, NULL);
}

int memory_get_swap_info(SwapInfo *swap_info)
{
    if (!swap_info)
        return -1;

    return parse_meminfo(NULL, swap_info);
}

uint64_t memory_get_ram_total_kb(void)
{
    DetailedRamInfo ram_info;
    if (memory_get_ram_info(&ram_info) != 0)
    {
        return 0;
    }
    return ram_info.total_kb;
}

uint64_t memory_get_ram_free_kb(void)
{
    DetailedRamInfo ram_info;
    if (memory_get_ram_info(&ram_info) != 0)
    {
        return 0;
    }
    return ram_info.free_kb;
}

uint64_t memory_get_ram_available_kb(void)
{
    DetailedRamInfo ram_info;
    if (memory_get_ram_info(&ram_info) != 0)
    {
        return 0;
    }
    return ram_info.available_kb;
}

uint64_t memory_get_ram_used_kb(void)
{
    DetailedRamInfo ram_info;
    if (memory_get_ram_info(&ram_info) != 0)
    {
        return 0;
    }
    return ram_info.used_kb;
}

float memory_get_ram_usage_percent(void)
{
    DetailedRamInfo ram_info;
    if (memory_get_ram_info(&ram_info) != 0)
    {
        return -1.0f;
    }
    return ram_info.usage_percent;
}

float memory_get_swap_usage_percent(void)
{
    SwapInfo swap_info;
    if (memory_get_swap_info(&swap_info) != 0)
    {
        return -1.0f;
    }
    return swap_info.usage_percent;
}