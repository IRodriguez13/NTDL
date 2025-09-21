/**
 * @file audio_interface.h
 * @brief Interfaz común para obtener información de audio en todas las plataformas
 */

#ifndef AUDIO_INTERFACE_H
#define AUDIO_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../../metrics.h"

    /* ============================================================================
     * INTERFAZ COMÚN DE AUDIO
     * ============================================================================ */

    /**
     * @brief Inicializa el módulo de audio
     * @return 0 en éxito, -1 en error
     */
    int audio_init(void);

    /**
     * @brief Limpia los recursos del módulo de audio
     */
    void audio_cleanup(void);

    /**
     * @brief Obtiene el número de dispositivos de audio disponibles
     * @return Número de dispositivos, 0 si no hay ninguno
     */
    int audio_get_device_count(void);

    /**
     * @brief Obtiene información de un dispositivo de audio específico
     * @param device_id ID del dispositivo (0 a device_count-1)
     * @param audio_info Puntero a estructura AudioDeviceInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int audio_get_device_info(int device_id, AudioDeviceInfo *audio_info);

    /**
     * @brief Obtiene información del dispositivo de audio por defecto
     * @param audio_info Puntero a estructura AudioDeviceInfo donde almacenar la información
     * @return 0 en éxito, -1 en error
     */
    int audio_get_default_device_info(AudioDeviceInfo *audio_info);

    /**
     * @brief Obtiene el volumen del dispositivo por defecto
     * @return Volumen en porcentaje (0-100), -1 si no disponible
     */
    int audio_get_master_volume_percent(void);

    /**
     * @brief Verifica si el audio está silenciado
     * @return 1 si está silenciado, 0 si no, -1 en error
     */
    int audio_is_muted(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_INTERFACE_H */