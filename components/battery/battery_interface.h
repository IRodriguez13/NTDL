/**
 * @file battery_interface.h
 * @brief Interfaz común para obtener información de batería en todas las plataformas
 */

#ifndef BATTERY_INTERFACE_H
#define BATTERY_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ COMÚN DE BATERÍA
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de batería
 * @return 0 en éxito, -1 en error
 */
int battery_init(void);

/**
 * @brief Limpia los recursos del módulo de batería
 */
void battery_cleanup(void);

/**
 * @brief Obtiene información completa de la batería
 * @param battery_info Puntero a estructura BatteryInfo donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int battery_get_info(BatteryInfo *battery_info);

/**
 * @brief Verifica si hay batería presente
 * @return 1 si hay batería, 0 si no, -1 en error
 */
int battery_is_present(void);

/**
 * @brief Obtiene el porcentaje de carga de la batería
 * @return Porcentaje (0.0-100.0), -1.0 si no disponible
 */
float battery_get_charge_percent(void);

/**
 * @brief Verifica si la batería está cargando
 * @return 1 si está cargando, 0 si no, -1 en error
 */
int battery_is_charging(void);

/**
 * @brief Obtiene el tiempo restante de batería
 * @return Tiempo en minutos, -1 si no disponible
 */
int battery_get_time_remaining_minutes(void);

/**
 * @brief Obtiene el nivel de desgaste de la batería
 * @return Porcentaje de desgaste (0.0-100.0), -1.0 si no disponible
 */
float battery_get_wear_level_percent(void);

/**
 * @brief Obtiene el número de ciclos de carga
 * @return Número de ciclos, -1 si no disponible
 */
int battery_get_cycle_count(void);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_INTERFACE_H */