/**
 * @file sensors_linux.c
 * @brief Implementación de sensores para Linux
 */

#include "sensors_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <dirent.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int sensor_count = 0;
static SensorInfo detected_sensors[MAX_SENSORS];

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static SensorType determine_sensor_type(const char *sensor_path, const char *sensor_name) {
    if (strstr(sensor_path, "temp") || strstr(sensor_name, "temp") || 
        strstr(sensor_name, "thermal") || strstr(sensor_name, "Core")) {
        return SENSOR_TYPE_TEMPERATURE;
    }
    
    if (strstr(sensor_path, "fan") || strstr(sensor_name, "fan") || 
        strstr(sensor_name, "Fan")) {
        return SENSOR_TYPE_FAN_SPEED;
    }
    
    if (strstr(sensor_path, "in") || strstr(sensor_name, "in") || 
        strstr(sensor_name, "Vcore") || strstr(sensor_name, "voltage")) {
        return SENSOR_TYPE_VOLTAGE;
    }
    
    if (strstr(sensor_path, "power") || strstr(sensor_name, "power") || 
        strstr(sensor_name, "Power")) {
        return SENSOR_TYPE_POWER;
    }
    
    if (strstr(sensor_path, "curr") || strstr(sensor_name, "curr") || 
        strstr(sensor_name, "Current")) {
        return SENSOR_TYPE_CURRENT;
    }
    
    return SENSOR_TYPE_UNKNOWN;
}

static void set_sensor_unit(SensorInfo *sensor) {
    switch (sensor->type) {
        case SENSOR_TYPE_TEMPERATURE:
            strcpy(sensor->unit, "°C");
            break;
        case SENSOR_TYPE_VOLTAGE:
            strcpy(sensor->unit, "V");
            break;
        case SENSOR_TYPE_FAN_SPEED:
            strcpy(sensor->unit, "RPM");
            break;
        case SENSOR_TYPE_POWER:
            strcpy(sensor->unit, "W");
            break;
        case SENSOR_TYPE_CURRENT:
            strcpy(sensor->unit, "A");
            break;
        case SENSOR_TYPE_HUMIDITY:
            strcpy(sensor->unit, "%");
            break;
        default:
            strcpy(sensor->unit, "");
            break;
    }
}

static int read_sensor_value(const char *path, float *value) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    int raw_value;
    if (fscanf(f, "%d", &raw_value) == 1) {
        // La mayoría de sensores en Linux reportan valores en miliunidades
        *value = raw_value / 1000.0f;
        fclose(f);
        return 0;
    }
    
    fclose(f);
    return -1;
}

static int read_sensor_label(const char *base_path, const char *sensor_file, char *label, size_t label_size) {
    char label_path[512];
    char *underscore = strrchr(sensor_file, '_');
    if (!underscore) return -1;
    
    // Construir path del label (ej: temp1_input -> temp1_label)
    int prefix_len = underscore - sensor_file;
    snprintf(label_path, sizeof(label_path), "%s/%.*s_label", base_path, prefix_len, sensor_file);
    
    FILE *f = fopen(label_path, "r");
    if (f) {
        if (fgets(label, label_size, f)) {
            // Quitar newline
            char *newline = strchr(label, '\n');
            if (newline) *newline = '\0';
            fclose(f);
            return 0;
        }
        fclose(f);
    }
    
    // Si no hay label, usar el nombre del archivo
    strncpy(label, sensor_file, label_size - 1);
    label[label_size - 1] = '\0';
    return 0;
}

