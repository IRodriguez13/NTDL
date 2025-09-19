/**
 * @file sensors_interface.h
 * @brief Interfaz común para obtener información de sensores en todas las plataformas
 */

#ifndef SENSORS_INTERFACE_H
#define SENSORS_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ COMÚN DE SENSORES
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de sensores
 * @return 0 en éxito, -1 en error
 */
int sensors_init(void);

/**
 * @brief Limpia los recursos del módulo de sensores
 */
void sensors_cleanup(void);

/**
 * @brief Obtiene el número de sensores disponibles
 * @return Número de sensores, 0 si no hay ninguno
 */
int sensors_get_count(void);

/**
 * @brief Obtiene información de un sensor específico
 * @param sensor_id ID del sensor (0 a sensor_count-1)
 * @param sensor_info Puntero a estructura SensorInfo donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int sensors_get_info(int sensor_id, SensorInfo *sensor_info);

/**
 * @brief Obtiene todos los sensores de temperatura
 * @param temp_sensors Array donde almacenar los sensores de temperatura
 * @param max_sensors Tamaño máximo del array
 * @return Número de sensores de temperatura encontrados, -1 en error
 */
int sensors_get_temperature_sensors(SensorInfo *temp_sensors, int max_sensors);

/**
 * @brief Obtiene todos los sensores de ventiladores
 * @param fan_sensors Array donde almacenar los sensores de ventiladores
 * @param max_sensors Tamaño máximo del array
 * @return Número de sensores de ventiladores encontrados, -1 en error
 */
int sensors_get_fan_sensors(SensorInfo *fan_sensors, int max_sensors);

/**
 * @brief Obtiene todos los sensores de voltaje
 * @param voltage_sensors Array donde almacenar los sensores de voltaje
 * @param max_sensors Tamaño máximo del array
 * @return Número de sensores de voltaje encontrados, -1 en error
 */
int sensors_get_voltage_sensors(SensorInfo *voltage_sensors, int max_sensors);

/**
 * @brief Busca un sensor por nombre
 * @param sensor_name Nombre del sensor a buscar
 * @param sensor_info Puntero donde almacenar la información del sensor
 * @return 0 en éxito, -1 si no se encuentra
 */
int sensors_find_by_name(const char *sensor_name, SensorInfo *sensor_info);

/**
 * @brief Obtiene la temperatura promedio del sistema
 * @return Temperatura promedio en °C, -1.0 si no disponible
 */
float sensors_get_average_temperature(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSORS_INTERFACE_H */