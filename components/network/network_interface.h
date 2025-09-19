/**
 * @file network_interface.h
 * @brief Interfaz común para obtener información de red en todas las
 * plataformas
 */

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../metrics.h"

/* ============================================================================
 * INTERFAZ COMÚN DE RED
 * ============================================================================
 */

/**
 * @brief Inicializa el módulo de red
 * @return 0 en éxito, -1 en error
 */
int network_init(void);

/**
 * @brief Limpia los recursos del módulo de red
 */
void network_cleanup(void);

/**
 * @brief Obtiene el número de interfaces de red disponibles
 * @return Número de interfaces, 0 si no hay ninguna
 */
int network_get_interface_count(void);

/**
 * @brief Obtiene información de una interfaz de red específica
 * @param interface_id ID de la interfaz (0 a interface_count-1)
 * @param net_info Puntero a estructura NetworkInfo donde almacenar la
 * información
 * @return 0 en éxito, -1 en error
 */
int network_get_interface_info(int interface_id, NetworkInfo *net_info);

/**
 * @brief Obtiene estadísticas de una interfaz de red
 * @param interface_name Nombre de la interfaz (ej: eth0, wlan0)
 * @param net_info Puntero a estructura NetworkInfo donde almacenar las
 * estadísticas
 * @return 0 en éxito, -1 en error
 */
int network_get_interface_stats(const char *interface_name,
                                NetworkInfo *net_info);

/**
 * @brief Verifica si una interfaz está activa
 * @param interface_name Nombre de la interfaz
 * @return 1 si está activa, 0 si no, -1 en error
 */
int network_is_interface_up(const char *interface_name);

/**
 * @brief Verifica si una interfaz es inalámbrica
 * @param interface_name Nombre de la interfaz
 * @return 1 si es inalámbrica, 0 si no, -1 en error
 */
int network_is_interface_wireless(const char *interface_name);

/**
 * @brief Obtiene la dirección IP de una interfaz
 * @param interface_name Nombre de la interfaz
 * @param ip_buffer Buffer donde almacenar la IP
 * @param buffer_size Tamaño del buffer
 * @return 0 en éxito, -1 en error
 */
int network_get_interface_ip(const char *interface_name, char *ip_buffer,
                             size_t buffer_size);

/**
 * @brief Obtiene la dirección MAC de una interfaz
 * @param interface_name Nombre de la interfaz
 * @param mac_buffer Buffer donde almacenar la MAC
 * @param buffer_size Tamaño del buffer
 * @return 0 en éxito, -1 en error
 */
int network_get_interface_mac(const char *interface_name, char *mac_buffer,
                              size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_INTERFACE_H */