static int scan_hwmon_sensors(void) {
    glob_t glob_result;
    sensor_count = 0;
    
    // Buscar todos los directorios hwmon
    if (glob("/sys/class/hwmon/hwmon*", GLOB_NOSORT, NULL, &glob_result) != 0) {
        return -1;
    }
    
    for (size_t i = 0; i < glob_result.gl_pathc && sensor_count < MAX_SENSORS; i++) {
        DIR *dir = opendir(glob_result.gl_pathv[i]);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && sensor_count < MAX_SENSORS) {
            // Buscar archivos de sensores (temp*_input, fan*_input, in*_input, etc.)
            if (strstr(entry->d_name, "_input") == NULL) continue;
            
            char sensor_path[512];
            snprintf(sensor_path, sizeof(sensor_path), "%s/%s", glob_result.gl_pathv[i], entry->d_name);
            
            SensorInfo *sensor = &detected_sensors[sensor_count];
            memset(sensor, 0, sizeof(SensorInfo));
            
            // Leer valor del sensor
            if (read_sensor_value(sensor_path, &sensor->value) != 0) {
                continue; // Saltar si no podemos leer el valor
            }
            
            // Obtener label del sensor
            read_sensor_label(glob_result.gl_pathv[i], entry->d_name, sensor->label, sizeof(sensor->label));
            
            // Determinar tipo de sensor
            sensor->type = determine_sensor_type(sensor_path, sensor->label);
            
            // Establecer unidad
            set_sensor_unit(sensor);
            
            // Construir nombre completo
            char *hwmon_name = strrchr(glob_result.gl_pathv[i], '/');
            if (hwmon_name) {
                hwmon_name++; // Saltar '/'
                int ret = snprintf(sensor->name, sizeof(sensor->name), "%s/%s", hwmon_name, sensor->label);
                if (ret >= (int)sizeof(sensor->name)) {
                    // Truncated, use just the label
                    strncpy(sensor->name, sensor->label, sizeof(sensor->name) - 1);
                    sensor->name[sizeof(sensor->name) - 1] = '\0';
                }
            } else {
                strncpy(sensor->name, sensor->label, sizeof(sensor->name) - 1);
                sensor->name[sizeof(sensor->name) - 1] = '\0';
            }
            
            // Intentar leer valores min/max/critical si existen
            char temp_path[512];
            char *underscore = strrchr(entry->d_name, '_');
            if (underscore) {
                int prefix_len = underscore - entry->d_name;
                
                // Valor mínimo
                snprintf(temp_path, sizeof(temp_path), "%s/%.*s_min", 
                         glob_result.gl_pathv[i], prefix_len, entry->d_name);
                read_sensor_value(temp_path, &sensor->min_value);
                
                // Valor máximo
                snprintf(temp_path, sizeof(temp_path), "%s/%.*s_max", 
                         glob_result.gl_pathv[i], prefix_len, entry->d_name);
                read_sensor_value(temp_path, &sensor->max_value);
                
                // Valor crítico
                snprintf(temp_path, sizeof(temp_path), "%s/%.*s_crit", 
                         glob_result.gl_pathv[i], prefix_len, entry->d_name);
                read_sensor_value(temp_path, &sensor->critical_value);
            }
            
            // Valores por defecto si no se pudieron leer
            if (sensor->min_value == 0.0f) sensor->min_value = -1.0f;
            if (sensor->max_value == 0.0f) sensor->max_value = -1.0f;
            if (sensor->critical_value == 0.0f) sensor->critical_value = -1.0f;
            
            sensor_count++;
        }
        
        closedir(dir);
    }
    
    globfree(&glob_result);
    return sensor_count;
}

