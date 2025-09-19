/**
 * @file sensors_macos.c
 * @brief Implementación de sensores para macOS usando IOKit
 */

#ifdef __APPLE__

#include "sensors_interface.h"
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL sensors_initialized = FALSE;
static SensorInfo sensor_list[MAX_SENSORS];
static int sensor_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Convierte CFString a C string
 */
static void cfstring_to_cstring(CFStringRef cfstr, char *buffer, size_t buffer_size) {
    if (cfstr && buffer && buffer_size > 0) {
        CFStringGetCString(cfstr, buffer, buffer_size, kCFStringEncodingUTF8);
    } else if (buffer && buffer_size > 0) {
        buffer[0] = '\0';
    }
}

/**
 * @brief Obtiene valor numérico de una propiedad CFNumber
 */
static float get_cf_number_float_value(CFNumberRef number) {
    float value = 0.0f;
    if (number && CFGetTypeID(number) == CFNumberGetTypeID()) {
        CFNumberGetValue(number, kCFNumberFloatType, &value);
    }
    return value;
}

/**
 * @brief Determina el tipo de sensor basado en su clave
 */
static SensorType determine_sensor_type(const char *sensor_key) {
    if (strstr(sensor_key, "temp") || strstr(sensor_key, "Temp") || 
        strstr(sensor_key, "TEMP") || strstr(sensor_key, "thermal")) {
        return SENSOR_TYPE_TEMPERATURE;
    } else if (strstr(sensor_key, "fan") || strstr(sensor_key, "Fan") || 
               strstr(sensor_key, "FAN")) {
        return SENSOR_TYPE_FAN;
    } else if (strstr(sensor_key, "volt") || strstr(sensor_key, "Volt") || 
               strstr(sensor_key, "VOLT")) {
        return SENSOR_TYPE_VOLTAGE;
    } else if (strstr(sensor_key, "power") || strstr(sensor_key, "Power") || 
               strstr(sensor_key, "POWER")) {
        return SENSOR_TYPE_POWER;
    } else {
        return SENSOR_TYPE_UNKNOWN;
    }
}

/**
 * @brief Enumera sensores usando IOKit (IOHWSensor)
 */
static int enumerate_sensors_iokit(void) {
    io_iterator_t iterator;
    io_object_t service;
    
    // Buscar sensores de hardware
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                   IOServiceMatching("IOHWSensor"),
                                                   &iterator);
    
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    
    sensor_count = 0;
    
    while ((service = IOIteratorNext(iterator)) != 0 && sensor_count < MAX_SENSORS) {
        CFMutableDictionaryRef properties = NULL;
        
        kr = IORegistryEntryCreateCFProperties(service, &properties,
                                              kCFAllocatorDefault, kNilOptions);
        
        if (kr == KERN_SUCCESS && properties) {
            SensorInfo *sensor = &sensor_list[sensor_count];
            memset(sensor, 0, sizeof(SensorInfo));
            
            // Clave del sensor
            CFStringRef sensor_key = CFDictionaryGetValue(properties, CFSTR("sensor-key"));
            char key_str[256] = {0};
            if (sensor_key) {
                cfstring_to_cstring(sensor_key, key_str, sizeof(key_str));
                strncpy(sensor->name, key_str, sizeof(sensor->name) - 1);
            }
            
            // Tipo de sensor
            sensor->type = determine_sensor_type(key_str);
            
            // Valor actual
            CFNumberRef current_value = CFDictionaryGetValue(properties, CFSTR("current-value"));
            if (current_value) {
                SInt32 raw_value;
                CFNumberGetValue(current_value, kCFNumberSInt32Type, &raw_value);
                
                // Los valores suelen venir en formato fixed-point
                if (sensor->type == SENSOR_TYPE_TEMPERATURE) {
                    sensor->value = raw_value / 65536.0f; // Temperatura en °C
                    strncpy(sensor->label, "Temperature", sizeof(sensor->label) - 1);
                    sensor->min_value = 0.0f;
                    sensor->max_value = 100.0f;
                    sensor->critical_value = 85.0f;
                } else if (sensor->type == SENSOR_TYPE_FAN) {
                    sensor->value = (float)raw_value; // RPM
                    strncpy(sensor->label, "Fan Speed", sizeof(sensor->label) - 1);
                    sensor->min_value = 0.0f;
                    sensor->max_value = 6000.0f;
                    sensor->critical_value = 100.0f; // RPM mínimo crítico
                } else if (sensor->type == SENSOR_TYPE_VOLTAGE) {
                    sensor->value = raw_value / 65536.0f; // Voltios
                    strncpy(sensor->label, "Voltage", sizeof(sensor->label) - 1);
                    sensor->min_value = 0.0f;
                    sensor->max_value = 20.0f;
                    sensor->critical_value = 15.0f;
                } else {
                    sensor->value = (float)raw_value;
                    strncpy(sensor->label, "Unknown", sizeof(sensor->label) - 1);
                    sensor->min_value = 0.0f;
                    sensor->max_value = 1000.0f;
                    sensor->critical_value = 900.0f;
                }
            }
            
            // Valores mínimo y máximo (si están disponibles)
            CFNumberRef min_value = CFDictionaryGetValue(properties, CFSTR("min-value"));
            if (min_value) {
                SInt32 raw_min;
                CFNumberGetValue(min_value, kCFNumberSInt32Type, &raw_min);
                if (sensor->type == SENSOR_TYPE_TEMPERATURE) {
                    sensor->min_value = raw_min / 65536.0f;
                } else {
                    sensor->min_value = (float)raw_min;
                }
            }
            
            CFNumberRef max_value = CFDictionaryGetValue(properties, CFSTR("max-value"));
            if (max_value) {
                SInt32 raw_max;
                CFNumberGetValue(max_value, kCFNumberSInt32Type, &raw_max);
                if (sensor->type == SENSOR_TYPE_TEMPERATURE) {
                    sensor->max_value = raw_max / 65536.0f;
                } else {
                    sensor->max_value = (float)raw_max;
                }
            }
            
            sensor_count++;
            CFRelease(properties);
        }
        
        IOObjectRelease(service);
    }
    
    IOObjectRelease(iterator);
    return 0;
}

