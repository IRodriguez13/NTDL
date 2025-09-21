// src/battery_linux.c
#include "battery_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <dirent.h>


// Función para leer un valor entero desde un archivo
int read_int_from_file(const char *filepath, int *value)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return -1;

    if (fscanf(fp, "%d", value) != 1)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

// Función para leer una cadena desde un archivo
int read_string_from_file(const char *filepath, char *buffer, size_t buffer_size)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp)
        return -1;

    if (fgets(buffer, buffer_size, fp))
    {
        // Quitar salto de línea
        char *newline = strchr(buffer, '\n');
        if (newline)
            *newline = '\0';
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

// Función para obtener información de una batería específica
int get_single_battery_info(const char *battery_path, BatteryInfo *info)
{
    char filepath[512];
    char status_str[32];
    int temp_value;

    // Verificar si la batería está presente
    snprintf(filepath, sizeof(filepath), "%s/present", battery_path);
    if (read_int_from_file(filepath, &temp_value) != 0 || temp_value != 1)
    {
        info->is_present = 0;
        return -1;
    }
    info->is_present = 1;

    // Obtener estado de carga (Charging, Discharging, Full, etc.)
    snprintf(filepath, sizeof(filepath), "%s/status", battery_path);
    if (read_string_from_file(filepath, status_str, sizeof(status_str)) == 0)
    {
        if (strcmp(status_str, "Charging") == 0)
        {
            info->is_charging = 1;
        }
        else if (strcmp(status_str, "Discharging") == 0)
        {
            info->is_charging = 0;
        }
        else if (strcmp(status_str, "Full") == 0)
        {
            info->is_charging = 0;
            info->percentage = 100;
        }
        else
        {
            info->is_charging = 0;
        }
    }

    // Obtener capacidad (porcentaje)
    snprintf(filepath, sizeof(filepath), "%s/capacity", battery_path);
    if (read_int_from_file(filepath, &info->percentage) != 0)
    {
        info->percentage = -1;
    }

    // Obtener capacidad de diseño (en µWh o µAh)
    snprintf(filepath, sizeof(filepath), "%s/energy_full_design", battery_path);
    if (read_int_from_file(filepath, &temp_value) == 0)
    {
        info->design_capacity_mwh = temp_value / 1000; // Convertir de µWh a mWh
    }
    else
    {
        // Intentar con charge_full_design (para baterías que reportan en mAh)
        snprintf(filepath, sizeof(filepath), "%s/charge_full_design", battery_path);
        if (read_int_from_file(filepath, &temp_value) == 0)
        {
            // Obtener voltaje para convertir mAh a mWh
            int voltage = 0;
            snprintf(filepath, sizeof(filepath), "%s/voltage_now", battery_path);
            if (read_int_from_file(filepath, &voltage) == 0)
            {
                info->design_capacity_mwh = (temp_value / 1000) * (voltage / 1000000);
            }
            else
            {
                info->design_capacity_mwh = -1;
            }
        }
        else
        {
            info->design_capacity_mwh = -1;
        }
    }

    // Obtener capacidad actual completa
    snprintf(filepath, sizeof(filepath), "%s/energy_full", battery_path);
    if (read_int_from_file(filepath, &temp_value) == 0)
    {
        info->full_charge_capacity_mwh = temp_value / 1000;
    }
    else
    {
        snprintf(filepath, sizeof(filepath), "%s/charge_full", battery_path);
        if (read_int_from_file(filepath, &temp_value) == 0)
        {
            int voltage = 0;
            snprintf(filepath, sizeof(filepath), "%s/voltage_now", battery_path);
            if (read_int_from_file(filepath, &voltage) != 0)
            {
                info->full_charge_capacity_mwh = -1;
            }
            
            info->full_charge_capacity_mwh = (temp_value / 1000) * (voltage / 1000000);
            
        }
        else
        {
            info->full_charge_capacity_mwh = -1;
        }
    }
            

    // Obtener capacidad actual
    snprintf(filepath, sizeof(filepath), "%s/energy_now", battery_path);
    if (read_int_from_file(filepath, &temp_value) == 0)
    {
        info->current_capacity_mwh = temp_value / 1000;
    }
    else
    {
        snprintf(filepath, sizeof(filepath), "%s/charge_now", battery_path);
        if (read_int_from_file(filepath, &temp_value) == 0)
        {
            int voltage = 0;
            snprintf(filepath, sizeof(filepath), "%s/voltage_now", battery_path);
            if (read_int_from_file(filepath, &voltage) != 0)
            {
                info->current_capacity_mwh = -1;
                return;
            }

            info->current_capacity_mwh = (temp_value / 1000) * (voltage / 1000000);
        }
        else
        {
            info->current_capacity_mwh = -1;
        }
    }

    // Obtener ciclos de carga
    snprintf(filepath, sizeof(filepath), "%s/cycle_count", battery_path);
    if (read_int_from_file(filepath, &info->cycle_count) != 0)
    {
        info->cycle_count = -1;
    }

    // Obtener voltaje actual
    snprintf(filepath, sizeof(filepath), "%s/voltage_now", battery_path);
    if (read_int_from_file(filepath, &temp_value) == 0)
    {
        info->voltage_mv = temp_value / 1000; // Convertir de µV a mV
    }
    else
    {
        info->voltage_mv = -1;
    }

    // Obtener corriente actual
    snprintf(filepath, sizeof(filepath), "%s/current_now", battery_path);
    if (read_int_from_file(filepath, &temp_value) == 0)
    {
        info->current_ma = temp_value / 1000; // Convertir de µA a mA
    }
    else
    {
        info->current_ma = -1;
    }

    // Calcular tiempo restante (estimado)
    info->time_remaining_minutes = -1;
    if (info->current_capacity_mwh > 0 && info->current_ma != 0)
    {
        if (info->is_charging)
        {
            // Tiempo hasta carga completa
            if (info->current_ma > 0 && info->full_charge_capacity_mwh > 0)
            {
                int remaining_mwh = info->full_charge_capacity_mwh - info->current_capacity_mwh;
                if (remaining_mwh > 0)
                {
                    info->time_remaining_minutes = (remaining_mwh * 60) / abs(info->current_ma);
                }
            }
        }
        else
        {
            // Tiempo hasta descarga completa
            if (info->current_ma < 0)
            {
                info->time_remaining_minutes = (info->current_capacity_mwh * 60) / abs(info->current_ma);
            }
        }
    }

    return 0;
}

// Función para liberar memoria de información de batería
void free_battery_info(BatteryInfo *info)
{
    if (info)
    {
        free(info);
    }
}

// Función principal para obtener información de batería
BatteryInfo *alloc_battery_info()
{
    BatteryInfo *info = (BatteryInfo *)malloc(sizeof(BatteryInfo));
    if (!info)
    {
        Kerror("malloc failed for BatteryInfo");
        goto clenaup;
        return NULL;
    }

    memset(info, 0, sizeof(BatteryInfo));

    // Valores por defecto
    info->is_present = 0;
    info->percentage = -1;
    info->design_capacity_mwh = -1;
    info->full_charge_capacity_mwh = -1;
    info->current_capacity_mwh = -1;
    info->cycle_count = -1;
    info->voltage_mv = -1;
    info->current_ma = -1;
    info->time_remaining_minutes = -1;

    // Buscar baterías en /sys/class/power_supply/
    glob_t glob_result;
    int glob_status = glob("/sys/class/power_supply/BAT*", GLOB_NOSORT, NULL, &glob_result);

    if (glob_status != 0)
    {
        printf("DEBUG: No se encontraron baterías en /sys/class/power_supply/\n");
        free(info);
        return NULL;
    }

    // Usar la primera batería encontrada
    if (glob_result.gl_pathc > 0)
    {
        printf("DEBUG: Procesando batería: %s\n", glob_result.gl_pathv[0]);

        if (get_single_battery_info(glob_result.gl_pathv[0], info) == 0)
        {
            printf("DEBUG: Información de batería obtenida exitosamente\n");
            globfree(&glob_result);
            return info;
        }
    }

    globfree(&glob_result);

    // Si no se encontró información válida de batería
    printf("DEBUG: No se pudo obtener información de batería válida\n");
    return NULL;

clenaup:
    free(info);

}

// Función para imprimir información de batería
void print_battery_info(const BatteryInfo *info)
{
    if (!info)
    {
        printf("\n==== Battery Info ====\n");
        printf("No battery found or information not available\n");
        printf("======================\n");
        return;
    }

    if (!info->is_present)
    {
        printf("\n==== Battery Info ====\n");
        printf("Battery not present\n");
        printf("======================\n");
        return;
    }

    printf("\n==== Battery Info ====\n");

    // Estado y porcentaje
    if (info->percentage >= 0)
    {
        printf("Charge Level:     %d%%\n", info->percentage);
    }

    printf("Status:           %s\n", info->is_charging ? "Charging" : "Discharging");

    // Capacidades
    if (info->design_capacity_mwh > 0)
    {
        printf("Design Capacity:  %d mWh\n", info->design_capacity_mwh);
    }

    if (info->full_charge_capacity_mwh > 0)
    {
        printf("Full Capacity:    %d mWh\n", info->full_charge_capacity_mwh);

        // Calcular salud de la batería
        if (info->design_capacity_mwh > 0)
        {
            int health = (info->full_charge_capacity_mwh * 100) / info->design_capacity_mwh;
            printf("Battery Health:   %d%%\n", health);
        }
    }

    if (info->current_capacity_mwh > 0)
    {
        printf("Current Capacity: %d mWh\n", info->current_capacity_mwh);
    }

    // Ciclos de carga
    if (info->cycle_count >= 0)
    {
        printf("Charge Cycles:    %d\n", info->cycle_count);
    }

    // Información eléctrica
    if (info->voltage_mv > 0)
    {
        printf("Voltage:          %d mV (%.2f V)\n", info->voltage_mv, info->voltage_mv / 1000.0);
    }

    if (info->current_ma != 0)
    {
        if (info->is_charging)
        {
            printf("Charging Rate:    %d mA\n", abs(info->current_ma));
        }
        else
        {
            printf("Discharge Rate:   %d mA\n", abs(info->current_ma));
        }
    }

    // Tiempo restante
    if (info->time_remaining_minutes > 0)
    {
        int hours = info->time_remaining_minutes / 60;
        int minutes = info->time_remaining_minutes % 60;

        if (info->is_charging)
        {
            printf("Time to Full:     %d:%02d\n", hours, minutes);
        }
        else
        {
            printf("Time Remaining:   %d:%02d\n", hours, minutes);
        }
    }

    printf("======================\n");
}

