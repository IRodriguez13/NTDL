/**
 * @file network_macos.c
 * @brief Implementación de red para macOS usando SystemConfiguration y APIs del sistema
 */

#ifdef __APPLE__

#include "network_interface.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL network_initialized = FALSE;
static NetworkInterface network_list[MAX_NETWORK_INTERFACES];
static int network_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Convierte dirección MAC de sockaddr_dl a string
 */
static void mac_addr_to_string(struct sockaddr_dl *sdl, char *mac_str, size_t mac_str_size)
{
    if (sdl->sdl_alen == 6)
    {
        unsigned char *mac = (unsigned char *)LLADDR(sdl);
        snprintf(mac_str, mac_str_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else
    {
        strncpy(mac_str, "00:00:00:00:00:00", mac_str_size - 1);
        mac_str[mac_str_size - 1] = '\0';
    }
}

/**
 * @brief Determina el tipo de interfaz de red
 */
static void determine_interface_type(const char *if_name, char *type_str, size_t type_size, int *is_wireless)
{
    *is_wireless = 0;

    if (strncmp(if_name, "lo", 2) == 0)
    {
        strncpy(type_str, "Loopback", type_size - 1);
    }
    else if (strncmp(if_name, "en", 2) == 0)
    {
        // En macOS, en0 suele ser Ethernet, en1+ pueden ser WiFi
        if (strcmp(if_name, "en0") == 0)
        {
            strncpy(type_str, "Ethernet", type_size - 1);
        }
        else
        {
            strncpy(type_str, "WiFi", type_size - 1);
            *is_wireless = 1;
        }
    }
    else if (strncmp(if_name, "awdl", 4) == 0)
    {
        strncpy(type_str, "AWDL", type_size - 1);
        *is_wireless = 1;
    }
    else if (strncmp(if_name, "utun", 4) == 0)
    {
        strncpy(type_str, "VPN", type_size - 1);
    }
    else if (strncmp(if_name, "bridge", 6) == 0)
    {
        strncpy(type_str, "Bridge", type_size - 1);
    }
    else
    {
        strncpy(type_str, "Other", type_size - 1);
    }

    type_str[type_size - 1] = '\0';
}

/**
 * @brief Obtiene estadísticas de red para una interfaz
 */
static int get_interface_statistics(const char *if_name, NetworkInterface *net_if)
{
    // En macOS, las estadísticas se pueden obtener de /proc/net/dev equivalente
    // o usando APIs específicas. Por simplicidad, usamos valores por defecto.

    // Intentar obtener estadísticas básicas usando if_data
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) != 0)
    {
        return -1;
    }

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (strcmp(ifa->ifa_name, if_name) == 0 && ifa->ifa_addr->sa_family == AF_LINK)
        {
            struct if_data *if_data = (struct if_data *)ifa->ifa_data;
            if (if_data)
            {
                net_if->bytes_sent = if_data->ifi_obytes;
                net_if->bytes_received = if_data->ifi_ibytes;
                net_if->packets_sent = if_data->ifi_opackets;
                net_if->packets_received = if_data->ifi_ipackets;
                net_if->errors_in = if_data->ifi_ierrors;
                net_if->errors_out = if_data->ifi_oerrors;

                // Velocidad (si está disponible)
                if (if_data->ifi_baudrate > 0)
                {
                    net_if->speed_mbps = (float)(if_data->ifi_baudrate / 1000000.0);
                }

                freeifaddrs(ifap);
                return 0;
            }
        }
    }

    freeifaddrs(ifap);
    return -1;
}

/**
 * @brief Enumera interfaces de red usando getifaddrs
 */
