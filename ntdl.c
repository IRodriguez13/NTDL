/**
 * @file ntdl.c
 * @brief Implementación de la API Externa de SysMon
 */

#include "ntdl.h"
#include "core/sysmon_core.h"
#include "components/cpu/cpu_interface.h"
#include "components/gpu/gpu_interface.h"
#include "components/memory/memory_interface.h"
#include "components/disk/disk_interface.h"
#include "components/network/network_interface.h"
#include "components/sensors/sensors_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static SystemStatus g_system_status;
static int g_status_valid = 0;
// Error buffer removed - using direct error strings instead

/* ============================================================================
 * FUNCIONES DE INICIALIZACIÓN Y LIMPIEZA
 * ============================================================================ */

int sysmon_init(void)
{
    int result = sysmon_core_init();
    if (result == 0)
    {
        g_status_valid = 0; // Invalidar datos previos
    }
    return result;
}

void sysmon_cleanup(void)
{
    sysmon_core_cleanup();
    g_status_valid = 0;
}

/* ============================================================================
 * FUNCIÓN PRINCIPAL DE RECOLECCIÓN DE MÉTRICAS
 * ============================================================================ */

int sysmon_collect_metrics(SystemStatus *status)
{
    if (!status)
        return -1;

    int result = sysmon_core_collect_all_metrics(status);
    if (result == 0)
    {
        // Actualizar cache global
        memcpy(&g_system_status, status, sizeof(SystemStatus));
        g_status_valid = 1;
    }
    return result;
}

/* ============================================================================
 * FUNCIONES AUXILIARES INTERNAS
 * ============================================================================ */

static int ensure_status_valid(void)
{
    if (!g_status_valid)
    {
        if (sysmon_collect_metrics(&g_system_status) != 0)
        {
            return -1;
        }
    }
    return 0;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN BÁSICA DEL SISTEMA
 * ============================================================================ */

const char *sysmon_get_os_name(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    return g_system_status.os_name;
}

const char *sysmon_get_os_version(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    return g_system_status.os_version;
}

const char *sysmon_get_kernel_version(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    return g_system_status.kernel_version;
}

const char *sysmon_get_hostname(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    return g_system_status.hostname;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE CPU
 * ============================================================================ */

const char *sysmon_get_cpu_model(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    return g_system_status.cpu.model;
}

int sysmon_get_cpu_cores(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.cpu.physical_cores;
}

int sysmon_get_cpu_threads(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.cpu.logical_cores;
}

float sysmon_get_cpu_usage_percent(void)
{
    // Para uso de CPU, siempre obtener datos frescos
    float usage = cpu_get_usage_percent();
    if (usage >= 0)
    {
        return usage;
    }

    // Fallback al cache
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.cpu.usage_percent;
}

float sysmon_get_cpu_temperature(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.cpu.temperature;
}

float sysmon_get_cpu_frequency(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.cpu.current_frequency_mhz;
}

const CoreInfo *sysmon_get_core_info(int core_id)
{
    if (ensure_status_valid() != 0)
        return NULL;
    if (core_id < 0 || core_id >= g_system_status.cpu.num_cores_info)
        return NULL;
    return &g_system_status.cpu.cores[core_id];
}

int sysmon_get_num_cores(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.cpu.num_cores_info;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE MEMORIA RAM
 * ============================================================================ */

unsigned long sysmon_get_ram_total_kb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.ram.total_kb;
}

unsigned long sysmon_get_ram_free_kb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.ram.free_kb;
}

unsigned long sysmon_get_ram_used_kb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.ram.used_kb;
}

float sysmon_get_ram_usage_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.ram.usage_percent;
}

const DetailedRamInfo *sysmon_get_detailed_ram_info(void)
{
    if (ensure_status_valid() != 0)
        return NULL;
    return &g_system_status.ram;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE GPU
 * ============================================================================ */

const char *sysmon_get_gpu_model(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    if (g_system_status.num_gpus == 0)
        return "No GPU";
    return g_system_status.gpus[0].name;
}

const char *sysmon_get_gpu_driver(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    if (g_system_status.num_gpus == 0)
        return "No GPU";
    return g_system_status.gpus[0].driver_version;
}

const char *sysmon_get_gpu_vendor(void)
{
    if (ensure_status_valid() != 0)
        return "Unknown";
    if (g_system_status.num_gpus == 0)
        return "No GPU";
    return g_system_status.gpus[0].vendor;
}

unsigned long sysmon_get_gpu_memory_total_mb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    if (g_system_status.num_gpus == 0)
        return 0;
    return (unsigned long)g_system_status.gpus[0].memory_total_mb;
}

unsigned long sysmon_get_gpu_memory_used_mb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    if (g_system_status.num_gpus == 0)
        return 0;
    return (unsigned long)g_system_status.gpus[0].memory_used_mb;
}

unsigned long sysmon_get_gpu_memory_free_mb(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    if (g_system_status.num_gpus == 0)
        return 0;
    return (unsigned long)g_system_status.gpus[0].memory_free_mb;
}

