#include "../metrics.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

// Variables globales para calcular uso de CPU
static natural_t last_cpu_count = 0;
static processor_info_array_t last_cpu_info = NULL;
static mach_msg_type_number_t last_num_cpu_info = 0;

float get_cpu_usage_xnu() {
    kern_return_t kr;
    natural_t num_cpu_info = 0;
    processor_info_array_t cpu_info = NULL;
    mach_msg_type_number_t num_cpu_info = 0;
    
    kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, 
                            &num_cpu_info, &cpu_info, &num_cpu_info);
    if (kr != KERN_SUCCESS) {
        return 0.0f;
    }
    
    float total_cpu_usage = 0.0f;
    
    for (natural_t i = 0; i < num_cpu_info; i++) {
        processor_cpu_load_info_t cpu_load_info = (processor_cpu_load_info_t)cpu_info;
        
        if (last_cpu_info != NULL) {
            unsigned long total = cpu_load_info[i].cpu_ticks[CPU_STATE_USER] +
                                cpu_load_info[i].cpu_ticks[CPU_STATE_SYSTEM] +
                                cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] +
                                cpu_load_info[i].cpu_ticks[CPU_STATE_NICE];
            
            unsigned long last_total = last_cpu_info[i].cpu_ticks[CPU_STATE_USER] +
                                     last_cpu_info[i].cpu_ticks[CPU_STATE_SYSTEM] +
                                     last_cpu_info[i].cpu_ticks[CPU_STATE_IDLE] +
                                     last_cpu_info[i].cpu_ticks[CPU_STATE_NICE];
            
            unsigned long total_diff = total - last_total;
            unsigned long idle_diff = cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] - 
                                    last_cpu_info[i].cpu_ticks[CPU_STATE_IDLE];
            
            if (total_diff > 0) {
                float cpu_usage = 100.0f * (1.0f - ((float)idle_diff / total_diff));
                total_cpu_usage += cpu_usage;
            }
        }
    }
    
    // Liberar información anterior
    if (last_cpu_info != NULL) {
        vm_deallocate(mach_task_self(), (vm_address_t)last_cpu_info, 
                     last_num_cpu_info * sizeof(processor_info_t));
    }
    
    // Guardar información actual para la próxima llamada
    last_cpu_info = cpu_info;
    last_num_cpu_info = num_cpu_info;
    last_cpu_count = num_cpu_info;
    
    return total_cpu_usage / num_cpu_info;
}

void get_cpu_info_xnu(char *model, int *cores) {
    // Obtener número de cores
    int mib[2] = {CTL_HW, HW_NCPU};
    size_t len = sizeof(*cores);
    if (sysctl(mib, 2, cores, &len, NULL, 0) != 0) {
        *cores = 1;
    }
    
    // Obtener modelo de CPU
    int mib_model[2] = {CTL_HW, HW_MODEL};
    size_t model_len = 128;
    if (sysctl(mib_model, 2, model, &model_len, NULL, 0) != 0) {
        strcpy(model, "Unknown CPU");
    }
}

void get_memory_info_xnu(unsigned long *total, unsigned long *free) {
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t info_count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                     (host_info64_t)&vm_stats, &info_count);
    
    // Calcular memoria total y libre
    *total = (vm_stats.active_count + vm_stats.inactive_count + 
              vm_stats.wire_count + vm_stats.free_count) * vm_page_size / 1024;
    *free = vm_stats.free_count * vm_page_size / 1024;
}

void get_disk_info_xnu(char *model, int *health) {
    // Obtener información del primer disco usando IOKit
    io_registry_entry_t disk = IO_OBJECT_NULL;
    io_iterator_t iterator = IO_OBJECT_NULL;
    
    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, 
                                                   IOServiceMatching("IOMedia"), 
                                                   &iterator);
    if (kr == KERN_SUCCESS) {
        disk = IOIteratorNext(iterator);
        if (disk != IO_OBJECT_NULL) {
            CFStringRef model_cf = IORegistryEntryCreateCFProperty(disk, 
                                                                 CFSTR("IOMediaModel"), 
                                                                 kCFAllocatorDefault, 0);
            if (model_cf) {
                CFStringGetCString(model_cf, model, 128, kCFStringEncodingUTF8);
                CFRelease(model_cf);
            } else {
                strcpy(model, "Generic Disk");
            }
            IOObjectRelease(disk);
        }
        IOObjectRelease(iterator);
    } else {
        strcpy(model, "Generic Disk");
    }
    
    // Para simplificar, asumimos que el disco está saludable
    *health = 100;
}

void collect_metrics(SystemStatus *status) {
    strcpy(status->os_name, "macOS");
    
    // Obtener información de CPU
    get_cpu_info_xnu(status->cpu_model, &status->cpu_cores);
    status->cpu_usage_percent = get_cpu_usage_xnu();
    
    // Obtener información de memoria
    get_memory_info_xnu(&status->ram_total_kb, &status->ram_free_kb);
    
    // Obtener información de disco
    get_disk_info_xnu(status->disk_model, &status->disk_health);
}
