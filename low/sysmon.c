#include "sysmon.h"
#include <stdlib.h>
#include <string.h>

// Variables globales para almacenar el estado del sistema
static SystemStatus current_status;
static int initialized = 0;

int sysmon_init(void) {
    if (initialized) return 0;
    
    // Inicializar estructura
    memset(&current_status, 0, sizeof(SystemStatus));
    
    // Recolectar métricas iniciales
    collect_metrics(&current_status);
    
    initialized = 1;
    return 0;
}

int sysmon_get_metrics(SystemStatus *status) {
    if (!initialized) {
        if (sysmon_init() != 0) return -1;
    }
    
    // Actualizar métricas
    collect_metrics(&current_status);
    
    // Copiar al buffer proporcionado
    if (status) {
        memcpy(status, &current_status, sizeof(SystemStatus));
    }
    
    return 0;
}

float sysmon_get_cpu_usage(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_usage_percent;
}

unsigned long sysmon_get_ram_total(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.ram_total_kb;
}

unsigned long sysmon_get_ram_free(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.ram_free_kb;
}

unsigned long sysmon_get_ram_used(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.ram_total_kb - current_status.ram_free_kb;
}

float sysmon_get_ram_usage_percent(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    if (current_status.ram_total_kb > 0) {
        return ((float)(current_status.ram_total_kb - current_status.ram_free_kb) / 
                current_status.ram_total_kb) * 100.0f;
    }
    return 0.0f;
}

int sysmon_get_cpu_cores(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_cores;
}

const char* sysmon_get_cpu_model(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_model;
}

const char* sysmon_get_os_name(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.os_name;
}

const char* sysmon_get_disk_model(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.disk_model;
}

int sysmon_get_disk_health(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.disk_health;
}



void sysmon_cleanup(void) {
    // Por ahora no hay recursos específicos que limpiar
    initialized = 0;
}