/**
 * @brief Enumera sensores usando AppleSMC (si está disponible)
 */
static int enumerate_sensors_smc(void) {
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                      IOServiceMatching("AppleSMC"));
    
    if (service == 0) {
        return -1; // SMC no disponible
    }
    
    io_connect_t connection;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connection);
    IOObjectRelease(service);
    
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    
    // Aquí se podría implementar lectura directa del SMC
    // Por simplicidad, cerramos la conexión
    IOServiceClose(connection);
    
    return 0;
}

/**
 * @brief Agrega sensores simulados si no se encuentran sensores reales
 */
static void add_simulated_sensors(void) {
    if (sensor_count == 0) {
        // Agregar sensor de CPU simulado
        SensorInfo *cpu_sensor = &sensor_list[sensor_count++];
        memset(cpu_sensor, 0, sizeof(SensorInfo));
        cpu_sensor->type = SENSOR_TYPE_TEMPERATURE;
        strncpy(cpu_sensor->name, "CPU Temperature", sizeof(cpu_sensor->name) - 1);
        strncpy(cpu_sensor->label, "CPU Temp", sizeof(cpu_sensor->label) - 1);
        cpu_sensor->value = 50.0f; // Valor simulado
        cpu_sensor->min_value = 20.0f;
        cpu_sensor->max_value = 100.0f;
        cpu_sensor->critical_value = 85.0f;
        
        // Agregar sensor de GPU simulado (para Apple Silicon)
        if (sensor_count < MAX_SENSORS) {
            SensorInfo *gpu_sensor = &sensor_list[sensor_count++];
            memset(gpu_sensor, 0, sizeof(SensorInfo));
            gpu_sensor->type = SENSOR_TYPE_TEMPERATURE;
            strncpy(gpu_sensor->name, "GPU Temperature", sizeof(gpu_sensor->name) - 1);
            strncpy(gpu_sensor->label, "GPU Temp", sizeof(gpu_sensor->label) - 1);
            gpu_sensor->value = 45.0f; // Valor simulado
            gpu_sensor->min_value = 20.0f;
            gpu_sensor->max_value = 90.0f;
            gpu_sensor->critical_value = 80.0f;
        }
        
        // Agregar sensor de ventilador simulado
        if (sensor_count < MAX_SENSORS) {
            SensorInfo *fan_sensor = &sensor_list[sensor_count++];
            memset(fan_sensor, 0, sizeof(SensorInfo));
            fan_sensor->type = SENSOR_TYPE_FAN;
            strncpy(fan_sensor->name, "System Fan", sizeof(fan_sensor->name) - 1);
            strncpy(fan_sensor->label, "Fan Speed", sizeof(fan_sensor->label) - 1);
            fan_sensor->value = 2000.0f; // RPM simulado
            fan_sensor->min_value = 0.0f;
            fan_sensor->max_value = 6000.0f;
            fan_sensor->critical_value = 100.0f;
        }
    }
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int sensors_init(void) {
    if (sensors_initialized) {
        return 0;
    }
    
    // Limpiar lista de sensores
    memset(sensor_list, 0, sizeof(sensor_list));
    sensor_count = 0;
    
    // Intentar enumerar sensores usando IOKit
    enumerate_sensors_iokit();
    
    // Intentar enumerar sensores usando SMC
    enumerate_sensors_smc();
    
    // Si no se encontraron sensores, agregar algunos simulados
    add_simulated_sensors();
    
    sensors_initialized = TRUE;
    return 0;
}

void sensors_cleanup(void) {
    sensors_initialized = FALSE;
    sensor_count = 0;
}

int sensors_get_count(void) {
    return sensors_initialized ? sensor_count : -1;
}

int sensors_get_info(int sensor_id, SensorInfo *info) {
    if (!info || !sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count) {
        return -1;
    }
    
    memcpy(info, &sensor_list[sensor_id], sizeof(SensorInfo));
    return 0;
}

int sensors_get_metrics(SensorMetrics *metrics) {
    if (!metrics || !sensors_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(SensorMetrics));
    
    metrics->num_sensors = sensor_count;
    
    // Copiar información de todos los sensores
    for (int i = 0; i < sensor_count && i < MAX_SENSORS; i++) {
        memcpy(&metrics->sensors[i], &sensor_list[i], sizeof(SensorInfo));
    }
    
    return 0;
}

float sensors_get_value(int sensor_id) {
    if (!sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count) {
        return -1.0f;
    }
    
    return sensor_list[sensor_id].value;
}

int sensors_get_name(int sensor_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !sensors_initialized || 
        sensor_id < 0 || sensor_id >= sensor_count) {
        return -1;
    }
    
    strncpy(buffer, sensor_list[sensor_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

SensorType sensors_get_type(int sensor_id) {
    if (!sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count) {
        return SENSOR_TYPE_UNKNOWN;
    }
    
    return sensor_list[sensor_id].type;
}

#endif /* __APPLE__ */