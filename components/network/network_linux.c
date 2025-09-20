/**
 * @file network_linux.c
 * @brief Implementación de red para Linux
 */

#include "network_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int interface_count = 0;
static NetworkInfo detected_interfaces[MAX_NETWORK_INTERFACES];

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int get_interface_stats_from_proc(const char *interface_name, NetworkInfo *net_info)
{
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f)
        return -1;

    char line[512];
    // Saltar las dos primeras líneas (headers)
    if (!fgets(line, sizeof(line), f))
    {
        fclose(f);
        return -1;
    }
    if (!fgets(line, sizeof(line), f))
    {
        fclose(f);
        return -1;
    }

    while (fgets(line, sizeof(line), f))
    {
        char iface[64];
        uint64_t rx_bytes, rx_packets, rx_errs, rx_drop;
        uint64_t tx_bytes, tx_packets, tx_errs, tx_drop;

        if (sscanf(line, "%63[^:]: %lu %lu %lu %lu %*u %*u %*u %*u %lu %lu %lu %lu",
                   iface, &rx_bytes, &rx_packets, &rx_errs, &rx_drop,
                   &tx_bytes, &tx_packets, &tx_errs, &tx_drop) == 9)
        {

            if (strcmp(iface, interface_name) == 0)
            {
                net_info->bytes_recv = rx_bytes;
                net_info->packets_recv = rx_packets;
                net_info->errors_in = (uint32_t)rx_errs;
                net_info->drops_in = (uint32_t)rx_drop;

                net_info->bytes_sent = tx_bytes;
                net_info->packets_sent = tx_packets;
                net_info->errors_out = (uint32_t)tx_errs;
                net_info->drops_out = (uint32_t)tx_drop;

                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return -1;
}

static int get_interface_mac_address(const char *interface_name, char *mac_buffer, size_t buffer_size)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
    {
        unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        snprintf(mac_buffer, buffer_size, "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        close(sock);
        return 0;
    }

    close(sock);
    return -1;
}

static int is_interface_up(const char *interface_name)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
    {
        close(sock);
        return (ifr.ifr_flags & IFF_UP) ? 1 : 0;
    }

    close(sock);
    return -1;
}

static int is_interface_wireless(const char *interface_name)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", interface_name);

    // Si existe el directorio wireless, es una interfaz inalámbrica
    FILE *f = fopen(path, "r");
    if (f)
    {
        fclose(f);
        return 1;
    }

    // También verificar por nombre común
    if (strncmp(interface_name, "wlan", 4) == 0 ||
        strncmp(interface_name, "wlp", 3) == 0 ||
        strncmp(interface_name, "wifi", 4) == 0)
    {
        return 1;
    }

    return 0;
}

static int get_interface_speed(const char *interface_name)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/speed", interface_name);

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    int speed_mbps;
    if (fscanf(f, "%d", &speed_mbps) == 1)
    {
        fclose(f);
        return speed_mbps;
    }

    fclose(f);
    return -1;
}

static int scan_network_interfaces(void)
{
    struct ifaddrs *ifaddrs_ptr, *ifa;
    interface_count = 0;

    if (getifaddrs(&ifaddrs_ptr) == -1)
    {
        return -1;
    }

    for (ifa = ifaddrs_ptr; ifa != NULL && interface_count < MAX_NETWORK_INTERFACES; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Solo procesar interfaces IPv4
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;

        // Saltar loopback
        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        // Verificar si ya tenemos esta interfaz
        int already_exists = 0;
        for (int i = 0; i < interface_count; i++)
        {
            if (strcmp(detected_interfaces[i].interface, ifa->ifa_name) == 0)
            {
                already_exists = 1;
                break;
            }
        }
        if (already_exists)
            continue;

        NetworkInfo *net_info = &detected_interfaces[interface_count];
        memset(net_info, 0, sizeof(NetworkInfo));

        // Nombre de la interfaz
        strncpy(net_info->interface, ifa->ifa_name, sizeof(net_info->interface) - 1);

        // Dirección IP
        struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
        strncpy(net_info->ip_address, inet_ntoa(addr_in->sin_addr), sizeof(net_info->ip_address) - 1);

        // Dirección MAC
        get_interface_mac_address(ifa->ifa_name, net_info->mac_address, sizeof(net_info->mac_address));

        // Estado de la interfaz
        net_info->is_up = is_interface_up(ifa->ifa_name);

        // Verificar si es inalámbrica
        net_info->is_wireless = is_interface_wireless(ifa->ifa_name);

        // Velocidad
        net_info->speed_mbps = get_interface_speed(ifa->ifa_name);

        // Estadísticas de tráfico
        get_interface_stats_from_proc(ifa->ifa_name, net_info);

        interface_count++;
    }

    freeifaddrs(ifaddrs_ptr);
    return interface_count;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int network_init(void)
{
    if (initialized)
        return 0;

    // Escanear interfaces de red
    scan_network_interfaces();

    initialized = 1;
    return 0;
}

void network_cleanup(void)
{
    initialized = 0;
    interface_count = 0;
}

int network_get_interface_count(void)
{
    if (!initialized)
    {
        network_init();
    }
    return interface_count;
}

int network_get_interface_info(int interface_id, NetworkInfo *net_info)
{
    if (!net_info || interface_id < 0 || interface_id >= interface_count)
    {
        return -1;
    }

    if (!initialized)
    {
        network_init();
    }

    // Actualizar estadísticas (pueden cambiar)
    get_interface_stats_from_proc(detected_interfaces[interface_id].interface,
                                  &detected_interfaces[interface_id]);

    // Copiar información
    memcpy(net_info, &detected_interfaces[interface_id], sizeof(NetworkInfo));
    return 0;
}

int network_get_interface_stats(const char *interface_name, NetworkInfo *net_info)
{
    if (!interface_name || !net_info)
        return -1;

    return get_interface_stats_from_proc(interface_name, net_info);
}

int network_is_interface_up(const char *interface_name)
{
    if (!interface_name)
        return -1;

    return is_interface_up(interface_name);
}

int network_is_interface_wireless(const char *interface_name)
{
    if (!interface_name)
        return -1;

    return is_interface_wireless(interface_name);
}

int network_get_interface_ip(const char *interface_name, char *ip_buffer, size_t buffer_size)
{
    if (!interface_name || !ip_buffer || buffer_size == 0)
        return -1;

    struct ifaddrs *ifaddrs_ptr, *ifa;

    if (getifaddrs(&ifaddrs_ptr) == -1)
    {
        return -1;
    }

    for (ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (strcmp(ifa->ifa_name, interface_name) != 0)
            continue;

        struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
        strncpy(ip_buffer, inet_ntoa(addr_in->sin_addr), buffer_size - 1);
        ip_buffer[buffer_size - 1] = '\0';

        freeifaddrs(ifaddrs_ptr);
        return 0;
    }

    freeifaddrs(ifaddrs_ptr);
    return -1;
}

int network_get_interface_mac(const char *interface_name, char *mac_buffer, size_t buffer_size)
{
    if (!interface_name || !mac_buffer || buffer_size == 0)
        return -1;

    return get_interface_mac_address(interface_name, mac_buffer, buffer_size);
}