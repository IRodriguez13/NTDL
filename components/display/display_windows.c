/**
 * @file display_windows.c
 * @brief Implementación de display para Windows usando EnumDisplayDevices y APIs del sistema
 */

#ifdef _WIN32

#include "display_interface.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL display_initialized = FALSE;
static DisplayInfo display_list[MAX_DISPLAYS];
static int display_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Callback para enumerar monitores
 */
static BOOL CALLBACK monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    if (display_count >= MAX_DISPLAYS) {
        return FALSE; // Detener enumeración
    }
    
    MONITORINFOEX monitor_info;
    monitor_info.cbSize = sizeof(MONITORINFOEX);
    
    if (GetMonitorInfo(hMonitor, (LPMONITORINFO)&monitor_info)) {
        DisplayInfo *display = &display_list[display_count];
        memset(display, 0, sizeof(DisplayInfo));
        
        // Nombre del dispositivo
        strncpy(display->name, monitor_info.szDevice, sizeof(display->name) - 1);
        
        // Resolución
        display->width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
        display->height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
        
        // Posición
        display->x = monitor_info.rcMonitor.left;
        display->y = monitor_info.rcMonitor.top;
        
        // Monitor primario
        display->is_primary = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) ? 1 : 0;
        
        // Estado (asumir conectado si aparece en la enumeración)
        display->is_connected = 1;
        display->is_enabled = 1;
        
        // Obtener información adicional del dispositivo
        DISPLAY_DEVICE display_device;
        display_device.cb = sizeof(DISPLAY_DEVICE);
        
        if (EnumDisplayDevices(monitor_info.szDevice, 0, &display_device, 0)) {
            // Descripción del monitor
            strncpy(display->description, display_device.DeviceString, sizeof(display->description) - 1);
            
            // Estado del dispositivo
            display->is_enabled = (display_device.StateFlags & DISPLAY_DEVICE_ACTIVE) ? 1 : 0;
        }
        
        // Obtener configuración del modo de display
        DEVMODE dev_mode;
        dev_mode.dmSize = sizeof(DEVMODE);
        
        if (EnumDisplaySettings(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode)) {
            // Frecuencia de actualización
            display->refresh_rate = (float)dev_mode.dmDisplayFrequency;
            
            // Profundidad de color
            display->color_depth = dev_mode.dmBitsPerPel;
            
            // Orientación
            switch (dev_mode.dmDisplayOrientation) {
                case DMDO_DEFAULT:
                    display->rotation = 0;
                    break;
                case DMDO_90:
                    display->rotation = 90;
                    break;
                case DMDO_180:
                    display->rotation = 180;
                    break;
                case DMDO_270:
                    display->rotation = 270;
                    break;
                default:
                    display->rotation = 0;
                    break;
            }
        } else {
            // Valores por defecto
            display->refresh_rate = 60.0f;
            display->color_depth = 32;
            display->rotation = 0;
        }
        
        // Información adicional (no disponible fácilmente)
        display->brightness = -1.0f; // Requiere APIs específicas
        display->contrast = -1.0f;
        display->gamma = -1.0f;
        
        // Tamaño físico (no disponible fácilmente en Windows)
        display->width_mm = -1.0f;
        display->height_mm = -1.0f;
        display->diagonal_inches = -1.0f;
        
        // Calcular DPI aproximado (asumiendo tamaño estándar)
        // Esto es una aproximación, el DPI real requiere información del hardware
        HDC hdc = GetDC(NULL);
        if (hdc) {
            display->dpi_x = (float)GetDeviceCaps(hdc, LOGPIXELSX);
            display->dpi_y = (float)GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        } else {
            display->dpi_x = 96.0f; // DPI estándar de Windows
            display->dpi_y = 96.0f;
        }
        
        display_count++;
    }
    
    return TRUE; // Continuar enumeración
}

/**
 * @brief Enumera todos los displays del sistema
 */
static int enumerate_displays(void) {
    display_count = 0;
    
    // Enumerar monitores usando EnumDisplayMonitors
    if (!EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, 0)) {
        return -1;
    }
    
    return display_count > 0 ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int display_init(void) {
    if (display_initialized) {
        return 0;
    }
    
    // Limpiar lista de displays
    memset(display_list, 0, sizeof(display_list));
    display_count = 0;
    
    // Enumerar displays
    if (enumerate_displays() != 0) {
        return -1;
    }
    
    display_initialized = TRUE;
    return 0;
}

void display_cleanup(void) {
    display_initialized = FALSE;
    display_count = 0;
}

int display_get_count(void) {
    return display_initialized ? display_count : -1;
}

int display_get_info(int display_id, DisplayInfo *info) {
    if (!info || !display_initialized || display_id < 0 || display_id >= display_count) {
        return -1;
    }
    
    memcpy(info, &display_list[display_id], sizeof(DisplayInfo));
    return 0;
}

int display_get_metrics(DisplayMetrics *metrics) {
    if (!metrics || !display_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(DisplayMetrics));
    
    metrics->num_displays = display_count;
    
    // Copiar información de todos los displays
    for (int i = 0; i < display_count && i < MAX_DISPLAYS; i++) {
        memcpy(&metrics->displays[i], &display_list[i], sizeof(DisplayInfo));
    }
    
    return 0;
}

int display_get_resolution(int display_id, int *width, int *height) {
    if (!width || !height || !display_initialized || 
        display_id < 0 || display_id >= display_count) {
        return -1;
    }
    
    *width = display_list[display_id].width;
    *height = display_list[display_id].height;
    return 0;
}

float display_get_refresh_rate(int display_id) {
    if (!display_initialized || display_id < 0 || display_id >= display_count) {
        return -1.0f;
    }
    
    return display_list[display_id].refresh_rate;
}

int display_get_name(int display_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !display_initialized || 
        display_id < 0 || display_id >= display_count) {
        return -1;
    }
    
    strncpy(buffer, display_list[display_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

int display_is_primary(int display_id) {
    if (!display_initialized || display_id < 0 || display_id >= display_count) {
        return -1;
    }
    
    return display_list[display_id].is_primary ? 1 : 0;
}

float display_get_brightness(int display_id) {
    if (!display_initialized || display_id < 0 || display_id >= display_count) {
        return -1.0f;
    }
    
    // El brillo en Windows requiere APIs específicas como WMI o DDC/CI
    // Por simplicidad, retornamos el valor cached
    return display_list[display_id].brightness;
}

#endif /* _WIN32 */