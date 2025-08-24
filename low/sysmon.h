#ifndef SYSMON_H
#define SYSMON_H

#include "metrics.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Inicializar el sistema de monitoreo
    int sysmon_init(void);

    // Obtener métricas del sistema
    int sysmon_get_metrics(SystemStatus *status);

    // Función para obtener el tamaño de SystemStatus
    size_t sysmon_get_system_status_size(void);

    // Obtener métricas específicas básicas
    float sysmon_get_cpu_usage(void);
    unsigned long sysmon_get_ram_total(void);
    unsigned long sysmon_get_ram_free(void);
    unsigned long sysmon_get_ram_used(void);
    float sysmon_get_ram_usage_percent(void);
    int sysmon_get_cpu_cores(void);
    const char *sysmon_get_cpu_model(void);
    const char *sysmon_get_os_name(void);

    // Obtener métricas detalladas de CPU
    float sysmon_get_cpu_temperature(void);
    float sysmon_get_cpu_frequency(void);
    int sysmon_get_cpu_threads(void);
    CoreInfo *sysmon_get_core_info(int core_id);
    int sysmon_get_num_cores(void);

    // Obtener métricas de GPU
    const char *sysmon_get_gpu_model(void);
    float sysmon_get_gpu_usage(void);
    float sysmon_get_gpu_temperature(void);
    float sysmon_get_gpu_memory_usage(void);

    // Obtener información de sensores
    SensorInfo *sysmon_get_sensor_info(int sensor_id);
    int sysmon_get_num_sensors(void);

    // Obtener información de la motherboard
    MotherboardInfo *sysmon_get_motherboard_info(void);

    // Obtener información de discos
    DiskInfo *sysmon_get_disk_info(int disk_id);
    int sysmon_get_num_disks(void);

    // Obtener información de red
    NetworkInfo *sysmon_get_network_info(int interface_id);
    int sysmon_get_num_network_interfaces(void);

    // Obtener información de energía
    float sysmon_get_power_consumption(void);
    float sysmon_get_battery_percent(void);
    int sysmon_get_ac_connected(void);

    // Obtener información de carga del sistema
    float sysmon_get_load_average_1min(void);
    float sysmon_get_load_average_5min(void);
    float sysmon_get_load_average_15min(void);

    // Obtener información de procesos
    int sysmon_get_total_processes(void);
    int sysmon_get_running_processes(void);
    int sysmon_get_sleeping_processes(void);
    int sysmon_get_stopped_processes(void);
    int sysmon_get_zombie_processes(void);

    // Obtener información del sistema
    const char *sysmon_get_os_version(void);
    const char *sysmon_get_kernel_version(void);
    const char *sysmon_get_hostname(void);
    unsigned long sysmon_get_timestamp(void);

    // Limpiar recursos
    void sysmon_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // SYSMON_H
