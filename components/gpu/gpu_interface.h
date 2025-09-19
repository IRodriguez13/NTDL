/**
 * @file gpu_interface.h
 * @brief Interfaz común para obtener información de GPU en todas las plataformas
 */

#ifndef GPU_INTERFACE_H
#define GPU_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ COMÚN DE GPU
 * ============================================================================ */

/**
 * @brief Inicializa el módulo de GPU
 * @return 0 en éxito, -1 en error
 */
int gpu_init(void);

/**
 * @brief Limpia los recursos del módulo de GPU
 */
void gpu_cleanup(void);

/**
 * @brief Obtiene el número de GPUs disponibles
 * @return Número de GPUs, 0 si no hay ninguna
 */
int gpu_get_count(void);

/**
 * @brief Obtiene información de una GPU específica
 * @param gpu_id ID de la GPU (0 a gpu_count-1)
 * @param metrics Puntero a estructura GpuMetrics donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int gpu_get_metrics(int gpu_id, GpuMetrics *metrics);

/**
 * @brief Obtiene información de la GPU principal (primera disponible)
 * @param metrics Puntero a estructura GpuMetrics donde almacenar la información
 * @return 0 en éxito, -1 en error
 */
int gpu_get_primary_metrics(GpuMetrics *metrics);

/**
 * @brief Obtiene el nombre/modelo de una GPU
 * @param gpu_id ID de la GPU
 * @param buffer Buffer donde almacenar el nombre
 * @param size Tamaño del buffer
 * @return 0 en éxito, -1 en error
 */
int gpu_get_name(int gpu_id, char *buffer, size_t size);

/**
 * @brief Obtiene el uso actual de una GPU
 * @param gpu_id ID de la GPU
 * @return Porcentaje de uso (0.0-100.0), -1.0 en error
 */
float gpu_get_usage_percent(int gpu_id);

/**
 * @brief Obtiene la temperatura de una GPU
 * @param gpu_id ID de la GPU
 * @return Temperatura en °C, -1.0 si no disponible
 */
float gpu_get_temperature(int gpu_id);

/**
 * @brief Obtiene el uso de memoria de una GPU
 * @param gpu_id ID de la GPU
 * @return Porcentaje de uso de memoria (0.0-100.0), -1.0 en error
 */
float gpu_get_memory_usage_percent(int gpu_id);

/**
 * @brief Obtiene la memoria total de una GPU
 * @param gpu_id ID de la GPU
 * @return Memoria total en MB, 0 si no disponible
 */
uint64_t gpu_get_memory_total_mb(int gpu_id);

/**
 * @brief Obtiene la memoria usada de una GPU
 * @param gpu_id ID de la GPU
 * @return Memoria usada en MB, 0 si no disponible
 */
uint64_t gpu_get_memory_used_mb(int gpu_id);

#ifdef __cplusplus
}
#endif

#endif /* GPU_INTERFACE_H */