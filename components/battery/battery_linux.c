/**
 * @file battery_linux.c
 * @brief Implementación de batería para Linux
 */

#include "battery_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <dirent.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static char battery_path[256] = {0};

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int find_battery_path(void) {
    glob_t glob_result;
    
    // Buscar baterías en /sys/class/power_supply/
    if (glob("/sys/class/power_supply/BAT*", GLOB_NOSORT, NULL, &glob_result) == 0) {
        if (glob_result.gl_pathc > 0) {
            strncpy(battery_path, glob_result.gl_pathv[0], sizeof(battery_path) - 1);
            battery_path[sizeof(battery_path) - 1] = '\0';
            globfree(&glob_result);
            return 0;
        }
        globfree(&glob_result);
    }
    
    return -1;
}

static int read_battery_file_int(const char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", battery_path, filename);
    
    FILE *f = fopen(full_path, "r");
    if (!f) return -1;
    
    int value;
    if (fscanf(f, "%d", &value) == 1) {
        fclose(f);
        return value;
    }
    
    fclose(f);
    return -1;
}

static int read_battery_file_string(const char *filename, char *buffer, size_t size) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", battery_path, filename);
    
    FILE *f = fopen(full_path, "r");
    if (!f) return -1;
    
    if (fgets(buffer, size, f)) {
        // Quitar newline
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        fclose(f);
        return 0;
    }
    
    fclose(f);
    return -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int battery_init(void) {
    if (initialized) return 0;
    
    if (find_battery_path() != 0) {
        return -1; // No hay batería
    }
    
    initialized = 1;
    return 0;
}

void battery_cleanup(void) {
    initialized = 0;
    battery_path[0] = '\0';
}

int battery_get_info(BatteryInfo *battery_info) {
    if (!battery_info) return -1;
    
    if (!initialized && battery_init() != 0) {
        return -1;
    }
    
    memset(battery_info, 0, sizeof(BatteryInfo));
    
    // Verificar presencia
    battery_info->is_present = 1;
    
    // Obtener capacidad actual y total
    int capacity_now = read_battery_file_int("energy_now");
    int capacity_full = read_battery_file_int("energy_full");
    int capacity_design = read_battery_file_int("energy_full_design");
    
    // Si no hay energy_*, intentar con charge_*
    if (capacity_now == -1) {
        capacity_now = read_battery_file_int("charge_now");
        capacity_full = read_battery_file_int("charge_full");
        capacity_design = read_battery_file_int("charge_full_design");
    }
    
    // Calcular porcentaje
    if (capacity_now > 0 && capacity_full > 0) {
        battery_info->percent = ((float)capacity_now / capacity_full) * 100.0f;
    }
    
    // Calcular desgaste
    if (capacity_full > 0 && capacity_design > 0) {
        battery_info->wear_level_percent = 100.0f - ((float)capacity_full / capacity_design) * 100.0f;
        battery_info->design_capacity_wh = capacity_design / 1000000.0f; // µWh a Wh
        battery_info->full_charge_capacity_wh = capacity_full / 1000000.0f;
    }
    
    // Estado de carga
    char status[64];
    if (read_battery_file_string("status", status, sizeof(status)) == 0) {
        battery_info->is_charging = (strcmp(status, "Charging") == 0) ? 1 : 0;
        battery_info->is_plugged = (strcmp(status, "Not charging") == 0 || 
                                   strcmp(status, "Charging") == 0 || 
                                   strcmp(status, "Full") == 0) ? 1 : 0;
    }
    
    // Voltaje
    int voltage_now = read_battery_file_int("voltage_now");
    if (voltage_now > 0) {
        battery_info->voltage = voltage_now / 1000000.0f; // µV a V
    } else {
        battery_info->voltage = -1.0f;
    }
    
    // Corriente
    int current_now = read_battery_file_int("current_now");
    if (current_now > 0) {
        battery_info->current_ma = current_now / 1000.0f; // µA a mA
    } else {
        battery_info->current_ma = -1.0f;
    }
    
    // Potencia
    int power_now = read_battery_file_int("power_now");
    if (power_now > 0) {
        battery_info->power_w = power_now / 1000000.0f; // µW a W
    } else if (voltage_now > 0 && current_now > 0) {
        battery_info->power_w = (voltage_now / 1000000.0f) * (current_now / 1000000.0f);
    } else {
        battery_info->power_w = -1.0f;
    }
    
    // Ciclos de carga
    battery_info->cycle_count = read_battery_file_int("cycle_count");
    
    // Tecnología
    if (read_battery_file_string("technology", battery_info->technology, 
                                sizeof(battery_info->technology)) != 0) {
        strncpy(battery_info->technology, "Unknown", sizeof(battery_info->technology) - 1);
    }
    
    // Fabricante
    if (read_battery_file_string("manufacturer", battery_info->manufacturer, 
                                sizeof(battery_info->manufacturer)) != 0) {
        strncpy(battery_info->manufacturer, "Unknown", sizeof(battery_info->manufacturer) - 1);
    }
    
    // Modelo
    if (read_battery_file_string("model_name", battery_info->model, 
                                sizeof(battery_info->model)) != 0) {
        strncpy(battery_info->model, "Unknown", sizeof(battery_info->model) - 1);
    }
    
    // Tiempo restante (estimación simple)
    if (battery_info->power_w > 0 && capacity_now > 0 && !battery_info->is_charging) {
        float remaining_wh = capacity_now / 1000000.0f;
        battery_info->time_remaining_minutes = (int)((remaining_wh / battery_info->power_w) * 60);
    } else {
        battery_info->time_remaining_minutes = -1;
    }
    
    return 0;
}

int battery_is_present(void) {
    if (!initialized && battery_init() != 0) {
        return 0;
    }
    return 1;
}

float battery_get_charge_percent(void) {
    BatteryInfo info;
    if (battery_get_info(&info) == 0) {
        return info.percent;
    }
    return -1.0f;
}

int battery_is_charging(void) {
    BatteryInfo info;
    if (battery_get_info(&info) == 0) {
        return info.is_charging;
    }
    return -1;
}

int battery_get_time_remaining_minutes(void) {
    BatteryInfo info;
    if (battery_get_info(&info) == 0) {
        return info.time_remaining_minutes;
    }
    return -1;
}

float battery_get_wear_level_percent(void) {
    BatteryInfo info;
    if (battery_get_info(&info) == 0) {
        return info.wear_level_percent;
    }
    return -1.0f;
}

int battery_get_cycle_count(void) {
    BatteryInfo info;
    if (battery_get_info(&info) == 0) {
        return info.cycle_count;
    }
    return -1;
}