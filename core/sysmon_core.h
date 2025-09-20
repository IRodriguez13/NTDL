/**
 * @file sysmon_core.h
 * @brief Núcleo central del sistema de monitoreo - Unifica todos los componentes
 */

#ifndef SYSMON_CORE_H
#define SYSMON_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../metrics.h"

    /* ============================================================================
     * INTERFAZ CENTRAL DEL SISTEMA
     * ============================================================================ */

    /**
     * @brief Inicializa el sistema completo de monitoreo
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_init(void);

    /**
     * @brief Limpia todos los recursos del sistema de monitoreo
     */
    void sysmon_core_cleanup(void);

    /**
     * @brief Recolecta todas las métricas del sistema
     * @param status Puntero a estructura SystemStatus donde almacenar todas las métricas
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_collect_all_metrics(SystemStatus *status);

    /**
     * @brief Actualiza solo las métricas que cambian frecuentemente (CPU, memoria, red)
     * @param status Puntero a estructura SystemStatus donde actualizar las métricas
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_update_dynamic_metrics(SystemStatus *status);

    /**
     * @brief Obtiene información básica del sistema (SO, hostname, etc.)
     * @param status Puntero a estructura SystemStatus donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_system_info(SystemStatus *status);

    /**
     * @brief Obtiene métricas de CPU
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de CPU
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_cpu_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de memoria
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de memoria
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_memory_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de GPU
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de GPU
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_gpu_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de discos
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de discos
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_disk_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de red
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de red
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_network_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de sensores
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de sensores
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_sensor_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de pantallas
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de pantallas
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_display_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de batería
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de batería
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_battery_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas de audio
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas de audio
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_audio_metrics(SystemStatus *status);

    /**
     * @brief Obtiene métricas avanzadas (sensores, memoria, red)
     * @param status Puntero a estructura SystemStatus donde almacenar las métricas avanzadas
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_advanced_metrics(SystemStatus *status);

    /**
     * @brief Verifica si el sistema está inicializado
     * @return 1 si está inicializado, 0 si no
     */
    int sysmon_core_is_initialized(void);

    /**
     * @brief Obtiene la versión del sistema de monitoreo
     * @return String con la versión
     */
    const char *sysmon_core_get_version(void);

    /**
     * @brief Obtiene estadísticas de rendimiento del sistema de monitoreo
     * @param collection_time_ms Tiempo de la última recolección en ms
     * @param total_collections Número total de recolecciones realizadas
     * @return 0 en éxito, -1 en error
     */
    int sysmon_core_get_performance_stats(double *collection_time_ms, uint64_t *total_collections);

#ifdef __cplusplus
}
#endif

#endif /* SYSMON_CORE_H */