/**
 * @file extern_api.h
 * @brief API Externa Final de la Biblioteca SysMon
 * @version 1.0.0
 * @date 2024
 *
 * Esta es la API pública final para consumir métricas del sistema
 * desde aplicaciones externas usando ctypes, cffi u otras bibliotecas FFI.
 *
 * Compatible con: Linux, Windows, macOS (XNU)
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "metrics.h"
#include "common/common.h"

    /* ============================================================================
     * FUNCIONES DE INICIALIZACIÓN Y LIMPIEZA
     * ============================================================================ */

    /**
     * @brief Inicializa la biblioteca sysmon
     * @return 0 en éxito, -1 en error
     *
     * Esta función debe ser llamada antes de usar cualquier otra función
     * de la biblioteca. Configura las estructuras internas necesarias
     * para recolectar métricas del sistema.
     */
    int sysmon_init(void);

    /**
     * @brief Limpia los recursos de la biblioteca sysmon
     *
     * Libera memoria y recursos utilizados por la biblioteca.
     * Debe ser llamada al finalizar el uso de la biblioteca.
     */
    void sysmon_cleanup(void);

    /* ============================================================================
     * FUNCIÓN PRINCIPAL DE RECOLECCIÓN DE MÉTRICAS
     * ============================================================================ */

    /**
     * @brief Recolecta todas las métricas del sistema
     * @param status Puntero a estructura SystemStatus donde se almacenarán las métricas
     * @return 0 en éxito, -1 en error
     *
     * Esta es la función principal que recolecta toda la información
     * del sistema: CPU, RAM, discos, red, GPU, sensores, etc.
     *
     * Ejemplo de uso:
     * @code
     * SystemStatus status;
     * if (sysmon_collect_metrics(&status) == 0) {
     *     printf("CPU Usage: %.2f%%\n", status.cpu_usage_percent);
     *     printf("RAM Usage: %.2f%%\n", status.ram_usage_percent);
     * }
     * @endcode
     */
    int sysmon_collect_metrics(SystemStatus *status);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN BÁSICA DEL SISTEMA
     * ============================================================================ */

    /**
     * @brief Obtiene el nombre del sistema operativo
     * @return Puntero a string con el nombre del OS
     */
    const char *sysmon_get_os_name(void);

    /**
     * @brief Obtiene la versión del sistema operativo
     * @return Puntero a string con la versión del OS
     */
    const char *sysmon_get_os_version(void);

    /**
     * @brief Obtiene la versión del kernel
     * @return Puntero a string con la versión del kernel
     */
    const char *sysmon_get_kernel_version(void);

    /**
     * @brief Obtiene el hostname del sistema
     * @return Puntero a string con el hostname
     */
    const char *sysmon_get_hostname(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE CPU
     * ============================================================================ */

    /**
     * @brief Obtiene el modelo de CPU
     * @return Puntero a string con el modelo de CPU
     */
    const char *sysmon_get_cpu_model(void);

    /**
     * @brief Obtiene el número de núcleos físicos
     * @return Número de núcleos CPU
     */
    int sysmon_get_cpu_cores(void);

    /**
     * @brief Obtiene el número de threads lógicos
     * @return Número de threads CPU
     */
    int sysmon_get_cpu_threads(void);

    /**
     * @brief Obtiene el porcentaje de uso de CPU
     * @return Porcentaje de uso (0.0 - 100.0)
     */
    float sysmon_get_cpu_usage_percent(void);

    /**
     * @brief Obtiene la temperatura promedio de la CPU
     * @return Temperatura en grados Celsius (-1.0 si no disponible)
     */
    float sysmon_get_cpu_temperature(void);

    /**
     * @brief Obtiene la frecuencia promedio de la CPU
     * @return Frecuencia en MHz (-1.0 si no disponible)
     */
    float sysmon_get_cpu_frequency(void);

    /**
     * @brief Obtiene información de un núcleo específico
     * @param core_id ID del núcleo (0 a num_cores-1)
     * @return Puntero a estructura CoreInfo, NULL si no válido
     */
    const CoreInfo *sysmon_get_core_info(int core_id);

    /**
     * @brief Obtiene el número total de núcleos
     * @return Número total de núcleos
     */
    int sysmon_get_num_cores(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE MEMORIA RAM
     * ============================================================================ */

    /**
     * @brief Obtiene la memoria RAM total en KB
     * @return Memoria total en KB
     */
    unsigned long sysmon_get_ram_total_kb(void);

    /**
     * @brief Obtiene la memoria RAM libre en KB
     * @return Memoria libre en KB
     */
    unsigned long sysmon_get_ram_free_kb(void);

    /**
     * @brief Obtiene la memoria RAM usada en KB
     * @return Memoria usada en KB
     */
    unsigned long sysmon_get_ram_used_kb(void);

    /**
     * @brief Obtiene el porcentaje de uso de RAM
     * @return Porcentaje de uso (0.0 - 100.0)
     */
    float sysmon_get_ram_usage_percent(void);

    /**
     * @brief Obtiene información detallada de RAM
     * @return Puntero a estructura DetailedRamInfo
     */
    const DetailedRamInfo *sysmon_get_detailed_ram_info(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE GPU
     * ============================================================================ */

    /**
     * @brief Obtiene el modelo de GPU
     * @return Puntero a string con el modelo de GPU
     */
    const char *sysmon_get_gpu_model(void);

    /**
     * @brief Obtiene el driver de GPU
     * @return Puntero a string con el driver de GPU
     */
    const char *sysmon_get_gpu_driver(void);

    /**
     * @brief Obtiene el fabricante de GPU
     * @return Puntero a string con el fabricante de GPU
     */
    const char *sysmon_get_gpu_vendor(void);

    /**
     * @brief Obtiene la memoria total de GPU en MB
     * @return Memoria total en MB
     */
    unsigned long sysmon_get_gpu_memory_total_mb(void);

    /**
     * @brief Obtiene la memoria usada de GPU en MB
     * @return Memoria usada en MB
     */
    unsigned long sysmon_get_gpu_memory_used_mb(void);

    /**
     * @brief Obtiene la memoria libre de GPU en MB
     * @return Memoria libre en MB
     */
    unsigned long sysmon_get_gpu_memory_free_mb(void);

    /**
     * @brief Obtiene el porcentaje de uso de GPU
     * @return Porcentaje de uso (0.0 - 100.0)
     */
    float sysmon_get_gpu_usage_percent(void);

    /**
     * @brief Obtiene la temperatura de GPU
     * @return Temperatura en grados Celsius (-1.0 si no disponible)
     */
    float sysmon_get_gpu_temperature(void);

    /**
     * @brief Obtiene el porcentaje de uso de memoria de GPU
     * @return Porcentaje de uso de memoria (0.0 - 100.0)
     */
    float sysmon_get_gpu_memory_usage_percent(void);

    /**
     * @brief Obtiene la velocidad del ventilador de GPU en RPM
     * @return Velocidad en RPM (-1 si no disponible)
     */
    int sysmon_get_gpu_fan_speed(void);

    /**
     * @brief Obtiene el consumo de energía de GPU en Watts
     * @return Consumo en Watts (-1 si no disponible)
     */
    int sysmon_get_gpu_power_usage_w(void);

    /**
     * @brief Obtiene la frecuencia de clock de GPU en MHz
     * @return Frecuencia en MHz (-1 si no disponible)
     */
    int sysmon_get_gpu_clock_mhz(void);

    /**
     * @brief Obtiene la frecuencia de memoria de GPU en MHz
     * @return Frecuencia en MHz (-1 si no disponible)
     */
    int sysmon_get_gpu_memory_clock_mhz(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE DISCOS
     * ============================================================================ */

    /**
     * @brief Obtiene información de un disco específico
     * @param disk_id ID del disco (0 a num_disks-1)
     * @return Puntero a estructura DiskInfo, NULL si no válido
     */
    const DiskInfo *sysmon_get_disk_info(int disk_id);

    /**
     * @brief Obtiene el número total de discos
     * @return Número total de discos
     */
    int sysmon_get_num_disks(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE RED
     * ============================================================================ */

    /**
     * @brief Obtiene información de una interfaz de red específica
     * @param interface_id ID de la interfaz (0 a num_network_interfaces-1)
     * @return Puntero a estructura NetworkInfo, NULL si no válido
     */
    const NetworkInfo *sysmon_get_network_info(int interface_id);

    /**
     * @brief Obtiene el número total de interfaces de red
     * @return Número total de interfaces
     */
    int sysmon_get_num_network_interfaces(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE SENSORES
     * ============================================================================ */

    /**
     * @brief Obtiene información de un sensor específico
     * @param sensor_id ID del sensor (0 a num_sensors-1)
     * @return Puntero a estructura SensorInfo, NULL si no válido
     */
    const SensorInfo *sysmon_get_sensor_info(int sensor_id);

    /**
     * @brief Obtiene el número total de sensores
     * @return Número total de sensores
     */
    int sysmon_get_num_sensors(void);

    /**
     * @brief Obtiene el número total de GPUs
     * @return Número total de GPUs
     */
    int sysmon_get_num_gpus(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE MOTHERBOARD
     * ============================================================================ */

    /**
     * @brief Obtiene información de la motherboard
     * @return Puntero a estructura MotherboardInfo
     */
    const MotherboardInfo *sysmon_get_motherboard_info(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE ENERGÍA
     * ============================================================================ */

    /**
     * @brief Obtiene el consumo de energía del sistema en Watts
     * @return Consumo en Watts (-1.0 si no disponible)
     */
    float sysmon_get_power_consumption(void);

    /**
     * @brief Obtiene el porcentaje de batería
     * @return Porcentaje de batería (0.0 - 100.0, -1.0 si no disponible)
     */
    float sysmon_get_battery_percent(void);

    /**
     * @brief Verifica si el sistema está conectado a AC
     * @return 1 si conectado a AC, 0 si no, -1 si no disponible
     */
    int sysmon_get_ac_connected(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE CARGA DEL SISTEMA
     * ============================================================================ */

    /**
     * @brief Obtiene la carga promedio de 1 minuto
     * @return Carga promedio de 1 minuto
     */
    float sysmon_get_load_average_1min(void);

    /**
     * @brief Obtiene la carga promedio de 5 minutos
     * @return Carga promedio de 5 minutos
     */
    float sysmon_get_load_average_5min(void);

    /**
     * @brief Obtiene la carga promedio de 15 minutos
     * @return Carga promedio de 15 minutos
     */
    float sysmon_get_load_average_15min(void);

    /**
     * @brief Obtiene la carga promedio de 1 minuto como porcentaje
     * @return Carga promedio de 1 minuto como porcentaje
     */
    float sysmon_get_load_1min_percent(void);

    /**
     * @brief Obtiene la carga promedio de 5 minutos como porcentaje
     * @return Carga promedio de 5 minutos como porcentaje
     */
    float sysmon_get_load_5min_percent(void);

    /**
     * @brief Obtiene la carga promedio de 15 minutos como porcentaje
     * @return Carga promedio de 15 minutos como porcentaje
     */
    float sysmon_get_load_15min_percent(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE PROCESOS
     * ============================================================================ */

    /**
     * @brief Obtiene el número total de procesos
     * @return Número total de procesos
     */
    int sysmon_get_total_processes(void);

    /**
     * @brief Obtiene el número de procesos ejecutándose
     * @return Número de procesos ejecutándose
     */
    int sysmon_get_running_processes(void);

    /**
     * @brief Obtiene el número de procesos durmiendo
     * @return Número de procesos durmiendo
     */
    int sysmon_get_sleeping_processes(void);

    /**
     * @brief Obtiene el número de procesos detenidos
     * @return Número de procesos detenidos
     */
    int sysmon_get_stopped_processes(void);

    /**
     * @brief Obtiene el número de procesos zombie
     * @return Número de procesos zombie
     */
    int sysmon_get_zombie_processes(void);

    /* ============================================================================
     * GETTERS PARA INFORMACIÓN DE TIEMPO
     * ============================================================================ */

    /**
     * @brief Obtiene el timestamp de la última recolección
     * @return Timestamp en segundos desde epoch
     */
    unsigned long sysmon_get_timestamp(void);

    /**
     * @brief Obtiene el tiempo de actividad del sistema en segundos
     * @return Tiempo de actividad en segundos
     */
    unsigned long sysmon_get_uptime_seconds(void);

    /**
     * @brief Obtiene el tiempo idle del sistema en segundos
     * @return Tiempo idle en segundos (-1 si no disponible)
     */
    unsigned long sysmon_get_idle_time_seconds(void);

    /* ============================================================================
     * FUNCIONES DE UTILIDAD
     * ============================================================================ */

    /**
     * @brief Obtiene una descripción de error
     * @param error_code Código de error
     * @return Puntero a string con la descripción del error
     */
    const char *sysmon_get_error_string(int error_code);

    /**
     * @brief Obtiene la versión de la biblioteca
     * @return Puntero a string con la versión
     */
    const char *sysmon_get_version(void);

    /**
     * @brief Verifica si una función está disponible en la plataforma actual
     * @param function_name Nombre de la función a verificar
     * @return 1 si disponible, 0 si no disponible
     */
    int sysmon_is_function_available(const char *function_name);

#ifdef __cplusplus
}
#endif