float sysmon_get_gpu_usage_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    if (g_system_status.num_gpus == 0)
        return -1.0f;
    return g_system_status.gpus[0].usage_percent;
}

float sysmon_get_gpu_temperature(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    if (g_system_status.num_gpus == 0)
        return -1.0f;
    return g_system_status.gpus[0].temperature;
}

float sysmon_get_gpu_memory_usage_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    if (g_system_status.num_gpus == 0)
        return -1.0f;
    return g_system_status.gpus[0].memory_usage_percent;
}

int sysmon_get_gpu_fan_speed(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    if (g_system_status.num_gpus == 0)
        return -1;
    return g_system_status.gpus[0].fan_speed_rpm;
}

int sysmon_get_gpu_power_usage_w(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    if (g_system_status.num_gpus == 0)
        return -1;
    return g_system_status.gpus[0].power_usage_w;
}

int sysmon_get_gpu_clock_mhz(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    if (g_system_status.num_gpus == 0)
        return -1;
    return g_system_status.gpus[0].core_clock_mhz;
}

int sysmon_get_gpu_memory_clock_mhz(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    if (g_system_status.num_gpus == 0)
        return -1;
    return g_system_status.gpus[0].memory_clock_mhz;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE DISCOS
 * ============================================================================ */

const DiskInfo *sysmon_get_disk_info(int disk_id)
{
    if (ensure_status_valid() != 0)
        return NULL;
    if (disk_id < 0 || disk_id >= g_system_status.num_disks)
        return NULL;
    return &g_system_status.disks[disk_id];
}

int sysmon_get_num_disks(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return g_system_status.num_disks;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE RED
 * ============================================================================ */

const NetworkInfo *sysmon_get_network_info(int interface_id)
{
    if (ensure_status_valid() != 0)
        return NULL;
    if (interface_id < 0 || interface_id >= g_system_status.num_network_interfaces)
        return NULL;
    return &g_system_status.network_interfaces[interface_id];
}

int sysmon_get_num_network_interfaces(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return g_system_status.num_network_interfaces;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE SENSORES
 * ============================================================================ */

const SensorInfo *sysmon_get_sensor_info(int sensor_id)
{
    if (ensure_status_valid() != 0)
        return NULL;
    if (sensor_id < 0 || sensor_id >= g_system_status.num_sensors)
        return NULL;
    return &g_system_status.sensors[sensor_id];
}

int sysmon_get_num_sensors(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return g_system_status.num_sensors;
}

/* ============================================================================
 * FUNCIONES ADICIONALES PARA LA API
 * ============================================================================ */

int sysmon_get_num_gpus(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return g_system_status.num_gpus;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE CARGA DEL SISTEMA
 * ============================================================================ */

float sysmon_get_load_average_1min(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_average_1min;
}

float sysmon_get_load_average_5min(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_average_5min;
}

float sysmon_get_load_average_15min(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_average_15min;
}

float sysmon_get_load_1min_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_1min_percent;
}

float sysmon_get_load_5min_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_5min_percent;
}

float sysmon_get_load_15min_percent(void)
{
    if (ensure_status_valid() != 0)
        return -1.0f;
    return g_system_status.load_15min_percent;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE PROCESOS
 * ============================================================================ */

int sysmon_get_total_processes(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.total_processes;
}

int sysmon_get_running_processes(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.running_processes;
}

int sysmon_get_sleeping_processes(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.sleeping_processes;
}

int sysmon_get_stopped_processes(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.stopped_processes;
}

int sysmon_get_zombie_processes(void)
{
    if (ensure_status_valid() != 0)
        return -1;
    return g_system_status.zombie_processes;
}

/* ============================================================================
 * GETTERS PARA INFORMACIÓN DE TIEMPO
 * ============================================================================ */

unsigned long sysmon_get_timestamp(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.timestamp;
}

unsigned long sysmon_get_uptime_seconds(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.uptime_seconds;
}

unsigned long sysmon_get_idle_time_seconds(void)
{
    if (ensure_status_valid() != 0)
        return 0;
    return (unsigned long)g_system_status.idle_time_seconds;
}

/* ============================================================================
 * FUNCIONES DE UTILIDAD
 * ============================================================================ */

const char *sysmon_get_error_string(int error_code)
{
    switch (error_code)
    {
    case 0:
        return "Success";
    case -1:
        return "General error";
    default:
        return "Unknown error";
    }
}

const char *sysmon_get_version(void)
{
    return sysmon_core_get_version();
}

int sysmon_is_function_available(const char *function_name)
{
    if (!function_name)
        return 0;

    // Todas las funciones básicas están disponibles en Linux
    return 1;
}

/* ============================================================================
 * FUNCIONES DE COMPATIBILIDAD CON LA API ANTERIOR
 * ============================================================================ */

// Mantener compatibilidad con las funciones existentes
int get_cpu_model(char *buffer, size_t size)
{
    return cpu_get_model(buffer, size);
}

double get_cpu_usage(void)
{
    return (double)cpu_get_usage_percent();
}