static int enumerate_network_interfaces(void)
{
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) != 0)
    {
        return -1;
    }

    network_count = 0;

    // Primera pasada: contar interfaces únicas
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Buscar si ya tenemos esta interfaz
        int found = 0;
        for (int i = 0; i < network_count; i++)
        {
            if (strcmp(network_list[i].name, ifa->ifa_name) == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found && network_count < MAX_NETWORK_INTERFACES)
        {
            NetworkInterface *net_if = &network_list[network_count];
            memset(net_if, 0, sizeof(NetworkInterface));

            // Nombre de la interfaz
            strncpy(net_if->name, ifa->ifa_name, sizeof(net_if->name) - 1);

            // Descripción (igual al nombre en macOS)
            strncpy(net_if->description, ifa->ifa_name, sizeof(net_if->description) - 1);

            // Determinar tipo de interfaz
            determine_interface_type(ifa->ifa_name, net_if->type,
                                     sizeof(net_if->type), &net_if->is_wireless);

            // Estado de la interfaz
            net_if->is_up = (ifa->ifa_flags & IFF_UP) ? 1 : 0;

            network_count++;
        }
    }

    // Segunda pasada: llenar información detallada
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Buscar la interfaz en nuestra lista
        NetworkInterface *net_if = NULL;
        for (int i = 0; i < network_count; i++)
        {
            if (strcmp(network_list[i].name, ifa->ifa_name) == 0)
            {
                net_if = &network_list[i];
                break;
            }
        }

        if (net_if == NULL)
            continue;

        switch (ifa->ifa_addr->sa_family)
        {
        case AF_INET:
        {
            // Dirección IPv4
            struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &sin->sin_addr, net_if->ip_address,
                      sizeof(net_if->ip_address));

            // Máscara de subred
            if (ifa->ifa_netmask)
            {
                struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
                inet_ntop(AF_INET, &netmask->sin_addr, net_if->netmask,
                          sizeof(net_if->netmask));
            }
            break;
        }

        case AF_LINK:
        {
            // Dirección MAC
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            mac_addr_to_string(sdl, net_if->mac_address, sizeof(net_if->mac_address));
            break;
        }
        }
    }

    freeifaddrs(ifap);

    // Tercera pasada: obtener estadísticas
    for (int i = 0; i < network_count; i++)
    {
        get_interface_statistics(network_list[i].name, &network_list[i]);
    }

    return network_count > 0 ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int network_init(void)
{
    if (network_initialized)
    {
        return 0;
    }

    // Limpiar lista de interfaces
    memset(network_list, 0, sizeof(network_list));
    network_count = 0;

    // Enumerar interfaces de red
    if (enumerate_network_interfaces() != 0)
    {
        return -1;
    }

    network_initialized = TRUE;
    return 0;
}

void network_cleanup(void)
{
    network_initialized = FALSE;
    network_count = 0;
}

int network_get_count(void)
{
    return network_initialized ? network_count : -1;
}

int network_get_interface_info(int interface_id, NetworkInterface *info)
{
    if (!info || !network_initialized || interface_id < 0 || interface_id >= network_count)
    {
        return -1;
    }

    memcpy(info, &network_list[interface_id], sizeof(NetworkInterface));
    return 0;
}

int network_get_metrics(NetworkMetrics *metrics)
{
    if (!metrics || !network_initialized)
    {
        return -1;
    }

    // Actualizar estadísticas antes de devolver métricas
    for (int i = 0; i < network_count; i++)
    {
        get_interface_statistics(network_list[i].name, &network_list[i]);
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(NetworkMetrics));

    metrics->num_interfaces = network_count;

    // Copiar información de todas las interfaces
    for (int i = 0; i < network_count && i < MAX_NETWORK_INTERFACES; i++)
    {
        memcpy(&metrics->interfaces[i], &network_list[i], sizeof(NetworkInterface));
    }

    return 0;
}

int network_get_interface_name(int interface_id, char *buffer, size_t size)
{
    if (!buffer || size == 0 || !network_initialized ||
        interface_id < 0 || interface_id >= network_count)
    {
        return -1;
    }

    strncpy(buffer, network_list[interface_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float network_get_bytes_sent(int interface_id)
{
    if (!network_initialized || interface_id < 0 || interface_id >= network_count)
    {
        return -1.0f;
    }

    return (float)network_list[interface_id].bytes_sent;
}

float network_get_bytes_received(int interface_id)
{
    if (!network_initialized || interface_id < 0 || interface_id >= network_count)
    {
        return -1.0f;
    }

    return (float)network_list[interface_id].bytes_received;
}

int network_is_interface_up(int interface_id)
{
    if (!network_initialized || interface_id < 0 || interface_id >= network_count)
    {
        return -1;
    }

    return network_list[interface_id].is_up ? 1 : 0;
}

#endif /* __APPLE__ */