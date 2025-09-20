/**
 * @file memory_windows.c
 * @brief Implementación de memoria para Windows usando GlobalMemoryStatusEx
 */

#ifdef _WIN32

#include "memory_interface.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL memory_initialized = FALSE;

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int memory_init(void)
{
    memory_initialized = TRUE;
    return 0;
}

void memory_cleanup(void)
{
    memory_initialized = FALSE;
}

int memory_get_metrics(MemoryMetrics *metrics)
{
    if (!metrics || !memory_initialized)
    {
        return -1;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(MemoryMetrics));

    // Información básica de RAM
    metrics->ram_total_gb = (float)(mem_status.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
    metrics->ram_used_gb = (float)((mem_status.ullTotalPhys - mem_status.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0));
    metrics->ram_free_gb = (float)(mem_status.ullAvailPhys / (1024.0 * 1024.0 * 1024.0));
    metrics->ram_available_gb = metrics->ram_free_gb; // En Windows, free y available son similares

    // Porcentajes
    metrics->ram_usage_percent = (float)mem_status.dwMemoryLoad;

    // Información de swap (archivo de paginación)
    metrics->swap_total_gb = (float)(mem_status.ullTotalPageFile / (1024.0 * 1024.0 * 1024.0));
    metrics->swap_used_gb = (float)((mem_status.ullTotalPageFile - mem_status.ullAvailPageFile) / (1024.0 * 1024.0 * 1024.0));
    metrics->swap_free_gb = (float)(mem_status.ullAvailPageFile / (1024.0 * 1024.0 * 1024.0));

    if (metrics->swap_total_gb > 0)
    {
        metrics->swap_usage_percent = (metrics->swap_used_gb / metrics->swap_total_gb) * 100.0f;
    }

    // Memoria virtual
    metrics->virtual_total_gb = (float)(mem_status.ullTotalVirtual / (1024.0 * 1024.0 * 1024.0));
    metrics->virtual_used_gb = (float)((mem_status.ullTotalVirtual - mem_status.ullAvailVirtual) / (1024.0 * 1024.0 * 1024.0));
    metrics->virtual_free_gb = (float)(mem_status.ullAvailVirtual / (1024.0 * 1024.0 * 1024.0));

    // En Windows no tenemos información directa de buffers/cache como en Linux
    metrics->buffers_gb = 0.0f;
    metrics->cached_gb = 0.0f;
    metrics->shared_gb = 0.0f;

    return 0;
}

float memory_get_ram_usage_percent(void)
{
    if (!memory_initialized)
    {
        return -1.0f;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1.0f;
    }

    return (float)mem_status.dwMemoryLoad;
}

float memory_get_ram_total_gb(void)
{
    if (!memory_initialized)
    {
        return -1.0f;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1.0f;
    }

    return (float)(mem_status.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_ram_used_gb(void)
{
    if (!memory_initialized)
    {
        return -1.0f;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1.0f;
    }

    return (float)((mem_status.ullTotalPhys - mem_status.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_ram_free_gb(void)
{
    if (!memory_initialized)
    {
        return -1.0f;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1.0f;
    }

    return (float)(mem_status.ullAvailPhys / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_swap_usage_percent(void)
{
    if (!memory_initialized)
    {
        return -1.0f;
    }

    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);

    if (!GlobalMemoryStatusEx(&mem_status))
    {
        return -1.0f;
    }

    if (mem_status.ullTotalPageFile == 0)
    {
        return 0.0f;
    }

    float used = (float)((mem_status.ullTotalPageFile - mem_status.ullAvailPageFile) / (1024.0 * 1024.0 * 1024.0));
    float total = (float)(mem_status.ullTotalPageFile / (1024.0 * 1024.0 * 1024.0));

    return (used / total) * 100.0f;
}

#endif /* _WIN32 */