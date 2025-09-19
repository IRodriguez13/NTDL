/**
 * @file battery_macos.c
 * @brief Implementación de batería para macOS usando IOPowerSources
 */

#ifdef __APPLE__

#include "battery_interface.h"
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL battery_initialized = FALSE;

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
 * @brief Obtiene valor entero de CFNumber
 */
static int get_cf_number_int_value(CFNumberRef number) {
    int value = 0;
    if (number && CFGetTypeID(number) == CFNumberGetTypeID()) {
        CFNumberGetValue(number, kCFNumberIntType, &value);
    }
    return value;
}

/**
 * @brief Obtiene valor booleano de CFBoolean
 */
static int get_cf_boolean_value(CFBooleanRef boolean) {
    if (boolean && CFGetTypeID(boolean) == CFBooleanGetTypeID()) {
        return CFBooleanGetValue(boolean) ? 1 : 0;
    }
    return 0;
}

/**
 * @brief Obtiene información de la batería usando IOPowerSources
 */
static int get_battery_info(BatteryMetrics *metrics) {
    CFTypeRef power_sources_info = IOPSCopyPowerSourcesInfo();
    if (!power_sources_info) {
        return -1;
    }
    
    CFArrayRef power_sources_list = IOPSCopyPowerSourcesList(power_sources_info);
    if (!power_sources_list) {
        CFRelease(power_sources_info);
        return -1;
    }
    
    CFIndex count = CFArrayGetCount(power_sources_list);
    int battery_found = 0;
    
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef power_source = CFArrayGetValueAtIndex(power_sources_list, i);
        CFDictionaryRef description = IOPSGetPowerSourceDescription(power_sources_info, power_source);
        
        if (!description) continue;
        
        // Verificar si es una batería
        CFStringRef transport_type = CFDictionaryGetValue(description, CFSTR(kIOPSTransportTypeKey));
        if (transport_type) {
            char transport_str[64];
            cfstring_to_cstring(transport_type, transport_str, sizeof(transport_str));
            
            if (strcmp(transport_str, "Internal") == 0) {
                battery_found = 1;
                metrics->is_present = 1;
                
                // Nivel de carga
                CFNumberRef current_capacity = CFDictionaryGetValue(description, CFSTR(kIOPSCurrentCapacityKey));
                CFNumberRef max_capacity = CFDictionaryGetValue(description, CFSTR(kIOPSMaxCapacityKey));
                
                if (current_capacity && max_capacity) {
                    int current = get_cf_number_int_value(current_capacity);
                    int max = get_cf_number_int_value(max_capacity);
                    
                    if (max > 0) {
                        metrics->charge_percent = (float)((current * 100.0) / max);
                    }
                }
                
                // Estado de carga
                CFStringRef power_source_state = CFDictionaryGetValue(description, CFSTR(kIOPSPowerSourceStateKey));
                if (power_source_state) {
                    char state_str[64];
                    cfstring_to_cstring(power_source_state, state_str, sizeof(state_str));
                    strncpy(metrics->status, state_str, sizeof(metrics->status) - 1);
                    
                    if (strcmp(state_str, "AC Power") == 0) {
                        metrics->is_charging = 0;
                        strncpy(metrics->status, "Not Charging", sizeof(metrics->status) - 1);
                    } else if (strcmp(state_str, "Battery Power") == 0) {
                        metrics->is_charging = 0;
                        strncpy(metrics->status, "Discharging", sizeof(metrics->status) - 1);
                    }
                }
                
                // Verificar si está cargando
                CFBooleanRef is_charging = CFDictionaryGetValue(description, CFSTR(kIOPSIsChargingKey));
                if (is_charging) {
                    metrics->is_charging = get_cf_boolean_value(is_charging);
                    if (metrics->is_charging) {
                        strncpy(metrics->status, "Charging", sizeof(metrics->status) - 1);
                    }
                }
                
                // Tiempo restante
                CFNumberRef time_to_empty = CFDictionaryGetValue(description, CFSTR(kIOPSTimeToEmptyKey));
                if (time_to_empty) {
                    metrics->time_remaining_minutes = get_cf_number_int_value(time_to_empty);
                }
                
                // Voltaje
                CFNumberRef voltage = CFDictionaryGetValue(description, CFSTR(kIOPSVoltageKey));
                if (voltage) {
                    metrics->voltage = (float)(get_cf_number_int_value(voltage) / 1000.0); // mV a V
                }
                
                // Corriente
                CFNumberRef amperage = CFDictionaryGetValue(description, CFSTR(kIOPSAmperageKey));
                if (amperage) {
                    metrics->current_ma = (float)get_cf_number_int_value(amperage);
                }
                
                // Potencia
                if (metrics->voltage > 0 && metrics->current_ma != 0) {
                    metrics->power_watts = (metrics->voltage * metrics->current_ma) / 1000.0f;
                }
                
                // Capacidad
                CFNumberRef design_capacity = CFDictionaryGetValue(description, CFSTR("DesignCapacity"));
                if (design_capacity) {
                    metrics->design_capacity_mah = (float)get_cf_number_int_value(design_capacity);
                }
                
                if (max_capacity) {
                    metrics->capacity_mah = (float)get_cf_number_int_value(max_capacity);
                }
                
                // Número de ciclos
                CFNumberRef cycle_count = CFDictionaryGetValue(description, CFSTR("CycleCount"));
                if (cycle_count) {
                    metrics->cycle_count = get_cf_number_int_value(cycle_count);
                }
                
                // Temperatura (si está disponible)
                CFNumberRef temperature = CFDictionaryGetValue(description, CFSTR("Temperature"));
                if (temperature) {
                    metrics->temperature = (float)(get_cf_number_int_value(temperature) / 100.0); // Centígrados
                }
                
                // Salud de la batería
                if (metrics->design_capacity_mah > 0 && metrics->capacity_mah > 0) {
                    float health_percent = (metrics->capacity_mah / metrics->design_capacity_mah) * 100.0f;
                    if (health_percent > 90.0f) {
                        strncpy(metrics->health, "Good", sizeof(metrics->health) - 1);
                    } else if (health_percent > 70.0f) {
                        strncpy(metrics->health, "Fair", sizeof(metrics->health) - 1);
                    } else {
                        strncpy(metrics->health, "Poor", sizeof(metrics->health) - 1);
                    }
                } else {
                    strncpy(metrics->health, "Unknown", sizeof(metrics->health) - 1);
                }
                
                // Tecnología (asumir Li-ion para macOS)
                strncpy(metrics->technology, "Li-ion", sizeof(metrics->technology) - 1);
                
                break; // Solo procesar la primera batería encontrada
            }
        }
    }
    
    CFRelease(power_sources_list);
    CFRelease(power_sources_info);
    
    return battery_found ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int battery_init(void) {
    battery_initialized = TRUE;
    return 0;
}

