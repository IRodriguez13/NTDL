/**
 * @file display_macos.c
 * @brief Implementación de display para macOS usando CoreGraphics y IOKit
 */

#ifdef __APPLE__

#include "display_interface.h"
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/IOKitLib.h>
#include <ApplicationServices/ApplicationServices.h>
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
 * @brief Obtiene información adicional del display usando IOKit
 */
static void get_display_iokit_info(CGDirectDisplayID display_id, DisplayInfo *display_info) {
    io_service_t service = CGDisplayIOServicePort(display_id);
    if (service == MACH_PORT_NULL) {
        return;
    }
    
    CFMutableDictionaryRef properties = NULL;
    kern_return_t kr = IORegistryEntryCreateCFProperties(service, &properties,
                                                        kCFAllocatorDefault, kNilOptions);
    
    if (kr == KERN_SUCCESS && properties) {
        // Nombre del display
        CFStringRef display_name = CFDictionaryGetValue(properties, CFSTR(kDisplayProductName));
        if (display_name) {
            cfstring_to_cstring(display_name, display_info->description, 
                               sizeof(display_info->description));
        }
        
        // Información del fabricante
        CFNumberRef vendor_id = CFDictionaryGetValue(properties, CFSTR(kDisplayVendorID));
        if (vendor_id) {
            int vendor;
            CFNumberGetValue(vendor_id, kCFNumberIntType, &vendor);
            
            // Mapear algunos vendor IDs conocidos
            switch (vendor) {
                case 0x1e6d: // LG
                    strncpy(display_info->name, "LG Display", sizeof(display_info->name) - 1);
                    break;
                case 0x4d10: // Sharp
                    strncpy(display_info->name, "Sharp Display", sizeof(display_info->name) - 1);
                    break;
                case 0x06b3: // Apple
                    strncpy(display_info->name, "Apple Display", sizeof(display_info->name) - 1);
                    break;
                default:
                    snprintf(display_info->name, sizeof(display_info->name), 
                            "Display (0x%04x)", vendor);
                    break;
            }
        }
        
        // Tamaño físico (si está disponible)
        CFNumberRef width_mm = CFDictionaryGetValue(properties, CFSTR(kDisplayHorizontalImageSize));
        CFNumberRef height_mm = CFDictionaryGetValue(properties, CFSTR(kDisplayVerticalImageSize));
        
        if (width_mm && height_mm) {
            int w_mm, h_mm;
            CFNumberGetValue(width_mm, kCFNumberIntType, &w_mm);
            CFNumberGetValue(height_mm, kCFNumberIntType, &h_mm);
            
            display_info->width_mm = (float)w_mm;
            display_info->height_mm = (float)h_mm;
            
            // Calcular diagonal en pulgadas
            if (w_mm > 0 && h_mm > 0) {
                float diagonal_mm = sqrtf(w_mm * w_mm + h_mm * h_mm);
                display_info->diagonal_inches = diagonal_mm / 25.4f;
            }
        }
        
        CFRelease(properties);
    }
}

/**
 * @brief Enumera displays usando CoreGraphics
 */
static int enumerate_displays_coregraphics(void) {
    CGDirectDisplayID displays[MAX_DISPLAYS];
    uint32_t display_count_cg;
    
    // Obtener lista de displays activos
    CGError error = CGGetActiveDisplayList(MAX_DISPLAYS, displays, &display_count_cg);
    if (error != kCGErrorSuccess) {
        return -1;
    }
    
    display_count = (int)display_count_cg;
    
    for (int i = 0; i < display_count; i++) {
        DisplayInfo *display_info = &display_list[i];
        memset(display_info, 0, sizeof(DisplayInfo));
        
        CGDirectDisplayID display_id = displays[i];
        
        // Información básica del display
        snprintf(display_info->name, sizeof(display_info->name), "Display %d", i);
        
        // Resolución
        display_info->width = (int)CGDisplayPixelsWide(display_id);
        display_info->height = (int)CGDisplayPixelsHigh(display_id);
        
        // Posición
        CGRect bounds = CGDisplayBounds(display_id);
        display_info->x = (int)bounds.origin.x;
        display_info->y = (int)bounds.origin.y;
        
        // Display principal
        display_info->is_primary = (CGDisplayIsMain(display_id)) ? 1 : 0;
        
        // Estado
        display_info->is_connected = 1; // Si aparece en la lista, está conectado
        display_info->is_enabled = (CGDisplayIsActive(display_id)) ? 1 : 0;
        
        // Modo de display actual
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display_id);
        if (mode) {
            // Frecuencia de actualización
            display_info->refresh_rate = (float)CGDisplayModeGetRefreshRate(mode);
            if (display_info->refresh_rate == 0) {
                display_info->refresh_rate = 60.0f; // Valor por defecto para displays sin frecuencia variable
            }
            
            // Profundidad de color (aproximada)
            display_info->color_depth = 32; // Valor típico para macOS
            
            CGDisplayModeRelease(mode);
        }
        
        // Rotación (siempre 0 en macOS por defecto)
        display_info->rotation = 0;
        
        // Brillo (requiere APIs privadas, usar valor por defecto)
        display_info->brightness = -1.0f;
        display_info->contrast = -1.0f;
        display_info->gamma = -1.0f;
        
        // DPI
        CGSize size_mm = CGDisplayScreenSize(display_id);
        if (size_mm.width > 0 && size_mm.height > 0) {
            display_info->width_mm = size_mm.width;
            display_info->height_mm = size_mm.height;
            
            // Calcular DPI
            display_info->dpi_x = (display_info->width * 25.4f) / size_mm.width;
            display_info->dpi_y = (display_info->height * 25.4f) / size_mm.height;
            
            // Diagonal en pulgadas
            float diagonal_mm = sqrtf(size_mm.width * size_mm.width + size_mm.height * size_mm.height);
            display_info->diagonal_inches = diagonal_mm / 25.4f;
        } else {
            // Valores por defecto si no se puede obtener el tamaño físico
            display_info->dpi_x = 72.0f; // DPI típico de macOS
            display_info->dpi_y = 72.0f;
            display_info->width_mm = -1.0f;
            display_info->height_mm = -1.0f;
            display_info->diagonal_inches = -1.0f;
        }
        
        // Obtener información adicional usando IOKit
        get_display_iokit_info(display_id, display_info);
        
        // Si no se obtuvo descripción, usar nombre genérico
        if (strlen(display_info->description) == 0) {
            snprintf(display_info->description, sizeof(display_info->description),
                    "%dx%d Display", display_info->width, display_info->height);
        }
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
    
    // Enumerar displays usando CoreGraphics
    if (enumerate_displays_coregraphics() != 0) {
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
    
    // El brillo en macOS requiere APIs privadas o herramientas específicas
    return display_list[display_id].brightness;
}

#endif /* __APPLE__ */