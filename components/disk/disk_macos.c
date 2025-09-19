/**
 * @file disk_macos.c
 * @brief Implementación de discos para macOS usando IOKit y DiskArbitration
 */

#ifdef __APPLE__

#include "disk_interface.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/IOBSD.h>
#include <DiskArbitration/DiskArbitration.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL disk_initialized = FALSE;
static DiskInfo disk_list[MAX_DISKS];
static int disk_count = 0;

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
 * @brief Obtiene información de discos usando IOKit
 */
static int enumerate_disks_iokit(void) {
    io_iterator_t iterator;
    io_object_t service;
    
    // Buscar medios de almacenamiento
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                   IOServiceMatching("IOMedia"),
                                                   &iterator);
    
    if (kr != KERN_SUCCESS) {
        return -1;
    }
    
    disk_count = 0;
    
    while ((service = IOIteratorNext(iterator)) != 0 && disk_count < MAX_DISKS) {
        CFMutableDictionaryRef properties = NULL;
        
        kr = IORegistryEntryCreateCFProperties(service, &properties,
                                              kCFAllocatorDefault, kNilOptions);
        
        if (kr == KERN_SUCCESS && properties) {
            // Verificar si es un disco completo (no una partición)
            CFBooleanRef is_whole = CFDictionaryGetValue(properties, CFSTR(kIOMediaWholeKey));
            if (is_whole && CFBooleanGetValue(is_whole)) {
                DiskInfo *disk = &disk_list[disk_count];
                memset(disk, 0, sizeof(DiskInfo));
                
                // Nombre del dispositivo BSD
                CFStringRef bsd_name = CFDictionaryGetValue(properties, CFSTR(kIOBSDNameKey));
                if (bsd_name) {
                    cfstring_to_cstring(bsd_name, disk->device, sizeof(disk->device));
                    // Agregar prefijo /dev/
                    char temp[256];
                    snprintf(temp, sizeof(temp), "/dev/%s", disk->device);
                    strncpy(disk->device, temp, sizeof(disk->device) - 1);
                }
                
                // Tamaño del disco
                CFNumberRef size_ref = CFDictionaryGetValue(properties, CFSTR(kIOMediaSizeKey));
                if (size_ref) {
                    int64_t size_bytes;
                    CFNumberGetValue(size_ref, kCFNumberSInt64Type, &size_bytes);
                    disk->size_gb = (float)(size_bytes / (1024.0 * 1024.0 * 1024.0));
                }
                
                // Obtener información del dispositivo padre (controlador de almacenamiento)
                io_registry_entry_t parent;
                kr = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
                if (kr == KERN_SUCCESS) {
                    CFMutableDictionaryRef parent_props = NULL;
                    kr = IORegistryEntryCreateCFProperties(parent, &parent_props,
                                                          kCFAllocatorDefault, kNilOptions);
                    
                    if (kr == KERN_SUCCESS && parent_props) {
                        // Modelo del disco
                        CFStringRef model = CFDictionaryGetValue(parent_props, CFSTR("Model"));
                        if (model) {
                            cfstring_to_cstring(model, disk->model, sizeof(disk->model));
                        }
                        
                        // Tipo de conexión para determinar SSD/HDD
                        CFStringRef protocol = CFDictionaryGetValue(parent_props, CFSTR("Protocol"));
                        if (protocol) {
                            char protocol_str[64];
                            cfstring_to_cstring(protocol, protocol_str, sizeof(protocol_str));
                            
                            if (strstr(protocol_str, "SATA") || strstr(protocol_str, "NVMe") || 
                                strstr(protocol_str, "PCIe")) {
                                // Asumir SSD para conexiones modernas
                                strncpy(disk->type, "SSD", sizeof(disk->type) - 1);
                            } else {
                                strncpy(disk->type, "HDD", sizeof(disk->type) - 1);
                            }
                        } else {
                            strncpy(disk->type, "Unknown", sizeof(disk->type) - 1);
                        }
                        
                        CFRelease(parent_props);
                    }
                    IOObjectRelease(parent);
                }
                
                // Información adicional (valores por defecto)
                disk->temperature = -1.0f; // Requiere SMART
                disk->usage_percent = 0.0f; // Se calculará después
                disk->health_percent = 100.0f; // Asumir saludable
                disk->power_on_hours = 0;
                disk->read_speed_mbps = -1.0f;
                disk->write_speed_mbps = -1.0f;
                
                disk_count++;
            }
            
            CFRelease(properties);
        }
        
        IOObjectRelease(service);
    }
    
    IOObjectRelease(iterator);
    return disk_count > 0 ? 0 : -1;
}

