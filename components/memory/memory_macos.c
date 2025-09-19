/**
 * @file memory_macos.c
 * @brief Implementación de memoria para macOS usando vm_statistics y sysctl
 */

#ifdef __APPLE__

#include "memory_interface.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_host.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL memory_initialized = FALSE;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene estadísticas de memoria virtual
 */
static int get_vm_statistics(vm_statistics64_data_t *vm_stats) {
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    
    kern_return_t kr = host_statistics64(mach_host_self(),
                                        HOST_VM_INFO64,
                                        (host_info64_t)vm_stats,
                                        &count);
    
    return (kr == KERN_SUCCESS) ? 0 : -1;
}

/**
 * @brief Obtiene el tamaño de página del sistema
 */
static vm_size_t get_page_size(void) {
    vm_size_t page_size;
    size_t size = sizeof(page_size);
    
    if (sysctlbyname("hw.pagesize", &page_size, &size, NULL, 0) == 0) {
        return page_size;
    }
    
    return 4096; // Valor por defecto
}

/**
 * @brief Obtiene la memoria física total
 */
static uint64_t get_physical_memory(void) {
    uint64_t mem_size;
    size_t size = sizeof(mem_size);
    
    if (sysctlbyname("hw.memsize", &mem_size, &size, NULL, 0) == 0) {
        return mem_size;
    }
    
    return 0;
}

/**
 * @brief Obtiene información de swap
 */
static int get_swap_info(uint64_t *swap_total, uint64_t *swap_used) {
    size_t size;
    struct xsw_usage swap_usage;
    
    size = sizeof(swap_usage);
    if (sysctlbyname("vm.swapusage", &swap_usage, &size, NULL, 0) == 0) {
        *swap_total = swap_usage.xsu_total;
        *swap_used = swap_usage.xsu_used;
        return 0;
    }
    
    *swap_total = 0;
    *swap_used = 0;
    return -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int memory_init(void) {
    memory_initialized = TRUE;
    return 0;
}

void memory_cleanup(void) {
    memory_initialized = FALSE;
}

int memory_get_metrics(MemoryMetrics *metrics) {
    if (!metrics || !memory_initialized) {
        return -1;
    }
    
    vm_statistics64_data_t vm_stats;
    vm_size_t page_size;
    uint64_t physical_memory;
    uint64_t swap_total, swap_used;
    
    // Obtener estadísticas de VM
    if (get_vm_statistics(&vm_stats) != 0) {
        return -1;
    }
    
    // Obtener tamaño de página y memoria física
    page_size = get_page_size();
    physical_memory = get_physical_memory();
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(MemoryMetrics));
    
    // Calcular métricas de memoria
    uint64_t free_pages = vm_stats.free_count;
    uint64_t active_pages = vm_stats.active_count;
    uint64_t inactive_pages = vm_stats.inactive_count;
    uint64_t wired_pages = vm_stats.wire_count;
    uint64_t compressed_pages = vm_stats.compressor_page_count;
    uint64_t speculative_pages = vm_stats.speculative_count;
    
    // Memoria total
    metrics->ram_total_gb = (float)(physical_memory / (1024.0 * 1024.0 * 1024.0));
    
    // Memoria libre (páginas libres + especulativas)
    uint64_t free_bytes = (free_pages + speculative_pages) * page_size;
    metrics->ram_free_gb = (float)(free_bytes / (1024.0 * 1024.0 * 1024.0));
    
    // Memoria usada (activa + inactiva + wired)
    uint64_t used_bytes = (active_pages + inactive_pages + wired_pages) * page_size;
    metrics->ram_used_gb = (float)(used_bytes / (1024.0 * 1024.0 * 1024.0));
    
    // Memoria disponible (libre + inactiva + especulativa)
    uint64_t available_bytes = (free_pages + inactive_pages + speculative_pages) * page_size;
    metrics->ram_available_gb = (float)(available_bytes / (1024.0 * 1024.0 * 1024.0));
    
    // Porcentaje de uso
    if (physical_memory > 0) {
        metrics->ram_usage_percent = (metrics->ram_used_gb / metrics->ram_total_gb) * 100.0f;
    }
    
    // Memoria comprimida (específico de macOS)
    uint64_t compressed_bytes = compressed_pages * page_size;
    metrics->cached_gb = (float)(compressed_bytes / (1024.0 * 1024.0 * 1024.0));
    
    // Memoria wired (no se puede intercambiar)
    uint64_t wired_bytes = wired_pages * page_size;
    metrics->buffers_gb = (float)(wired_bytes / (1024.0 * 1024.0 * 1024.0));
    
    // Información de swap
    if (get_swap_info(&swap_total, &swap_used) == 0) {
        metrics->swap_total_gb = (float)(swap_total / (1024.0 * 1024.0 * 1024.0));
        metrics->swap_used_gb = (float)(swap_used / (1024.0 * 1024.0 * 1024.0));
        metrics->swap_free_gb = metrics->swap_total_gb - metrics->swap_used_gb;
        
        if (metrics->swap_total_gb > 0) {
            metrics->swap_usage_percent = (metrics->swap_used_gb / metrics->swap_total_gb) * 100.0f;
        }
    }
    
    // Memoria compartida (no disponible directamente en macOS)
    metrics->shared_gb = 0.0f;
    
    return 0;
}

