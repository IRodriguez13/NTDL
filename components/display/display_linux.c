/**
 * @file display_linux.c
 * @brief Implementación de pantallas para Linux
 */

#include "display_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int display_count = 0;
static DisplayInfo detected_displays[MAX_DISPLAYS];
static Display *x_display = NULL;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int scan_x11_displays(void) {
    display_count = 0;
    
    // Abrir conexión con X11
    x_display = XOpenDisplay(NULL);
    if (!x_display) {
        return -1;
    }
    
    // Verificar extensión XRandR
    int xrandr_event_base, xrandr_error_base;
    if (!XRRQueryExtension(x_display, &xrandr_event_base, &xrandr_error_base)) {
        XCloseDisplay(x_display);
        x_display = NULL;
        return -1;
    }
    
    int screen = DefaultScreen(x_display);
    Window root = RootWindow(x_display, screen);
    
    // Obtener información de pantallas
    XRRScreenResources *screen_resources = XRRGetScreenResources(x_display, root);
    if (!screen_resources) {
        XCloseDisplay(x_display);
        x_display = NULL;
        return -1;
    }
    
    for (int i = 0; i < screen_resources->noutput && display_count < MAX_DISPLAYS; i++) {
        XRROutputInfo *output_info = XRRGetOutputInfo(x_display, screen_resources, screen_resources->outputs[i]);
        if (!output_info) continue;
        
        // Solo procesar pantallas conectadas
        if (output_info->connection != RR_Connected) {
            XRRFreeOutputInfo(output_info);
            continue;
        }
        
        DisplayInfo *display = &detected_displays[display_count];
        memset(display, 0, sizeof(DisplayInfo));
        
        // Nombre de la pantalla
        strncpy(display->name, output_info->name, sizeof(display->name) - 1);
        display->name[sizeof(display->name) - 1] = '\0';
        
        // Tamaño físico
        display->width_mm = output_info->mm_width;
        display->height_mm = output_info->mm_height;
        
        // Estado de conexión
        display->is_connected = 1;
        
        // Obtener información del CRTC si está activo
        if (output_info->crtc != None) {
            XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(x_display, screen_resources, output_info->crtc);
            if (crtc_info) {
                display->width_pixels = crtc_info->width;
                display->height_pixels = crtc_info->height;
                
                // Calcular DPI
                if (display->width_mm > 0 && display->height_mm > 0) {
                    display->dpi_x = (display->width_pixels * 25.4f) / display->width_mm;
                    display->dpi_y = (display->height_pixels * 25.4f) / display->height_mm;
                }
                
                // Obtener información del modo actual
                for (int j = 0; j < screen_resources->nmode; j++) {
                    if (screen_resources->modes[j].id == crtc_info->mode) {
                        XRRModeInfo *mode = &screen_resources->modes[j];
                        if (mode->hTotal && mode->vTotal) {
                            display->refresh_rate_hz = (float)mode->dotClock / 
                                                     (mode->hTotal * mode->vTotal);
                        }
                        break;
                    }
                }
                
                XRRFreeCrtcInfo(crtc_info);
            }
        }
        
        // Determinar si es pantalla principal (primera activa)
        display->is_primary = (display_count == 0) ? 1 : 0;
        
        // Profundidad de color (por defecto)
        display->color_depth_bits = DefaultDepth(x_display, screen);
        
        // Tipo de conexión (simplificado)
        if (strstr(display->name, "HDMI")) {
            strncpy(display->connection_type, "HDMI", sizeof(display->connection_type) - 1);
        } else if (strstr(display->name, "DP") || strstr(display->name, "DisplayPort")) {
            strncpy(display->connection_type, "DisplayPort", sizeof(display->connection_type) - 1);
        } else if (strstr(display->name, "VGA")) {
            strncpy(display->connection_type, "VGA", sizeof(display->connection_type) - 1);
        } else if (strstr(display->name, "DVI")) {
            strncpy(display->connection_type, "DVI", sizeof(display->connection_type) - 1);
        } else {
            strncpy(display->connection_type, "Unknown", sizeof(display->connection_type) - 1);
        }
        display->connection_type[sizeof(display->connection_type) - 1] = '\0';
        
        XRRFreeOutputInfo(output_info);
        display_count++;
    }
    
    XRRFreeScreenResources(screen_resources);
    return display_count;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int display_init(void) {
    if (initialized) return 0;
    
    // Escanear pantallas X11
    if (scan_x11_displays() < 0) {
        return -1;
    }
    
    initialized = 1;
    return 0;
}

void display_cleanup(void) {
    if (x_display) {
        XCloseDisplay(x_display);
        x_display = NULL;
    }
    initialized = 0;
    display_count = 0;
}

int display_get_count(void) {
    if (!initialized) {
        display_init();
    }
    return display_count;
}

int display_get_info(int display_id, DisplayInfo *display_info) {
    if (!display_info || display_id < 0 || display_id >= display_count) {
        return -1;
    }
    
    if (!initialized) {
        display_init();
    }
    
    memcpy(display_info, &detected_displays[display_id], sizeof(DisplayInfo));
    return 0;
}

int display_get_primary_info(DisplayInfo *display_info) {
    return display_get_info(0, display_info);
}

int display_get_desktop_resolution(int *total_width, int *total_height) {
    if (!total_width || !total_height) return -1;
    
    if (!initialized) {
        display_init();
    }
    
    if (!x_display) return -1;
    
    int screen = DefaultScreen(x_display);
    *total_width = DisplayWidth(x_display, screen);
    *total_height = DisplayHeight(x_display, screen);
    
    return 0;
}