// src/battery_linux.h
#pragma once

#include "../common/common.h"
#include <stddef.h>

// Estructura para información de batería
typedef struct
{
    int is_present;               // 1 si la batería está presente, 0 si no
    int percentage;               // Porcentaje de carga (0-100, -1 si no disponible)
    int is_charging;              // 1 si está cargando, 0 si está descargando
    int design_capacity_mwh;      // Capacidad de diseño en mWh (-1 si no disponible)
    int full_charge_capacity_mwh; // Capacidad actual completa en mWh (-1 si no disponible)
    int current_capacity_mwh;     // Capacidad actual en mWh (-1 si no disponible)
    int cycle_count;              // Número de ciclos de carga (-1 si no disponible)
    int voltage_mv;               // Voltaje actual en mV (-1 si no disponible)
    int current_ma;               // Corriente actual en mA (positivo=carga, negativo=descarga, 0 si no disponible)
    int time_remaining_minutes;   // Tiempo restante en minutos (-1 si no disponible)
} BatteryInfo;

// Funciones principales
BatteryInfo *alloc_battery_info();
void free_battery_info(BatteryInfo *info);
void print_battery_info(const BatteryInfo *info);

// Funciones auxiliares
int get_single_battery_info(const char *battery_path, BatteryInfo *info);
int read_int_from_file(const char *filepath, int *value);
int read_string_from_file(const char *filepath, char *buffer, size_t buffer_size);