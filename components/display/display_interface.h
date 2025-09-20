/**
 * @file display_interface.h
 * @brief Interfaz común para obtener información de pantallas en todas las plataformas
 */

#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../metrics.h"

    /* ============================================================================
     * ESTRUCTURAS DE INFORMACIÓN DE PANTALLA
     * ============================================================================ */

    // DisplayInfo is defined in metrics.h - no need to redefine here

    /* ============================================================================
     * INTERFAZ COMÚN DE PANTALLAS
     * ============================================================================ */

    /**
     * @brief Inicializa el módulo de pantallas
     * @return 0 en éxito, -1 en error
     */
    int display_init(void);

    /**
     * @brief Limpia los recursos del módulo de pantallas
     */
    void display_cleanup(void);

    /**
     * @brief Obtiene el número de pantallas disponibles
     * @return Número de pantallas, 0 si no hay ninguna
     */
    int display_get_count(void);

    /**
     * @brief Obtiene información de una pantalla específica
     * @param display_id ID de la pantalla (0 a display_count-1)
     * @param display_info Puntero a estructura DisplayInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int display_get_info(int display_id, DisplayInfo *display_info);

    /**
     * @brief Obtiene información de la pantalla principal
     * @param display_info Puntero a estructura DisplayInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int display_get_primary_info(DisplayInfo *display_info);

    /**
     * @brief Obtiene la resolución total del escritorio
     * @param total_width Puntero donde almacenar el ancho total
     * @param total_height Puntero donde almacenar el alto total
     * @return 0 en éxito, -1 en error
     */
    int display_get_desktop_resolution(int *total_width, int *total_height);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_INTERFACE_H */