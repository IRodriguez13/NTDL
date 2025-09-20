/**
 * @file memory_interface.h
 * @brief Interfaz común para obtener información de memoria en todas las plataformas
 */

#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../metrics.h"

    /* ============================================================================
     * INTERFAZ COMÚN DE MEMORIA
     * ============================================================================ */

    /**
     * @brief Inicializa el módulo de memoria
     * @return 0 en éxito, -1 en error
     */
    int memory_init(void);

    /**
     * @brief Limpia los recursos del módulo de memoria
     */
    void memory_cleanup(void);

    /**
     * @brief Obtiene información detallada de la memoria RAM
     * @param ram_info Puntero a estructura DetailedRamInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int memory_get_ram_info(DetailedRamInfo *ram_info);

    /**
     * @brief Obtiene información del swap
     * @param swap_info Puntero a estructura SwapInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int memory_get_swap_info(SwapInfo *swap_info);

    /**
     * @brief Obtiene la memoria RAM total en KB
     * @return Memoria total en KB, 0 en error
     */
    uint64_t memory_get_ram_total_kb(void);

    /**
     * @brief Obtiene la memoria RAM libre en KB
     * @return Memoria libre en KB, 0 en error
     */
    uint64_t memory_get_ram_free_kb(void);

    /**
     * @brief Obtiene la memoria RAM disponible en KB
     * @return Memoria disponible en KB, 0 en error
     */
    uint64_t memory_get_ram_available_kb(void);

    /**
     * @brief Obtiene la memoria RAM usada en KB
     * @return Memoria usada en KB, 0 en error
     */
    uint64_t memory_get_ram_used_kb(void);

    /**
     * @brief Obtiene el porcentaje de uso de RAM
     * @return Porcentaje de uso (0.0-100.0), -1.0 en error
     */
    float memory_get_ram_usage_percent(void);

    /**
     * @brief Obtiene el porcentaje de uso de swap
     * @return Porcentaje de uso (0.0-100.0), -1.0 en error
     */
    float memory_get_swap_usage_percent(void);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_INTERFACE_H */