void battery_cleanup(void) {
    battery_initialized = FALSE;
}

int battery_get_metrics(BatteryMetrics *metrics) {
    if (!metrics || !battery_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(BatteryMetrics));
    
    // Obtener información de la batería
    return get_battery_info(metrics);
}

int battery_is_present(void) {
    if (!battery_initialized) {
        return -1;
    }
    
    BatteryMetrics metrics;
    if (get_battery_info(&metrics) == 0) {
        return metrics.is_present ? 1 : 0;
    }
    
    return 0; // Asumir que no hay batería si no se puede obtener información
}

float battery_get_charge_percent(void) {
    if (!battery_initialized) {
        return -1.0f;
    }
    
    BatteryMetrics metrics;
    if (get_battery_info(&metrics) == 0) {
        return metrics.charge_percent;
    }
    
    return -1.0f;
}

int battery_is_charging(void) {
    if (!battery_initialized) {
        return -1;
    }
    
    BatteryMetrics metrics;
    if (get_battery_info(&metrics) == 0) {
        return metrics.is_charging ? 1 : 0;
    }
    
    return -1;
}

int battery_get_time_remaining_minutes(void) {
    if (!battery_initialized) {
        return -1;
    }
    
    BatteryMetrics metrics;
    if (get_battery_info(&metrics) == 0) {
        return metrics.time_remaining_minutes;
    }
    
    return -1;
}

int battery_get_status(char *buffer, size_t size) {
    if (!buffer || size == 0 || !battery_initialized) {
        return -1;
    }
    
    BatteryMetrics metrics;
    if (get_battery_info(&metrics) == 0) {
        strncpy(buffer, metrics.status, size - 1);
        buffer[size - 1] = '\0';
        return 0;
    }
    
    strncpy(buffer, "Unknown", size - 1);
    buffer[size - 1] = '\0';
    return -1;
}

#endif /* __APPLE__ */