float memory_get_ram_usage_percent(void) {
    if (!memory_initialized) {
        return -1.0f;
    }
    
    vm_statistics64_data_t vm_stats;
    vm_size_t page_size;
    uint64_t physical_memory;
    
    if (get_vm_statistics(&vm_stats) != 0) {
        return -1.0f;
    }
    
    page_size = get_page_size();
    physical_memory = get_physical_memory();
    
    if (physical_memory == 0) {
        return -1.0f;
    }
    
    // Calcular memoria usada
    uint64_t used_pages = vm_stats.active_count + vm_stats.inactive_count + vm_stats.wire_count;
    uint64_t used_bytes = used_pages * page_size;
    
    return (float)((used_bytes * 100.0) / physical_memory);
}

float memory_get_ram_total_gb(void) {
    if (!memory_initialized) {
        return -1.0f;
    }
    
    uint64_t physical_memory = get_physical_memory();
    return (float)(physical_memory / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_ram_used_gb(void) {
    if (!memory_initialized) {
        return -1.0f;
    }
    
    vm_statistics64_data_t vm_stats;
    vm_size_t page_size;
    
    if (get_vm_statistics(&vm_stats) != 0) {
        return -1.0f;
    }
    
    page_size = get_page_size();
    
    uint64_t used_pages = vm_stats.active_count + vm_stats.inactive_count + vm_stats.wire_count;
    uint64_t used_bytes = used_pages * page_size;
    
    return (float)(used_bytes / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_ram_free_gb(void) {
    if (!memory_initialized) {
        return -1.0f;
    }
    
    vm_statistics64_data_t vm_stats;
    vm_size_t page_size;
    
    if (get_vm_statistics(&vm_stats) != 0) {
        return -1.0f;
    }
    
    page_size = get_page_size();
    
    uint64_t free_pages = vm_stats.free_count + vm_stats.speculative_count;
    uint64_t free_bytes = free_pages * page_size;
    
    return (float)(free_bytes / (1024.0 * 1024.0 * 1024.0));
}

float memory_get_swap_usage_percent(void) {
    if (!memory_initialized) {
        return -1.0f;
    }
    
    uint64_t swap_total, swap_used;
    
    if (get_swap_info(&swap_total, &swap_used) != 0) {
        return -1.0f;
    }
    
    if (swap_total == 0) {
        return 0.0f;
    }
    
    return (float)((swap_used * 100.0) / swap_total);
}

#endif /* __APPLE__ */