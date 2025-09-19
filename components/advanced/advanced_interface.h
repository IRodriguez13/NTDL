/**
 * @file advanced_interface.h
 * @brief Interfaz para componentes avanzados (sensores, memoria, red)
 */

#ifndef ADVANCED_INTERFACE_H
#define ADVANCED_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ DE SENSORES AVANZADOS
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de sensores avanzados
 * @return 0 en éxito, -1 en error
 */
int advanced_sensors_init(void);

/**
 * @brief Limpia los recursos del módulo de sensores avanzados
 */
void advanced_sensors_cleanup(void);

/**
 * @brief Obtiene información completa de sensores avanzados
 * @param sensors_info Puntero a estructura AdvancedSensorsInfo donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int advanced_sensors_get_info(AdvancedSensorsInfo *sensors_info);

/* ============================================================================
 * INTERFAZ DE MEMORIA AVANZADA
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de memoria avanzada
 * @return 0 en éxito, -1 en error
 */
int advanced_memory_init(void);

/**
 * @brief Limpia los recursos del módulo de memoria avanzada
 */
void advanced_memory_cleanup(void);

/**
 * @brief Obtiene información avanzada de memoria
 * @param memory_info Puntero a estructura MemoryAdvancedInfo donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int advanced_memory_get_info(MemoryAdvancedInfo *memory_info);

/* ============================================================================
 * INTERFAZ DE RED AVANZADA
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de red avanzada
 * @return 0 en éxito, -1 en error
 */
int advanced_network_init(void);

/**
 * @brief Limpia los recursos del módulo de red avanzada
 */
void advanced_network_cleanup(void);

/**
 * @brief Obtiene información avanzada de red
 * @param network_info Puntero a estructura NetworkAdvancedInfo donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int advanced_network_get_info(NetworkAdvancedInfo *network_info);

#ifdef __cplusplus
}
#endif

#endif /* ADVANCED_INTERFACE_H */