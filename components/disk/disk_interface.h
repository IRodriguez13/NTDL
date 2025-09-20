/**
 * @file disk_interface.h
 * @brief Interfaz común para obtener información de discos en todas las plataformas
 */

#ifndef DISK_INTERFACE_H
#define DISK_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../metrics.h"

    /* ============================================================================
     * INTERFAZ COMÚN DE DISCOS
     * ============================================================================ */

    /**
     * @brief Inicializa el módulo de discos
     * @return 0 en éxito, -1 en error
     */
    int disk_init(void);

    /**
     * @brief Limpia los recursos del módulo de discos
     */
    void disk_cleanup(void);

    /**
     * @brief Obtiene el número de discos disponibles
     * @return Número de discos, 0 si no hay ninguno
     */
    int disk_get_count(void);

    /**
     * @brief Obtiene información de un disco específico
     * @param disk_id ID del disco (0 a disk_count-1)
     * @param disk_info Puntero a estructura DiskInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int disk_get_info(int disk_id, DiskInfo *disk_info);

    /**
     * @brief Obtiene información SMART de un disco
     * @param device_path Ruta del dispositivo (ej: /dev/sda)
     * @param disk_info Puntero a estructura DiskInfo donde almacenar la información SMART
     * @return 0 en éxito, -1 en error
     */
    int disk_get_smart_info(const char *device_path, DiskInfo *disk_info);

    /**
     * @brief Obtiene la temperatura de un disco
     * @param device_path Ruta del dispositivo
     * @return Temperatura en °C, -1.0 si no disponible
     */
    float disk_get_temperature(const char *device_path);

    /**
     * @brief Obtiene las horas de funcionamiento de un disco
     * @param device_path Ruta del dispositivo
     * @return Horas de funcionamiento, -1 si no disponible
     */
    int disk_get_power_on_hours(const char *device_path);

    /**
     * @brief Verifica si un disco es SSD
     * @param device_path Ruta del dispositivo
     * @return 1 si es SSD, 0 si es HDD, -1 si no se puede determinar
     */
    int disk_is_ssd(const char *device_path);

    /**
     * @brief Obtiene estadísticas de I/O de un disco
     * @param device_path Ruta del dispositivo
     * @param read_bytes Puntero donde almacenar bytes leídos
     * @param write_bytes Puntero donde almacenar bytes escritos
     * @param read_ops Puntero donde almacenar operaciones de lectura
     * @param write_ops Puntero donde almacenar operaciones de escritura
     * @return 0 en éxito, -1 en error
     */
    int disk_get_io_stats(const char *device_path, uint64_t *read_bytes, uint64_t *write_bytes,
                          uint32_t *read_ops, uint32_t *write_ops);

#ifdef __cplusplus
}
#endif

#endif /* DISK_INTERFACE_H */