static int scan_thermal_zone_sensors(void) {
    glob_t glob_result;
    
    // Buscar thermal zones adicionales
    if (glob("/sys/class/thermal/thermal_zone*/temp", GLOB_NOSORT, NULL, &glob_result) != 0) {
        return 0; // No es error, simplemente no hay thermal zones
    }
    
    for (size_t i = 0; i < glob_result.gl_pathc && sensor_count < MAX_SENSORS; i++) {
        SensorInfo *sensor = &detected_sensors[sensor_count];
        memset(sensor, 0, sizeof(SensorInfo));
        
        // Leer temperatura
        if (read_sensor_value(glob_result.gl_pathv[i], &sensor->value) != 0) {
            continue;
        }
        
        // Construir nombre
        char *zone_path = strdup(glob_result.gl_pathv[i]);
        char *temp_pos = strstr(zone_path, "/temp");
        if (temp_pos) {
            *temp_pos = '\0';
            char *zone_name = strrchr(zone_path, '/');
            if (zone_name) {
                zone_name++; // Saltar '/'
                snprintf(sensor->name, sizeof(sensor->name), "thermal/%s", zone_name);
                strncpy(sensor->label, zone_name, sizeof(sensor->label) - 1);
            }
        }
        free(zone_path);
        
        sensor->type = SENSOR_TYPE_TEMPERATURE;
        strcpy(sensor->unit, "°C");
        sensor->min_value = -1.0f;
        sensor->max_value = -1.0f;
        sensor->critical_value = -1.0f;
        
        sensor_count++;
    }
    
    globfree(&glob_result);
    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int sensors_init(void) {
    if (initialized) return 0;
    
    // Escanear sensores hwmon
    scan_hwmon_sensors();
    
    // Escanear thermal zones
    scan_thermal_zone_sensors();
    
    initialized = 1;
    return 0;
}

void sensors_cleanup(void) {
    initialized = 0;
    sensor_count = 0;
}

int sensors_get_count(void) {
    if (!initialized) {
        sensors_init();
    }
    return sensor_count;
}

int sensors_get_info(int sensor_id, SensorInfo *sensor_info) {
    if (!sensor_info || sensor_id < 0 || sensor_id >= sensor_count) {
        return -1;
    }
    
    if (!initialized) {
        sensors_init();
    }
    
    // Copiar información del sensor
    memcpy(sensor_info, &detected_sensors[sensor_id], sizeof(SensorInfo));
    return 0;
}

int sensors_get_temperature_sensors(SensorInfo *temp_sensors, int max_sensors) {
    if (!temp_sensors || max_sensors <= 0) return -1;
    
    if (!initialized) {
        sensors_init();
    }
    
    int temp_count = 0;
    for (int i = 0; i < sensor_count && temp_count < max_sensors; i++) {
        if (detected_sensors[i].type == SENSOR_TYPE_TEMPERATURE) {
            memcpy(&temp_sensors[temp_count], &detected_sensors[i], sizeof(SensorInfo));
            temp_count++;
        }
    }
    
    return temp_count;
}

int sensors_get_fan_sensors(SensorInfo *fan_sensors, int max_sensors) {
    if (!fan_sensors || max_sensors <= 0) return -1;
    
    if (!initialized) {
        sensors_init();
    }
    
    int fan_count = 0;
    for (int i = 0; i < sensor_count && fan_count < max_sensors; i++) {
        if (detected_sensors[i].type == SENSOR_TYPE_FAN_SPEED) {
            memcpy(&fan_sensors[fan_count], &detected_sensors[i], sizeof(SensorInfo));
            fan_count++;
        }
    }
    
    return fan_count;
}

int sensors_get_voltage_sensors(SensorInfo *voltage_sensors, int max_sensors) {
    if (!voltage_sensors || max_sensors <= 0) return -1;
    
    if (!initialized) {
        sensors_init();
    }
    
    int voltage_count = 0;
    for (int i = 0; i < sensor_count && voltage_count < max_sensors; i++) {
        if (detected_sensors[i].type == SENSOR_TYPE_VOLTAGE) {
            memcpy(&voltage_sensors[voltage_count], &detected_sensors[i], sizeof(SensorInfo));
            voltage_count++;
        }
    }
    
    return voltage_count;
}

int sensors_find_by_name(const char *sensor_name, SensorInfo *sensor_info) {
    if (!sensor_name || !sensor_info) return -1;
    
    if (!initialized) {
        sensors_init();
    }
    
    for (int i = 0; i < sensor_count; i++) {
        if (strstr(detected_sensors[i].name, sensor_name) != NULL ||
            strstr(detected_sensors[i].label, sensor_name) != NULL) {
            memcpy(sensor_info, &detected_sensors[i], sizeof(SensorInfo));
            return 0;
        }
    }
    
    return -1;
}

float sensors_get_average_temperature(void) {
    if (!initialized) {
        sensors_init();
    }
    
    float total_temp = 0.0f;
    int temp_count = 0;
    
    for (int i = 0; i < sensor_count; i++) {
        if (detected_sensors[i].type == SENSOR_TYPE_TEMPERATURE && 
            detected_sensors[i].value > 0 && detected_sensors[i].value < 150) { // Rango razonable
            total_temp += detected_sensors[i].value;
            temp_count++;
        }
    }
    
    if (temp_count == 0) return -1.0f;
    
    return total_temp / temp_count;
}