/**
 * @brief Obtiene información de uso de discos montados
 */
static int get_disk_usage_info(void) {
    struct statfs *mounts;
    int mount_count = getmntinfo(&mounts, MNT_NOWAIT);
    
    if (mount_count <= 0) {
        return -1;
    }
    
    // Para cada disco, buscar su punto de montaje
    for (int i = 0; i < disk_count; i++) {
        DiskInfo *disk = &disk_list[i];
        
        // Buscar el punto de montaje correspondiente
        for (int j = 0; j < mount_count; j++) {
            // Comparar nombres de dispositivo (simplificado)
            if (strstr(mounts[j].f_mntfromname, disk->device + 5)) { // Saltar "/dev/"
                struct statvfs vfs;
                if (statvfs(mounts[j].f_mntonname, &vfs) == 0) {
                    uint64_t total_bytes = vfs.f_blocks * vfs.f_frsize;
                    uint64_t free_bytes = vfs.f_bavail * vfs.f_frsize;
                    uint64_t used_bytes = total_bytes - free_bytes;
                    
                    if (total_bytes > 0) {
                        disk->usage_percent = (float)((used_bytes * 100.0) / total_bytes);
                    }
                    
                    // Punto de montaje
                    strncpy(disk->mount_point, mounts[j].f_mntonname, 
                           sizeof(disk->mount_point) - 1);
                }
                break;
            }
        }
    }
    
    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int disk_init(void) {
    if (disk_initialized) {
        return 0;
    }
    
    // Limpiar lista de discos
    memset(disk_list, 0, sizeof(disk_list));
    disk_count = 0;
    
    // Enumerar discos usando IOKit
    if (enumerate_disks_iokit() != 0) {
        return -1;
    }
    
    // Obtener información de uso
    get_disk_usage_info();
    
    disk_initialized = TRUE;
    return 0;
}

void disk_cleanup(void) {
    disk_initialized = FALSE;
    disk_count = 0;
}

int disk_get_count(void) {
    return disk_initialized ? disk_count : -1;
}

int disk_get_info(int disk_id, DiskInfo *info) {
    if (!info || !disk_initialized || disk_id < 0 || disk_id >= disk_count) {
        return -1;
    }
    
    memcpy(info, &disk_list[disk_id], sizeof(DiskInfo));
    return 0;
}

int disk_get_metrics(DiskMetrics *metrics) {
    if (!metrics || !disk_initialized) {
        return -1;
    }
    
    // Actualizar información de uso antes de devolver métricas
    get_disk_usage_info();
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(DiskMetrics));
    
    metrics->num_disks = disk_count;
    
    // Copiar información de todos los discos
    for (int i = 0; i < disk_count && i < MAX_DISKS; i++) {
        memcpy(&metrics->disks[i], &disk_list[i], sizeof(DiskInfo));
    }
    
    return 0;
}

float disk_get_usage_percent(int disk_id) {
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count) {
        return -1.0f;
    }
    
    return disk_list[disk_id].usage_percent;
}

float disk_get_temperature(int disk_id) {
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count) {
        return -1.0f;
    }
    
    // La temperatura SMART en macOS requiere herramientas específicas
    return disk_list[disk_id].temperature;
}

int disk_get_name(int disk_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !disk_initialized || disk_id < 0 || disk_id >= disk_count) {
        return -1;
    }
    
    strncpy(buffer, disk_list[disk_id].model, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float disk_get_health_percent(int disk_id) {
    if (!disk_initialized || disk_id < 0 || disk_id >= disk_count) {
        return -1.0f;
    }
    
    // La salud SMART en macOS requiere herramientas específicas
    return disk_list[disk_id].health_percent;
}

#endif /* __APPLE__ */