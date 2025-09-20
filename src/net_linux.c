// src/network_linux.c
#include "network_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <ifaddrs.h>

// Función para leer estadísticas de red desde /proc/net/dev
int read_network_stats(const char *interface, NetworkStats *stats)
{
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return -1;

    char line[512];
    int found = 0;

    // Saltar las dos primeras líneas de cabecera
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp))
    {
        char iface[32];
        unsigned long long rx_bytes, rx_packets, rx_errs, rx_drop;
        unsigned long long tx_bytes, tx_packets, tx_errs, tx_drop;

        // Parsear línea: interface: rx_bytes rx_packets rx_errs rx_drop ... tx_bytes tx_packets tx_errs tx_drop ...
        if (sscanf(line, " %31[^:]: %llu %llu %llu %llu %*u %*u %*u %*u %llu %llu %llu %llu",
                   iface, &rx_bytes, &rx_packets, &rx_errs, &rx_drop,
                   &tx_bytes, &tx_packets, &tx_errs, &tx_drop) >= 8)
        {

            if (strcmp(iface, interface) == 0)
            {
                stats->bytes_received = rx_bytes;
                stats->packets_received = rx_packets;
                stats->errors_received = rx_errs;
                stats->drops_received = rx_drop;
                stats->bytes_sent = tx_bytes;
                stats->packets_sent = tx_packets;
                stats->errors_sent = tx_errs;
                stats->drops_sent = tx_drop;
                found = 1;
                break;
            }
        }
    }

    fclose(fp);
    return found ? 0 : -1;
}

// Función para obtener velocidad del enlace usando ethtool
int get_link_speed(const char *interface)
{
    int sockfd;
    struct ifreq ifr;
    struct ethtool_cmd edata;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    edata.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (caddr_t)&edata;

    if (ioctl(sockfd, SIOCETHTOOL, &ifr) == 0)
    {
        close(sockfd);
        return edata.speed; // Velocidad en Mbps
    }

    close(sockfd);
    return -1;
}

// Función para verificar si la interfaz está activa
int is_interface_up(const char *interface)
{
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0)
    {
        close(sockfd);
        return (ifr.ifr_flags & IFF_UP) ? 1 : 0;
    }

    close(sockfd);
    return -1;
}

// Función para obtener dirección MAC
int get_mac_address(const char *interface, char *mac_str, size_t mac_size)
{
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0)
    {
        unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        snprintf(mac_str, mac_size, "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        close(sockfd);
        return 0;
    }

    close(sockfd);
    return -1;
}

// Función para obtener información de WiFi
int get_wifi_info(const char *interface, WiFiInfo *wifi_info)
{
    FILE *fp;
    char cmd[256];
    char line[256];

    // Verificar si es interfaz WiFi consultando /proc/net/wireless
    fp = fopen("/proc/net/wireless", "r");
    if (!fp)
        return -1;

    int is_wifi = 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, interface))
        {
            is_wifi = 1;
            break;
        }
    }
    fclose(fp);

    if (!is_wifi)
        return -1;

    // Inicializar estructura WiFi
    memset(wifi_info, 0, sizeof(WiFiInfo));
    wifi_info->signal_strength = -999;

    // Intentar obtener SSID usando iwgetid
    snprintf(cmd, sizeof(cmd), "iwgetid -r %s 2>/dev/null", interface);
    fp = popen(cmd, "r");
    if (fp)
    {
        if (fgets(wifi_info->ssid, sizeof(wifi_info->ssid), fp))
        {
            // Quitar salto de línea
            char *newline = strchr(wifi_info->ssid, '\n');
            if (newline)
                *newline = '\0';
        }
        pclose(fp);
    }

    // Intentar obtener información de señal usando iwconfig
    snprintf(cmd, sizeof(cmd), "iwconfig %s 2>/dev/null | grep 'Signal level'", interface);
    fp = popen(cmd, "r");
    if (fp)
    {
        if (fgets(line, sizeof(line), fp))
        {
            // Parsear algo como "Signal level=-45 dBm"
            char *signal_pos = strstr(line, "Signal level=");
            if (signal_pos)
            {
                int signal;
                if (sscanf(signal_pos, "Signal level=%d dBm", &signal) == 1)
                {
                    wifi_info->signal_strength = signal;
                }
            }
        }
        pclose(fp);
    }

    return 0;
}

