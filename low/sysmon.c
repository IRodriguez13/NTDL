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

// Funciones básicas
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
    return current_status.ram_used_kb;
}

float sysmon_get_ram_usage_percent(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.ram_usage_percent;
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

// Funciones detalladas de CPU
float sysmon_get_cpu_temperature(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_temperature;
}

float sysmon_get_cpu_frequency(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_frequency;
}

int sysmon_get_cpu_threads(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.cpu_threads;
}

CoreInfo* sysmon_get_core_info(int core_id) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    if (core_id >= 0 && core_id < current_status.num_cores) {
        return &current_status.cores[core_id];
    }
    return NULL;
}

int sysmon_get_num_cores(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.num_cores;
}

// Funciones de GPU
const char* sysmon_get_gpu_model(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.gpu_model;
}

float sysmon_get_gpu_usage(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.gpu_usage_percent;
}

float sysmon_get_gpu_temperature(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.gpu_temperature;
}

float sysmon_get_gpu_memory_usage(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.gpu_memory_usage_percent;
}

// Funciones de sensores
SensorInfo* sysmon_get_sensor_info(int sensor_id) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    if (sensor_id >= 0 && sensor_id < current_status.num_sensors) {
        return &current_status.sensors[sensor_id];
    }
    return NULL;
}

int sysmon_get_num_sensors(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.num_sensors;
}

// Funciones de motherboard
MotherboardInfo* sysmon_get_motherboard_info(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return &current_status.motherboard;
}

// Funciones de discos
DiskInfo* sysmon_get_disk_info(int disk_id) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    if (disk_id >= 0 && disk_id < current_status.num_disks) {
        return &current_status.disks[disk_id];
    }
    return NULL;
}

int sysmon_get_num_disks(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.num_disks;
}

// Funciones de red
NetworkInfo* sysmon_get_network_info(int interface_id) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    if (interface_id >= 0 && interface_id < current_status.num_network_interfaces) {
        return &current_status.network_interfaces[interface_id];
    }
    return NULL;
}

int sysmon_get_num_network_interfaces(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.num_network_interfaces;
}

// Funciones de energía
float sysmon_get_power_consumption(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.power_consumption;
}

float sysmon_get_battery_percent(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.battery_percent;
}

int sysmon_get_ac_connected(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.ac_connected;
}

// Funciones de carga del sistema
float sysmon_get_load_average_1min(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.load_average_1min;
}

float sysmon_get_load_average_5min(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.load_average_5min;
}

float sysmon_get_load_average_15min(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.load_average_15min;
}

// Funciones de procesos
int sysmon_get_total_processes(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.total_processes;
}

int sysmon_get_running_processes(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.running_processes;
}

int sysmon_get_sleeping_processes(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.sleeping_processes;
}

int sysmon_get_stopped_processes(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.stopped_processes;
}

int sysmon_get_zombie_processes(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.zombie_processes;
}

// Funciones de información del sistema
const char* sysmon_get_os_version(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.os_version;
}

const char* sysmon_get_kernel_version(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.kernel_version;
}

const char* sysmon_get_hostname(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.hostname;
}

unsigned long sysmon_get_timestamp(void) {
    if (!initialized) sysmon_init();
    collect_metrics(&current_status);
    return current_status.timestamp;
}

void sysmon_cleanup(void) {
    // Por ahora no hay recursos específicos que limpiar
    initialized = 0;
}
