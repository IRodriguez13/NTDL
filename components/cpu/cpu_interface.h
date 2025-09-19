/**
 * @file cpu_interface.h
 * @brief Interfaz común para obtener información de CPU en todas las plataformas
 */

#ifndef CPU_INTERFACE_H
#define CPU_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ COMÚN DE CPU
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de CPU
 * @return 0 en éxito, -1 en error
 */
int cpu_init(void);

/**
 * @brief Limpia los recursos del módulo de CPU
 */
void cpu_cleanup(void);

/**
 * @brief Obtiene información completa del CPU
 * @param metrics Puntero a estructura CpuMetrics donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int cpu_get_metrics(CpuMetrics *metrics);

/**
 * @brief Obtiene el modelo del CPU
 * @param buffer Buffer donde almacenar el modelo
 * @param size Tamaño del buffer
 * @return 0 en éxito, -1 en error
 */
int cpu_get_model(char *buffer, size_t size);

/**
 * @brief Obtiene el uso actual del CPU
 * @return Porcentaje de uso (0.0-100.0), -1.0 en error
 */
float cpu_get_usage_percent(void);

/**
 * @brief Obtiene la temperatura promedio del CPU
 * @return Temperatura en °C, -1.0 si no disponible
 */
float cpu_get_temperature(void);

/**
 * @brief Obtiene la frecuencia actual del CPU
 * @return Frecuencia en MHz, -1.0 si no disponible
 */
float cpu_get_frequency_mhz(void);

/**
 * @brief Obtiene información de un núcleo específico
 * @param core_id ID del núcleo (0 a num_cores-1)
 * @param core_info Puntero donde almacenar la información del núcleo
 * @return 0 en éxito, -1 en error
 */
int cpu_get_core_info(int core_id, CoreInfo *core_info);

/**
 * @brief Obtiene el número de núcleos físicos
 * @return Número de núcleos físicos
 */
int cpu_get_physical_cores(void);

/**
 * @brief Obtiene el número de núcleos lógicos
 * @return Número de núcleos lógicos
 */
int cpu_get_logical_cores(void);

#ifdef __cplusplus
}
#endif

#endif /* CPU_INTERFACE_H */