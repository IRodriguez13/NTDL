// src/sensors_linux.h
#pragma once

#include "../common/common.h"
#include <stddef.h>
#include <ctype.h>

#define MAX_SENSORS 32

// Estructura para un sensor individual de temperatura
typedef struct
{
    char sensor_name[64];       // Nombre del sensor (ej: "Core 0", "GPU", etc.)
    char sensor_type[32];       // Tipo ("CPU", "GPU", "Motherboard", "Storage", "Other")
    float temperature;          // Temperatura actual en °C
    float max_temperature;      // Temperatura máxima segura (-1 si no disponible)
    float critical_temperature; // Temperatura crítica (-1 si no disponible)
} TemperatureSensor;

// Estructura para información completa de temperatura
typedef struct
{
    TemperatureSensor sensors[MAX_SENSORS];
    int sensor_count;
} TemperatureInfo;

// Funciones principales
TemperatureInfo *alloc_temperature_info();
void free_temperature_info(TemperatureInfo *info);
void print_temperature_info(const TemperatureInfo *info);

// Funciones auxiliares
int read_hwmon_temperature(const char *hwmon_path, const char *temp_file, float *temperature);
int read_sensor_label(const char *hwmon_path, const char *label_file, char *label, size_t label_size);
void determine_sensor_type(const char *name, const char *label, char *type, size_t type_size);