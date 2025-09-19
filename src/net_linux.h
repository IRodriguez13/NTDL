// src/network_linux.h
#pragma once

#include "../common/common.h"
#include <stddef.h>

#define MAX_NETWORK_INTERFACES 16

// Estructura para estadísticas de red
typedef struct
{
    unsigned long long bytes_sent;
    unsigned long long bytes_received;
    unsigned long long packets_sent;
    unsigned long long packets_received;
    unsigned long long errors_sent;
    unsigned long long errors_received;
    unsigned long long drops_sent;
    unsigned long long drops_received;
} NetworkStats;

// Estructura para información WiFi
typedef struct
{
    char ssid[64];       // SSID de la red WiFi
    int signal_strength; // Fuerza de señal en dBm (-999 si no disponible)
    int channel;         // Canal WiFi (0 si no disponible)
} WiFiInfo;

// Estructura para interfaz de red
typedef struct
{
    char interface_name[64]; // Nombre de la interfaz (eth0, wlan0, etc.)
    char mac_address[18];    // Dirección MAC (formato xx:xx:xx:xx:xx:xx)
    int is_up;               // 1 si está activa, 0 si no
    int link_speed_mbps;     // Velocidad del enlace en Mbps (-1 si no disponible)
    NetworkStats stats;      // Estadísticas de tráfico
    int is_wifi;             // 1 si es interfaz WiFi, 0 si no
    WiFiInfo wifi_info;      // Información WiFi (si aplica)
} NetworkInterface;

// Estructura principal para información de red
typedef struct
{
    NetworkInterface interfaces[MAX_NETWORK_INTERFACES];
    int interface_count;
} NetworkInfo;

// Funciones principales
NetworkInfo *alloc_network_info();
void free_network_info(NetworkInfo *info);
void print_network_info(const NetworkInfo *info);

// Funciones auxiliares
int read_network_stats(const char *interface, NetworkStats *stats);
int get_link_speed(const char *interface);
int is_interface_up(const char *interface);
int get_mac_address(const char *interface, char *mac_str, size_t mac_size);
int get_wifi_info(const char *interface, WiFiInfo *wifi_info);