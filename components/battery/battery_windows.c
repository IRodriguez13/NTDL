/**
 * @file battery_windows.c
 * @brief Implementación de batería para Windows usando Power Management API
 */

#ifdef _WIN32

#include "battery_interface.h"
#include <windows.h>
#include <powrprof.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "powrprof.lib")

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL battery_initialized = FALSE;

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int battery_init(void)
{
    battery_initialized = TRUE;
    return 0;
}

void battery_cleanup(void)
{
    battery_initialized = FALSE;
}

int battery_get_metrics(BatteryMetrics *metrics)
{
    if (!metrics || !battery_initialized)
    {
        return -1;
    }

    SYSTEM_POWER_STATUS power_status;

    // Obtener estado de energía del sistema
    if (!GetSystemPowerStatus(&power_status))
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(BatteryMetrics));

    // Verificar si hay batería
    if (power_status.BatteryFlag == 128)
    { // No system battery
        metrics->is_present = 0;
        return 0;
    }

    metrics->is_present = 1;

    // Nivel de carga
    if (power_status.BatteryLifePercent != 255)
    { // 255 = unknown
        metrics->charge_percent = (float)power_status.BatteryLifePercent;
    }
    else
    {
        metrics->charge_percent = -1.0f;
    }

    // Estado de carga
    if (power_status.ACLineStatus == 1)
    { // AC power online
        if (power_status.BatteryFlag & 8)
        { // Charging
            strncpy(metrics->status, "Charging", sizeof(metrics->status) - 1);
            metrics->is_charging = 1;
        }
        else if (power_status.BatteryLifePercent == 100)
        {
            strncpy(metrics->status, "Full", sizeof(metrics->status) - 1);
            metrics->is_charging = 0;
        }
        else
        {
            strncpy(metrics->status, "Not Charging", sizeof(metrics->status) - 1);
            metrics->is_charging = 0;
        }
    }
    else
    {
        strncpy(metrics->status, "Discharging", sizeof(metrics->status) - 1);
        metrics->is_charging = 0;
    }

    // Tiempo restante
    if (power_status.BatteryLifeTime != 0xFFFFFFFF)
    { // 0xFFFFFFFF = unknown
        metrics->time_remaining_minutes = power_status.BatteryLifeTime / 60;
    }
    else
    {
        metrics->time_remaining_minutes = -1;
    }

    // Estado de la batería
    if (power_status.BatteryFlag & 1)
    { // High
        strncpy(metrics->health, "Good", sizeof(metrics->health) - 1);
    }
    else if (power_status.BatteryFlag & 2)
    { // Low
        strncpy(metrics->health, "Low", sizeof(metrics->health) - 1);
    }
    else if (power_status.BatteryFlag & 4)
    { // Critical
        strncpy(metrics->health, "Critical", sizeof(metrics->health) - 1);
    }
    else
    {
        strncpy(metrics->health, "Unknown", sizeof(metrics->health) - 1);
    }

    // Información adicional (no disponible fácilmente en Windows)
    metrics->voltage = -1.0f;
    metrics->current_ma = -1.0f;
    metrics->power_watts = -1.0f;
    metrics->capacity_mah = -1.0f;
    metrics->design_capacity_mah = -1.0f;
    metrics->cycle_count = -1;
    metrics->temperature = -1.0f;

    // Tecnología (asumir Li-ion por defecto)
    strncpy(metrics->technology, "Li-ion", sizeof(metrics->technology) - 1);

    return 0;
}

int battery_is_present(void)
{
    if (!battery_initialized)
    {
        return -1;
    }

    SYSTEM_POWER_STATUS power_status;

    if (!GetSystemPowerStatus(&power_status))
    {
        return -1;
    }

    return (power_status.BatteryFlag != 128) ? 1 : 0; // 128 = No system battery
}

float battery_get_charge_percent(void)
{
    if (!battery_initialized)
    {
        return -1.0f;
    }

    SYSTEM_POWER_STATUS power_status;

    if (!GetSystemPowerStatus(&power_status))
    {
        return -1.0f;
    }

    if (power_status.BatteryLifePercent == 255)
    { // Unknown
        return -1.0f;
    }

    return (float)power_status.BatteryLifePercent;
}

int battery_is_charging(void)
{
    if (!battery_initialized)
    {
        return -1;
    }

    SYSTEM_POWER_STATUS power_status;

    if (!GetSystemPowerStatus(&power_status))
    {
        return -1;
    }

    // Está cargando si está conectado a AC y la batería está cargando
    return (power_status.ACLineStatus == 1 && (power_status.BatteryFlag & 8)) ? 1 : 0;
}

int battery_get_time_remaining_minutes(void)
{
    if (!battery_initialized)
    {
        return -1;
    }

    SYSTEM_POWER_STATUS power_status;

    if (!GetSystemPowerStatus(&power_status))
    {
        return -1;
    }

    if (power_status.BatteryLifeTime == 0xFFFFFFFF)
    { // Unknown
        return -1;
    }

    return (int)(power_status.BatteryLifeTime / 60);
}

int battery_get_status(char *buffer, size_t size)
{
    if (!buffer || size == 0 || !battery_initialized)
    {
        return -1;
    }

    SYSTEM_POWER_STATUS power_status;

    if (!GetSystemPowerStatus(&power_status))
    {
        return -1;
    }

    if (power_status.BatteryFlag == 128)
    { // No battery
        strncpy(buffer, "No Battery", size - 1);
    }
    else if (power_status.ACLineStatus == 1)
    { // AC power online
        if (power_status.BatteryFlag & 8)
        { // Charging
            strncpy(buffer, "Charging", size - 1);
        }
        else if (power_status.BatteryLifePercent == 100)
        {
            strncpy(buffer, "Full", size - 1);
        }
        else
        {
            strncpy(buffer, "Not Charging", size - 1);
        }
    }
    else
    {
        strncpy(buffer, "Discharging", size - 1);
    }

    buffer[size - 1] = '\0';
    return 0;
}

#endif /* _WIN32 */