// Función principal para obtener información de red
NetworkInfo *alloc_network_info()
{
    NetworkInfo *info = (NetworkInfo *)malloc(sizeof(NetworkInfo));
    if (!info)
    {
        Kerror("malloc failed for NetworkInfo");
        return NULL;
    }

    memset(info, 0, sizeof(NetworkInfo));

    struct ifaddrs *ifaddrs_ptr;
    if (getifaddrs(&ifaddrs_ptr) != 0)
    {
        free(info);
        return NULL;
    }

    int interface_count = 0;
    struct ifaddrs *ifa;

    for (ifa = ifaddrs_ptr; ifa != NULL && interface_count < MAX_NETWORK_INTERFACES; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Saltar interfaz loopback
        if (strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        // Verificar si ya hemos procesado esta interfaz
        int already_processed = 0;
        for (int i = 0; i < interface_count; i++)
        {
            if (strcmp(info->interfaces[i].interface_name, ifa->ifa_name) == 0)
            {
                already_processed = 1;
                break;
            }
        }
        if (already_processed)
            continue;

        NetworkInterface *netif = &info->interfaces[interface_count];

        // Nombre de la interfaz
        strncpy(netif->interface_name, ifa->ifa_name, sizeof(netif->interface_name) - 1);

        // Obtener dirección MAC
        if (get_mac_address(ifa->ifa_name, netif->mac_address, sizeof(netif->mac_address)) != 0)
        {
            strcpy(netif->mac_address, "Unknown");
        }

        // Verificar si está activa
        netif->is_up = is_interface_up(ifa->ifa_name);

        // Obtener velocidad del enlace
        netif->link_speed_mbps = get_link_speed(ifa->ifa_name);

        // Obtener estadísticas de red
        NetworkStats stats;
        if (read_network_stats(ifa->ifa_name, &stats) == 0)
        {
            netif->stats = stats;
        }

        // Verificar si es WiFi e obtener información adicional
        WiFiInfo wifi_info;
        if (get_wifi_info(ifa->ifa_name, &wifi_info) == 0)
        {
            netif->wifi_info = wifi_info;
            netif->is_wifi = 1;
        }
        else
        {
            netif->is_wifi = 0;
            memset(&netif->wifi_info, 0, sizeof(WiFiInfo));
            netif->wifi_info.signal_strength = -999;
        }

        printf("DEBUG: Interfaz de red añadida: %s (UP=%d, WiFi=%d)\n",
               netif->interface_name, netif->is_up, netif->is_wifi);

        interface_count++;
    }

    freeifaddrs(ifaddrs_ptr);

    info->interface_count = interface_count;
    printf("DEBUG: Total interfaces de red encontradas: %d\n", interface_count);

    return info;
}

// Función para liberar memoria de información de red
void free_network_info(NetworkInfo *info)
{
    if (info)
    {
        free(info);
    }
}

// Función para imprimir información de red
void print_network_info(const NetworkInfo *info)
{
    if (!info || info->interface_count == 0)
    {
        printf("\n==== Network Interfaces ====\n");
        printf("No network interfaces found\n");
        printf("============================\n");
        return;
    }

    printf("\n==== Network Interfaces ====\n");
    printf("Found %d network interfaces:\n\n", info->interface_count);

    for (int i = 0; i < info->interface_count; i++)
    {
        const NetworkInterface *netif = &info->interfaces[i];

        printf("--- %s ---\n", netif->interface_name);
        printf("  MAC Address: %s\n", netif->mac_address);
        printf("  Status: %s\n", netif->is_up ? "UP" : "DOWN");

        if (netif->link_speed_mbps > 0)
        {
            printf("  Link Speed: %d Mbps\n", netif->link_speed_mbps);
        }
        else
        {
            printf("  Link Speed: Unknown\n");
        }

        // Estadísticas de tráfico
        printf("  Statistics:\n");
        printf("    RX: %llu bytes, %llu packets", netif->stats.bytes_received, netif->stats.packets_received);
        if (netif->stats.errors_received > 0 || netif->stats.drops_received > 0)
        {
            printf(" (%llu errors, %llu drops)", netif->stats.errors_received, netif->stats.drops_received);
        }
        printf("\n");

        printf("    TX: %llu bytes, %llu packets", netif->stats.bytes_sent, netif->stats.packets_sent);
        if (netif->stats.errors_sent > 0 || netif->stats.drops_sent > 0)
        {
            printf(" (%llu errors, %llu drops)", netif->stats.errors_sent, netif->stats.drops_sent);
        }
        printf("\n");

        // Información WiFi si aplica
        if (netif->is_wifi)
        {
            printf("  WiFi Info:\n");
            if (strlen(netif->wifi_info.ssid) > 0)
            {
                printf("    SSID: %s\n", netif->wifi_info.ssid);
            }
            if (netif->wifi_info.signal_strength > -999)
            {
                printf("    Signal: %d dBm\n", netif->wifi_info.signal_strength);
            }
        }

        printf("\n");
    }

    printf("============================\n");
}