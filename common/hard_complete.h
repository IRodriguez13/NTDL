// common/hardware_complete.h
#pragma once

#include "common.h"
#include <stddef.h>

// ==================== ESTRUCTURAS PARA GPU ====================
typedef struct
{
    char name[128];
    char vendor[64];
    char driver_version[64];
    int memory_total_mb;
    int memory_used_mb;
    int temperature;
    float utilization_percent;
    int core_clock_mhz;
    int memory_clock_mhz;
} GpuInfo;

// ==================== ESTRUCTURAS PARA SENSORES ====================
#define MAX_SENSORS 32

typedef struct
{
    char sensor_name[64];
    char sensor_type[32];
    float temperature;
    float max_temperature;
    float critical_temperature;
} TemperatureSensor;

typedef struct
{
    TemperatureSensor sensors[MAX_SENSORS];
    int sensor_count;
} TemperatureInfo;

// ==================== ESTRUCTURAS PARA RED ====================
#define MAX_NETWORK_INTERFACES 16

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

typedef struct
{
    char ssid[64];
    int signal_strength;
    int channel;
} WiFiInfo;

typedef struct
{
    char interface_name[64];
    char mac_address[18];
    int is_up;
    int link_speed_mbps;
    NetworkStats stats;
    int is_wifi;
    WiFiInfo wifi_info;
} NetworkInterface;

typedef struct
{
    NetworkInterface interfaces[MAX_NETWORK_INTERFACES];
    int interface_count;
} NetworkInfo;

// ==================== ESTRUCTURAS PARA BATERÍA ====================
typedef struct
{
    int is_present;
    int percentage;
    int is_charging;
    int design_capacity_mwh;
    int full_charge_capacity_mwh;
    int current_capacity_mwh;
    int cycle_count;
    int voltage_mv;
    int current_ma;
    int time_remaining_minutes;
} BatteryInfo;

// ==================== FUNCIONES EXPORTADAS ====================

// CPU (ya existentes)
CpuInfo *alloc_cpu_info();
void free_cpu_info(CpuInfo *info);
int get_cpu_model(char *buffer, size_t size);
double get_cpu_usage();

// GPU
GpuInfo *alloc_gpu_info();
void free_gpu_info(GpuInfo *info);
void print_gpu_info(const GpuInfo *info);

// Sensores de Temperatura
TemperatureInfo *alloc_temperature_info();
void free_temperature_info(TemperatureInfo *info);
void print_temperature_info(const TemperatureInfo *info);

// Red
NetworkInfo *alloc_network_info();
void free_network_info(NetworkInfo *info);
void print_network_info(const NetworkInfo *info);

// Batería
BatteryInfo *alloc_battery_info();
void free_battery_info(BatteryInfo *info);
void print_battery_info(const BatteryInfo *info);