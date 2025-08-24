#include "../metrics.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/wait.h>
#include <stdio.h>

// Variables globales para calcular uso de CPU
static unsigned long last_total = 0;
static unsigned long last_idle = 0;
static unsigned long last_cpu_times[MAX_CORES][4]; // user, nice, system, idle por núcleo

// Declaraciones de funciones
void get_disk_smart_info(DiskInfo *disk);
void get_disk_mount_point(DiskInfo *disk);
void get_network_interface_details(NetworkInfo *net);
void get_gpu_info(SystemStatus *status);

// Declaración manual de getloadavg
extern int getloadavg(double loadavg[], int nelem);

// Función para leer archivos del sistema
char* read_file_content(const char* path) 
{
    FILE *file = fopen(path, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    
    // Remover newline al final
    if (length > 0 && content[length-1] == '\n') {
        content[length-1] = '\0';
    }
    
    return content;
}

// Obtener temperatura de un núcleo específico
float get_core_temperature(int core_id) {
    char path[256];
    float temperature = -1.0f;
    
    // Intentar diferentes rutas para temperatura por núcleo
    // 1. Thermal zones genéricas
    snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", core_id);
    char *content = read_file_content(path);
    if (content) {
        temperature = atof(content) / 1000.0f;
        free(content);
        return temperature;
    }
    
    // 2. Coretemp específico para Intel/AMD
    snprintf(path, sizeof(path), "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp%d_input", core_id + 2);
    content = read_file_content(path);
    if (content) {
        temperature = atof(content) / 1000.0f;
        free(content);
        return temperature;
    }
    
    // 3. K10temp para AMD
    snprintf(path, sizeof(path), "/sys/devices/pci0000:00/0000:00:18.3/hwmon/hwmon*/temp%d_input", core_id + 1);
    content = read_file_content(path);
    if (content) {
        temperature = atof(content) / 1000.0f;
        free(content);
        return temperature;
    }
    
    // 4. Zenpower para AMD Ryzen
    snprintf(path, sizeof(path), "/sys/devices/pci0000:00/0000:00:00.0/hwmon/hwmon*/temp%d_input", core_id + 1);
    content = read_file_content(path);
    if (content) {
        temperature = atof(content) / 1000.0f;
        free(content);
        return temperature;
    }
    
    return -1.0f;
}

// Obtener frecuencia de un núcleo específico
float get_core_frequency(int core_id) {
    char path[256];
    
    // Intentar frecuencia actual
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", core_id);
    char *content = read_file_content(path);
    if (content) {
        float freq = atof(content) / 1000.0f; // Convertir de KHz a MHz
        free(content);
        return freq;
    }
    
    // Intentar frecuencia desde cpuinfo_cur_freq
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq", core_id);
    content = read_file_content(path);
    if (content) {
        float freq = atof(content) / 1000.0f;
        free(content);
        return freq;
    }
    
    // Leer desde /proc/cpuinfo como fallback
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        int current_processor = -1;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "processor", 9) == 0) {
                sscanf(line, "processor : %d", &current_processor);
            } else if (current_processor == core_id && strncmp(line, "cpu MHz", 7) == 0) {
                float freq;
                if (sscanf(line, "cpu MHz : %f", &freq) == 1) {
                    fclose(fp);
                    return freq;
                }
            }
        }
        fclose(fp);
    }
    
    return -1.0f;
}

// Obtener voltaje de un núcleo específico
float get_core_voltage(int core_id) {
    char path[256];
    
    // Intentar diferentes rutas para voltaje por núcleo
    // 1. Zenpower para AMD Ryzen
    snprintf(path, sizeof(path), "/sys/devices/pci0000:00/0000:00:00.0/hwmon/hwmon*/in%d_input", core_id);
    char *content = read_file_content(path);
    if (content) {
        float voltage = atof(content) / 1000.0f; // Convertir de mV a V
        free(content);
        return voltage;
    }
    
    // 2. Coretemp/hwmon genérico
    snprintf(path, sizeof(path), "/sys/devices/platform/coretemp.0/hwmon/hwmon*/in%d_input", core_id);
    content = read_file_content(path);
    if (content) {
        float voltage = atof(content) / 1000.0f;
        free(content);
        return voltage;
    }
    
    // 3. ACPI
    snprintf(path, sizeof(path), "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/device:00/PNP0C09:00/ACPI0007:%02d/power_supply/ADP1/voltage_now", core_id);
    content = read_file_content(path);
    if (content) {
        float voltage = atof(content) / 1000000.0f; // Convertir de µV a V
        free(content);
        return voltage;
    }
    
    return -1.0f;
}

// Obtener frecuencia máxima de un núcleo
float get_core_max_frequency(int core_id) {
    char path[256];
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", core_id);
    
    char *content = read_file_content(path);
    if (content) {
        float freq = atof(content) / 1000.0f; // Convertir de KHz a MHz
        free(content);
        return freq;
    }
    return -1.0f;
}

// Obtener frecuencia mínima de un núcleo
float get_core_min_frequency(int core_id) {
    char path[256];
    snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", core_id);
    
    char *content = read_file_content(path);
    if (content) {
        float freq = atof(content) / 1000.0f; // Convertir de KHz a MHz
        free(content);
        return freq;
    }
    return -1.0f;
}

// Obtener información de tiempo de actividad del sistema
void get_uptime_info(SystemStatus *status) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp) {
        double uptime, idle_time;
        if (fscanf(fp, "%lf %lf", &uptime, &idle_time) == 2) {
            status->uptime_seconds = (unsigned long)uptime;
            status->idle_time_seconds = (unsigned long)idle_time;
        }
        fclose(fp);
    }
}

// Obtener información detallada de memoria desde /proc/meminfo
void get_detailed_memory_info(SystemStatus *status) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) 
    {
        unsigned long value;
        if (sscanf(line, "MemTotal: %lu kB", &value) == 1) {
            status->detailed_ram.total_kb = value;
        } else if (sscanf(line, "MemFree: %lu kB", &value) == 1) {
            status->detailed_ram.free_kb = value;
        } else if (sscanf(line, "MemAvailable: %lu kB", &value) == 1) {
            status->detailed_ram.available_kb = value;
        } else if (sscanf(line, "Cached: %lu kB", &value) == 1) {
            status->detailed_ram.cached_kb = value;
        } else if (sscanf(line, "Buffers: %lu kB", &value) == 1) {
            status->detailed_ram.buffers_kb = value;
        } else if (sscanf(line, "Shmem: %lu kB", &value) == 1) {
            status->detailed_ram.shared_kb = value;
        } else if (sscanf(line, "Slab: %lu kB", &value) == 1) {
            status->detailed_ram.slab_kb = value;
        }
    }
    fclose(fp);
    
    // Calcular RAM usada
    status->detailed_ram.used_kb = status->detailed_ram.total_kb - status->detailed_ram.available_kb;
    status->detailed_ram.usage_percent = (status->detailed_ram.used_kb * 100.0f) / status->detailed_ram.total_kb;
}

// Obtener información de módulos de RAM desde DMI
void get_ram_modules_info(SystemStatus *status) {
    status->detailed_ram.num_sticks = 0;
    
    // Leer información desde DMI
    for (int slot = 0; slot < 16; slot++) {
        char path[512];
        
        // Verificar si el slot existe
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_size", slot);
        char *size_content = read_file_content(path);
        if (!size_content) continue;
        
        // Obtener tamaño
        unsigned long size_mb = 0;
        if (strstr(size_content, "MB")) {
            sscanf(size_content, "%lu MB", &size_mb);
        } else if (strstr(size_content, "GB")) {
            float size_gb;
            sscanf(size_content, "%f GB", &size_gb);
            size_mb = (unsigned long)(size_gb * 1024);
        }
        free(size_content);
        
        if (size_mb == 0) continue; // Slot vacío
        
        RamStickInfo *stick = &status->detailed_ram.sticks[status->detailed_ram.num_sticks];
        stick->slot_id = slot;
        stick->size_mb = size_mb;
        
        // Obtener fabricante
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_manufacturer", slot);
        char *manufacturer = read_file_content(path);
        if (manufacturer) {
            strncpy(stick->manufacturer, manufacturer, 63);
            free(manufacturer);
        } else {
            strcpy(stick->manufacturer, "Unknown");
        }
        
        // Obtener número de parte
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_part_number", slot);
        char *part_number = read_file_content(path);
        if (part_number) {
            strncpy(stick->part_number, part_number, 63);
            free(part_number);
        } else {
            strcpy(stick->part_number, "Unknown");
        }
        
        // Obtener serial
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_serial", slot);
        char *serial = read_file_content(path);
        if (serial) {
            strncpy(stick->serial_number, serial, 63);
            free(serial);
        } else {
            strcpy(stick->serial_number, "Unknown");
        }
        
        // Obtener tipo de memoria
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_type", slot);
        char *type = read_file_content(path);
        if (type) {
            strncpy(stick->type, type, 15);
            free(type);
        } else {
            strcpy(stick->type, "Unknown");
        }
        
        // Obtener frecuencia
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_speed", slot);
        char *speed = read_file_content(path);
        if (speed) {
            sscanf(speed, "%d MHz", &stick->frequency_mhz);
            free(speed);
        } else {
            stick->frequency_mhz = 0;
        }
        
        // Obtener voltaje (si está disponible)
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_voltage", slot);
        char *voltage = read_file_content(path);
        if (voltage) {
            float voltage_v;
            sscanf(voltage, "%f V", &voltage_v);
            stick->voltage_mv = (int)(voltage_v * 1000);
            free(voltage);
        } else {
            stick->voltage_mv = 0;
        }
        
        // Obtener form factor
        snprintf(path, sizeof(path), "/sys/devices/virtual/dmi/id/memory_device_%d_form_factor", slot);
        char *form_factor = read_file_content(path);
        if (form_factor) {
            strncpy(stick->form_factor, form_factor, 31);
            free(form_factor);
        } else {
            strcpy(stick->form_factor, "Unknown");
        }
        
        status->detailed_ram.num_sticks++;
    }
    
    // Determinar arquitectura de RAM y frecuencia general
    if (status->detailed_ram.num_sticks > 0) {
        strncpy(status->detailed_ram.architecture, status->detailed_ram.sticks[0].type, 15);
        status->detailed_ram.frequency_mhz = status->detailed_ram.sticks[0].frequency_mhz;
    } else {
        strcpy(status->detailed_ram.architecture, "Unknown");
        status->detailed_ram.frequency_mhz = 0;
    }
}

// Obtener información de caché del procesador
void get_cache_info(SystemStatus *status) {
    status->detailed_ram.l1_cache_kb = 0;
    status->detailed_ram.l2_cache_kb = 0;
    status->detailed_ram.l3_cache_kb = 0;
    
    // Leer información de caché desde /sys/devices/system/cpu/cpu0/cache/
    for (int level = 0; level < 4; level++) {
        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/size", level);
        
        char *content = read_file_content(path);
        if (content) {
            unsigned long size_kb = 0;
            if (strstr(content, "K")) {
                sscanf(content, "%lu", &size_kb);
            } else if (strstr(content, "M")) {
                float size_mb;
                sscanf(content, "%f", &size_mb);
                size_kb = (unsigned long)(size_mb * 1024);
            }
            
            // Determinar el nivel de caché
            char level_path[256];
            snprintf(level_path, sizeof(level_path), "/sys/devices/system/cpu/cpu0/cache/index%d/level", level);
            char *level_content = read_file_content(level_path);
            if (level_content) {
                int cache_level = atoi(level_content);
                switch (cache_level) {
                    case 1:
                        if (status->detailed_ram.l1_cache_kb == 0) 
                            status->detailed_ram.l1_cache_kb = size_kb;
                        break;
                    case 2:
                        if (status->detailed_ram.l2_cache_kb == 0) 
                            status->detailed_ram.l2_cache_kb = size_kb;
                        break;
                    case 3:
                        status->detailed_ram.l3_cache_kb = size_kb;
                        break;
                }
                free(level_content);
            }
            free(content);
        }
    }
}

// Obtener uso de CPU por núcleo
float get_core_usage(int core_id) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/stat");
    
    FILE *fp = fopen(path, "r");
    if (!fp) return 0.0f;
    
    char line[512];
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    float usage = 0.0f;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
            int current_core = atoi(line + 3);
            if (current_core == core_id) {
                sscanf(line, "cpu%d %lu %lu %lu %lu %lu %lu %lu %lu", 
                       &current_core, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
                
                unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
                unsigned long total_diff = total - last_cpu_times[core_id][3];
                unsigned long idle_diff = idle - last_cpu_times[core_id][2];
                
                if (total_diff > 0) {
                    usage = 100.0f * (1.0f - ((float)idle_diff / total_diff));
                }
                
                last_cpu_times[core_id][0] = user;
                last_cpu_times[core_id][1] = nice;
                last_cpu_times[core_id][2] = idle;
                last_cpu_times[core_id][3] = total;
                break;
            }
        }
    }
    fclose(fp);
    return usage;
}

// Obtener información de sensores usando hwmon directamente
void get_sensor_info(SystemStatus *status) {
    status->num_sensors = 0;
    
    // Leer sensores desde /sys/class/hwmon
    DIR *hwmon_dir = opendir("/sys/class/hwmon");
    if (!hwmon_dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(hwmon_dir)) != NULL && status->num_sensors < MAX_SENSORS) {
        if (strncmp(entry->d_name, "hwmon", 5) != 0) continue;
        
        char hwmon_path[256];
        snprintf(hwmon_path, sizeof(hwmon_path), "/sys/class/hwmon/%s", entry->d_name);
        
        // Buscar archivos de temperatura
        for (int i = 1; i <= 20; i++) {
            char temp_path[512];
            snprintf(temp_path, sizeof(temp_path), "%s/temp%d_input", hwmon_path, i);
            
            char *temp_content = read_file_content(temp_path);
            if (temp_content && status->num_sensors < MAX_SENSORS) {
                float temp = atof(temp_content) / 1000.0f;
                
                // Obtener nombre del sensor
                char name_path[512];
                snprintf(name_path, sizeof(name_path), "%s/temp%d_label", hwmon_path, i);
                char *name_content = read_file_content(name_path);
                
                if (name_content) {
                    strncpy(status->sensors[status->num_sensors].name, name_content, 63);
                    free(name_content);
                } else {
                    snprintf(status->sensors[status->num_sensors].name, 64, "%s_temp%d", entry->d_name, i);
                }
                
                strcpy(status->sensors[status->num_sensors].type, "temperature");
                status->sensors[status->num_sensors].value = temp;
                strcpy(status->sensors[status->num_sensors].unit, "°C");
                status->sensors[status->num_sensors].critical = (temp > 85.0f) ? 1 : 0;
                status->num_sensors++;
                
                free(temp_content);
            }
        }
        
        // Buscar voltajes
        for (int i = 0; i <= 10; i++) {
            char volt_path[512];
            snprintf(volt_path, sizeof(volt_path), "%s/in%d_input", hwmon_path, i);
            
            char *volt_content = read_file_content(volt_path);
            if (volt_content && status->num_sensors < MAX_SENSORS) {
                float voltage = atof(volt_content) / 1000.0f; // mV a V
                
                // Obtener nombre del sensor
                char name_path[512];
                snprintf(name_path, sizeof(name_path), "%s/in%d_label", hwmon_path, i);
                char *name_content = read_file_content(name_path);
                
                if (name_content) {
                    strncpy(status->sensors[status->num_sensors].name, name_content, 63);
                    free(name_content);
                } else {
                    snprintf(status->sensors[status->num_sensors].name, 64, "%s_in%d", entry->d_name, i);
                }
                
                strcpy(status->sensors[status->num_sensors].type, "voltage");
                status->sensors[status->num_sensors].value = voltage;
                strcpy(status->sensors[status->num_sensors].unit, "V");
                status->sensors[status->num_sensors].critical = (voltage < 0.5f || voltage > 2.0f) ? 1 : 0;
                status->num_sensors++;
                
                free(volt_content);
            }
        }
        
        // Buscar ventiladores
        for (int i = 1; i <= 10; i++) {
            char fan_path[512];
            snprintf(fan_path, sizeof(fan_path), "%s/fan%d_input", hwmon_path, i);
            
            char *fan_content = read_file_content(fan_path);
            if (fan_content && status->num_sensors < MAX_SENSORS) {
                float rpm = atof(fan_content);
                
                // Obtener nombre del sensor
                char name_path[512];
                snprintf(name_path, sizeof(name_path), "%s/fan%d_label", hwmon_path, i);
                char *name_content = read_file_content(name_path);
                
                if (name_content) {
                    strncpy(status->sensors[status->num_sensors].name, name_content, 63);
                    free(name_content);
                } else {
                    snprintf(status->sensors[status->num_sensors].name, 64, "%s_fan%d", entry->d_name, i);
                }
                
                strcpy(status->sensors[status->num_sensors].type, "fan");
                status->sensors[status->num_sensors].value = rpm;
                strcpy(status->sensors[status->num_sensors].unit, "RPM");
                status->sensors[status->num_sensors].critical = (rpm < 500.0f) ? 1 : 0;
                status->num_sensors++;
                
                free(fan_content);
            }
        }
    }
    closedir(hwmon_dir);
}

// Obtener información de la motherboard
void get_motherboard_info(SystemStatus *status) {
    // Leer información del DMI
    char *manufacturer = read_file_content("/sys/class/dmi/id/board_vendor");
    char *model = read_file_content("/sys/class/dmi/id/board_name");
    char *version = read_file_content("/sys/class/dmi/id/board_version");
    char *serial = read_file_content("/sys/class/dmi/id/board_serial");
    char *bios = read_file_content("/sys/class/dmi/id/bios_version");
    
    if (manufacturer) {
        strncpy(status->motherboard.manufacturer, manufacturer, 63);
        free(manufacturer);
    } else {
        strcpy(status->motherboard.manufacturer, "Unknown");
    }
    
    if (model) {
        strncpy(status->motherboard.model, model, 127);
        free(model);
    } else {
        strcpy(status->motherboard.model, "Unknown");
    }
    
    if (version) {
        strncpy(status->motherboard.version, version, 31);
        free(version);
    } else {
        strcpy(status->motherboard.version, "Unknown");
    }
    
    if (serial) {
        strncpy(status->motherboard.serial, serial, 63);
        free(serial);
    } else {
        strcpy(status->motherboard.serial, "Unknown");
    }
    
    if (bios) {
        strncpy(status->motherboard.bios_version, bios, 63);
        free(bios);
    } else {
        strcpy(status->motherboard.bios_version, "Unknown");
    }
    
    // Intentar obtener información del chipset
    char *chipset = read_file_content("/sys/class/dmi/id/chassis_type");
    if (chipset) {
        strncpy(status->motherboard.chipset, chipset, 63);
        free(chipset);
    } else {
        strcpy(status->motherboard.chipset, "Unknown");
    }
}

// Obtener información detallada de discos
void get_disk_info(SystemStatus *status) {
    status->num_disks = 0;
    
    DIR *dir = opendir("/sys/block");
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && status->num_disks < MAX_DISKS) {
        // Saltar entradas especiales
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        // Saltar dispositivos de loopback y ramdisk
        if (strncmp(entry->d_name, "loop", 4) == 0 || strncmp(entry->d_name, "ram", 3) == 0)
            continue;
        
        DiskInfo *disk = &status->disks[status->num_disks];
        strncpy(disk->name, entry->d_name, 63);
        snprintf(disk->device_path, sizeof(disk->device_path), "/dev/%s", entry->d_name);
        
        // Determinar protocolo del disco
        if (strncmp(entry->d_name, "nvme", 4) == 0) {
            strcpy(disk->protocol, "NVMe");
        } else if (strncmp(entry->d_name, "sd", 2) == 0) {
            // Verificar si es SSD o HDD
            char queue_path[256];
            snprintf(queue_path, sizeof(queue_path), "/sys/block/%s/queue/rotational", entry->d_name);
            char *rotational = read_file_content(queue_path);
            if (rotational) {
                if (atoi(rotational) == 0) {
                    strcpy(disk->protocol, "SSD");
                } else {
                    strcpy(disk->protocol, "HDD");
                }
                free(rotational);
            } else {
                strcpy(disk->protocol, "SATA");
            }
        } else {
            strcpy(disk->protocol, "Unknown");
        }
        
        // Obtener modelo del disco
        char model_path[256];
        snprintf(model_path, sizeof(model_path), "/sys/block/%s/device/model", entry->d_name);
        char *model_content = read_file_content(model_path);
        if (model_content) {
            strncpy(disk->model, model_content, 127);
            free(model_content);
        } else {
            strcpy(disk->model, "Unknown");
        }
        
        // Obtener serial del disco
        char serial_path[256];
        snprintf(serial_path, sizeof(serial_path), "/sys/block/%s/device/serial", entry->d_name);
        char *serial_content = read_file_content(serial_path);
        if (serial_content) {
            strncpy(disk->serial, serial_content, 63);
            free(serial_content);
        } else {
            strcpy(disk->serial, "Unknown");
        }
        
        // Obtener información de uso del disco
        char mount_path[256];
        snprintf(mount_path, sizeof(mount_path), "/dev/%s", entry->d_name);
        
        struct statvfs vfs;
        if (statvfs(mount_path, &vfs) == 0) {
            disk->total_kb = (vfs.f_blocks * vfs.f_frsize) / 1024;
            disk->free_kb = (vfs.f_bavail * vfs.f_frsize) / 1024;
            disk->used_kb = disk->total_kb - disk->free_kb;
            disk->usage_percent = (disk->used_kb * 100.0f) / disk->total_kb;
        } else {
            disk->total_kb = 0;
            disk->free_kb = 0;
            disk->used_kb = 0;
            disk->usage_percent = 0.0f;
        }
        
        // Obtener temperatura del disco (si está disponible)
        disk->temperature = -1.0f; // Por defecto
        
        // Intentar diferentes rutas para temperatura del disco
        char temp_path[256];
        snprintf(temp_path, sizeof(temp_path), "/sys/block/%s/device/hwmon/hwmon*/temp1_input", entry->d_name);
        
        // Usar glob para encontrar archivos de temperatura
        glob_t globbuf;
        int glob_result = glob(temp_path, 0, NULL, &globbuf);
        if (glob_result == 0 && globbuf.gl_pathc > 0) {
            char *temp_content = read_file_content(globbuf.gl_pathv[0]);
            if (temp_content) {
                disk->temperature = atof(temp_content) / 1000.0f; // Convertir de mC a C
                free(temp_content);
            }
            globfree(&globbuf);
        }
        
        // Obtener información SMART básica
        get_disk_smart_info(disk);
        
        // Obtener punto de montaje
        get_disk_mount_point(disk);
        
        status->num_disks++;
    }
    closedir(dir);
}

// Obtener información SMART del disco
void get_disk_smart_info(DiskInfo *disk) {
    // Inicializar valores por defecto
    disk->sectors_read = 0;
    disk->sectors_written = 0;
    disk->read_errors = 0;
    disk->write_errors = 0;
    disk->power_on_hours = 0;
    disk->power_cycles = 0;
    disk->smart_status = 0;
    disk->health = 100;
    
    // Intentar obtener información SMART usando smartctl si está disponible
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "smartctl -a %s 2>/dev/null", disk->device_path);
    
    FILE *fp = popen(cmd, "r");
    if (fp != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), fp) != NULL) {
            // Buscar información específica de SMART
            if (strstr(line, "Power_On_Hours")) {
                sscanf(line, "%*s %lu", &disk->power_on_hours);
            } else if (strstr(line, "Power_Cycle_Count")) {
                sscanf(line, "%*s %lu", &disk->power_cycles);
            } else if (strstr(line, "Sectors_Read")) {
                sscanf(line, "%*s %lu", &disk->sectors_read);
            } else if (strstr(line, "Sectors_Written")) {
                sscanf(line, "%*s %lu", &disk->sectors_written);
            } else if (strstr(line, "Read_Error_Rate")) {
                sscanf(line, "%*s %lu", &disk->read_errors);
            } else if (strstr(line, "Write_Error_Rate")) {
                sscanf(line, "%*s %lu", &disk->write_errors);
            } else if (strstr(line, "SMART overall-health")) {
                if (strstr(line, "PASSED")) {
                    disk->smart_status = 0; // OK
                } else if (strstr(line, "FAILED")) {
                    disk->smart_status = 2; // Critical
                    disk->health = 0;
                } else {
                    disk->smart_status = 1; // Warning
                    disk->health = 50;
                }
            }
        }
        pclose(fp);
    }
    
    // Si no se pudo obtener información SMART, intentar desde /sys
    if (disk->power_on_hours == 0) {
        char power_path[256];
        snprintf(power_path, sizeof(power_path), "/sys/block/%s/device/power_on_hours", disk->name);
        char *power_content = read_file_content(power_path);
        if (power_content) {
            disk->power_on_hours = atol(power_content);
            free(power_content);
        }
    }
}

// Obtener punto de montaje del disco
void get_disk_mount_point(DiskInfo *disk) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        strcpy(disk->mount_point, "Not mounted");
        strcpy(disk->filesystem, "Unknown");
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char device[256], mount_point[256], fs_type[32];
        if (sscanf(line, "%s %s %s", device, mount_point, fs_type) == 3) {
            if (strcmp(device, disk->device_path) == 0) {
                strncpy(disk->mount_point, mount_point, 255);
                strncpy(disk->filesystem, fs_type, 31);
                fclose(fp);
                return;
            }
        }
    }
    fclose(fp);
    
    strcpy(disk->mount_point, "Not mounted");
    strcpy(disk->filesystem, "Unknown");
}

// Obtener información de red
void get_network_info(SystemStatus *status) {
    status->num_network_interfaces = 0;
    
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;
    
    char line[512];
    fgets(line, sizeof(line), fp); // Saltar la primera línea
    fgets(line, sizeof(line), fp); // Saltar la segunda línea
    
    while (fgets(line, sizeof(line), fp) && status->num_network_interfaces < MAX_NETWORK_INTERFACES) {
        char interface[32];
        unsigned long rx_bytes, tx_bytes, rx_packets, tx_packets, rx_errors, tx_errors, rx_dropped, tx_dropped;
        
        if (sscanf(line, "%31s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   interface, &rx_bytes, &rx_packets, &rx_errors, &rx_dropped,
                   &tx_bytes, &tx_packets, &tx_errors, &tx_dropped,
                   &rx_bytes, &rx_packets, &rx_errors, &rx_dropped,
                   &tx_bytes, &tx_packets, &tx_errors, &tx_dropped) >= 5) {
            
            NetworkInfo *net = &status->network_interfaces[status->num_network_interfaces];
            
            // Remover ':' del nombre de la interfaz
            char *colon = strchr(interface, ':');
            if (colon) *colon = '\0';
            
            strncpy(net->name, interface, 31);
            net->rx_bytes = rx_bytes;
            net->tx_bytes = tx_bytes;
            net->rx_packets = rx_packets;
            net->tx_packets = tx_packets;
            net->rx_errors = rx_errors;
            net->tx_errors = tx_errors;
            net->rx_dropped = rx_dropped;
            net->tx_dropped = tx_dropped;
            
            // Calcular Mbps
            net->rx_mbps = (rx_bytes * 8.0f) / (1024 * 1024);
            net->tx_mbps = (tx_bytes * 8.0f) / (1024 * 1024);
            
            // Obtener información adicional de la interfaz
            get_network_interface_details(net);
            
            status->num_network_interfaces++;
        }
    }
    
    fclose(fp);
}

// Obtener detalles adicionales de la interfaz de red
void get_network_interface_details(NetworkInfo *net) {
    // Obtener dirección MAC
    char mac_path[256];
    snprintf(mac_path, sizeof(mac_path), "/sys/class/net/%s/address", net->name);
    char *mac_content = read_file_content(mac_path);
    if (mac_content) {
        strncpy(net->mac_address, mac_content, 17);
        free(mac_content);
    } else {
        strcpy(net->mac_address, "00:00:00:00:00:00");
    }
    
    // Obtener tipo de interfaz
    char type_path[256];
    snprintf(type_path, sizeof(type_path), "/sys/class/net/%s/type", net->name);
    char *type_content = read_file_content(type_path);
    if (type_content) {
        int type = atoi(type_content);
        switch (type) {
            case 1:
                strcpy(net->type, "ethernet");
                break;
            case 772:
                strcpy(net->type, "loopback");
                break;
            case 801:
                strcpy(net->type, "wifi");
                break;
            default:
                strcpy(net->type, "unknown");
                break;
        }
        free(type_content);
    } else {
        strcpy(net->type, "unknown");
    }
    
    // Obtener driver
    char driver_path[256];
    snprintf(driver_path, sizeof(driver_path), "/sys/class/net/%s/device/driver/module/name", net->name);
    char *driver_content = read_file_content(driver_path);
    if (driver_content) {
        strncpy(net->driver, driver_content, 63);
        free(driver_content);
    } else {
        strcpy(net->driver, "Unknown");
    }
    
    // Obtener velocidad de la interfaz
    char speed_path[256];
    snprintf(speed_path, sizeof(speed_path), "/sys/class/net/%s/speed", net->name);
    char *speed_content = read_file_content(speed_path);
    if (speed_content) {
        net->speed_mbps = atoi(speed_content);
        free(speed_content);
    } else {
        net->speed_mbps = 0;
    }
    
    // Obtener estado operativo
    char operstate_path[256];
    snprintf(operstate_path, sizeof(operstate_path), "/sys/class/net/%s/operstate", net->name);
    char *operstate_content = read_file_content(operstate_path);
    if (operstate_content) {
        if (strstr(operstate_content, "up")) {
            net->status = 1;
        } else {
            net->status = 0;
        }
        free(operstate_content);
    } else {
        net->status = 0;
    }
    
    // Obtener información de IP (simplificado)
    strcpy(net->ip_address, "0.0.0.0");
    strcpy(net->netmask, "0.0.0.0");
    strcpy(net->gateway, "0.0.0.0");
    strcpy(net->dns, "0.0.0.0");
    net->duplex = 1; // Full duplex por defecto
}

// Obtener información de GPU
void get_gpu_info(SystemStatus *status) {
    // Inicializar valores por defecto
    strcpy(status->gpu_model, "Unknown GPU");
    strcpy(status->gpu_driver, "Unknown");
    strcpy(status->gpu_vendor, "Unknown");
    status->gpu_memory_total_mb = 0;
    status->gpu_memory_used_mb = 0;
    status->gpu_memory_free_mb = 0;
    status->gpu_usage_percent = -1.0f;
    status->gpu_temperature = -1.0f;
    status->gpu_memory_usage_percent = -1.0f;
    status->gpu_fan_speed = -1;
    status->gpu_power_usage_w = -1;
    status->gpu_clock_mhz = -1;
    status->gpu_memory_clock_mhz = -1;
    
    // Intentar obtener información de GPU usando lspci
    FILE *fp = popen("lspci | grep -i vga", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            // Extraer información básica de la GPU
            if (strstr(line, "NVIDIA")) {
                strcpy(status->gpu_vendor, "NVIDIA");
                // Intentar obtener modelo específico
                if (strstr(line, "GTX") || strstr(line, "RTX") || strstr(line, "Quadro")) {
                    char *start = strstr(line, "NVIDIA");
                    if (start) {
                        strncpy(status->gpu_model, start, 127);
                    }
                }
            } else if (strstr(line, "AMD")) {
                strcpy(status->gpu_vendor, "AMD");
                if (strstr(line, "Radeon") || strstr(line, "RX") || strstr(line, "Vega")) {
                    char *start = strstr(line, "AMD");
                    if (start) {
                        strncpy(status->gpu_model, start, 127);
                    }
                }
            } else if (strstr(line, "Intel")) {
                strcpy(status->gpu_vendor, "Intel");
                if (strstr(line, "HD Graphics") || strstr(line, "UHD Graphics") || strstr(line, "Iris")) {
                    char *start = strstr(line, "Intel");
                    if (start) {
                        strncpy(status->gpu_model, start, 127);
                    }
                }
            }
        }
        pclose(fp);
    }
    
    // Intentar obtener información de driver
    FILE *driver_fp = popen("lspci -v | grep -A 10 -i vga | grep -i driver", "r");
    if (driver_fp) {
        char line[256];
        if (fgets(line, sizeof(line), driver_fp)) {
            char *driver_start = strstr(line, "driver");
            if (driver_start) {
                driver_start += 7; // Saltar "driver "
                char *driver_end = strchr(driver_start, '\n');
                if (driver_end) *driver_end = '\0';
                strncpy(status->gpu_driver, driver_start, 63);
            }
        }
        pclose(driver_fp);
    }
    
    // Intentar obtener información de memoria GPU (NVIDIA)
    if (strcmp(status->gpu_vendor, "NVIDIA") == 0) {
        FILE *nvidia_fp = popen("nvidia-smi --query-gpu=memory.total,memory.used,memory.free,utilization.gpu,temperature.gpu,fan.speed,power.draw,clocks.current.graphics,clocks.current.memory --format=csv,noheader,nounits 2>/dev/null", "r");
        if (nvidia_fp) {
            char line[512];
            if (fgets(line, sizeof(line), nvidia_fp)) {
                int total, used, free, util, temp, fan, power, clock, mem_clock;
                if (sscanf(line, "%d, %d, %d, %d, %d, %d, %d, %d, %d", 
                           &total, &used, &free, &util, &temp, &fan, &power, &clock, &mem_clock) >= 5) {
                    status->gpu_memory_total_mb = total;
                    status->gpu_memory_used_mb = used;
                    status->gpu_memory_free_mb = free;
                    status->gpu_usage_percent = util;
                    status->gpu_temperature = temp;
                    status->gpu_memory_usage_percent = (used * 100.0f) / total;
                    status->gpu_fan_speed = fan;
                    status->gpu_power_usage_w = power;
                    status->gpu_clock_mhz = clock;
                    status->gpu_memory_clock_mhz = mem_clock;
                }
            }
            pclose(nvidia_fp);
        }
    }
    
    // Intentar obtener información de GPU desde /sys (para GPUs integradas)
    if (status->gpu_usage_percent == -1.0f) {
        // Buscar sensores de GPU en hwmon
        DIR *dir = opendir("/sys/class/hwmon");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                char hwmon_path[256];
                snprintf(hwmon_path, sizeof(hwmon_path), "/sys/class/hwmon/%s", entry->d_name);
                
                // Verificar si es un sensor de GPU
                char name_path[256];
                snprintf(name_path, sizeof(name_path), "%s/name", hwmon_path);
                char *name_content = read_file_content(name_path);
                if (name_content) {
                    if (strstr(name_content, "gpu") || strstr(name_content, "GPU") || 
                        strstr(name_content, "amdgpu") || strstr(name_content, "i915")) {
                        
                        // Obtener temperatura de GPU
                        char temp_path[256];
                        snprintf(temp_path, sizeof(temp_path), "%s/temp1_input", hwmon_path);
                        char *temp_content = read_file_content(temp_path);
                        if (temp_content) {
                            status->gpu_temperature = atof(temp_content) / 1000.0f;
                            free(temp_content);
                        }
                        
                        // Obtener velocidad del ventilador
                        char fan_path[256];
                        snprintf(fan_path, sizeof(fan_path), "%s/fan1_input", hwmon_path);
                        char *fan_content = read_file_content(fan_path);
                        if (fan_content) {
                            status->gpu_fan_speed = atoi(fan_content);
                            free(fan_content);
                        }
                    }
                    free(name_content);
                }
            }
            closedir(dir);
        }
    }
}

// Obtener información de carga del sistema
void get_load_average(SystemStatus *status) {
    double loadavg[3];
    if (getloadavg(loadavg, 3) == 3) {
        status->load_average_1min = (float)loadavg[0];
        status->load_average_5min = (float)loadavg[1];
        status->load_average_15min = (float)loadavg[2];
    }
}

// Obtener información de procesos
void get_process_info(SystemStatus *status) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "procs_", 6) == 0) {
            int running, sleeping, stopped, zombie;
            if (sscanf(line, "procs_running %d", &running) == 1) {
                status->running_processes = running;
            } else if (sscanf(line, "procs_blocked %d", &sleeping) == 1) {
                status->sleeping_processes = sleeping;
            }
        }
    }
    fclose(fp);
    
    // Obtener total de procesos
    DIR *dir = opendir("/proc");
    if (dir) {
        int count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
                count++;
            }
        }
        status->total_processes = count;
        closedir(dir);
    }
}

float get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0f;
    
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    
    unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long total_diff = total - last_total;
    unsigned long idle_diff = idle - last_idle;
    
    float cpu_usage = 0.0f;
    if (total_diff > 0) {
        cpu_usage = 100.0f * (1.0f - ((float)idle_diff / total_diff));
    }
    
    last_total = total;
    last_idle = idle;
    
    return cpu_usage;
}

void collect_metrics(SystemStatus *status) {
    // Información básica del sistema
    strcpy(status->os_name, "Linux");
    
    // Información del kernel
    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(status->kernel_version, uts.release, 63);
        strncpy(status->hostname, uts.nodename, 63);
    }
    
    // Obtener hostname
    char *hostname = read_file_content("/proc/sys/kernel/hostname");
    if (hostname) {
        strncpy(status->hostname, hostname, 63);
        free(hostname);
    }
    
    // Información de CPU
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    colon++; // Saltar el ':'
                    while (*colon == ' ' || *colon == '\t') colon++; // Saltar espacios
                    strncpy(status->cpu_model, colon, 127);
                    status->cpu_model[127] = '\0';
                    // Remover newline
                    status->cpu_model[strcspn(status->cpu_model, "\n")] = 0;
                }
                break;
            }
        }
        fclose(fp);
    } else {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    status->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    status->cpu_threads = status->cpu_cores; // Simplificado
    status->cpu_usage_percent = get_cpu_usage();
    
    // Obtener información detallada por núcleo
    status->num_cores = 0;
    for (int i = 0; i < status->cpu_cores && i < MAX_CORES; i++) {
        status->cores[i].core_id = i;
        status->cores[i].temperature = get_core_temperature(i);
        status->cores[i].frequency = get_core_frequency(i);
        status->cores[i].voltage = get_core_voltage(i);
        status->cores[i].usage_percent = get_core_usage(i);
        status->cores[i].max_frequency = get_core_max_frequency(i);
        status->cores[i].min_frequency = get_core_min_frequency(i);
        status->cores[i].load = 0; // Por implementar si es necesario
        status->num_cores++;
    }
    
    // Temperatura promedio de CPU
    float total_temp = 0.0f;
    int valid_temps = 0;
    for (int i = 0; i < status->num_cores; i++) {
        if (status->cores[i].temperature > 0) {
            total_temp += status->cores[i].temperature;
            valid_temps++;
        }
    }
    status->cpu_temperature = (valid_temps > 0) ? total_temp / valid_temps : -1.0f;
    
    // Frecuencia promedio de CPU
    float total_freq = 0.0f;
    int valid_freqs = 0;
    for (int i = 0; i < status->num_cores; i++) {
        if (status->cores[i].frequency > 0) {
            total_freq += status->cores[i].frequency;
            valid_freqs++;
        }
    }
    status->cpu_frequency = (valid_freqs > 0) ? total_freq / valid_freqs : -1.0f;

    // Obtener información básica de memoria
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        status->ram_total_kb = si.totalram / 1024;
        status->ram_free_kb = si.freeram / 1024;
        status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
        status->ram_usage_percent = (status->ram_used_kb * 100.0f) / status->ram_total_kb;
    } else {
        // Fallback a /proc/meminfo
        FILE *mem = fopen("/proc/meminfo", "r");
        if (mem) {
            char line[256];
            while (fgets(line, sizeof(line), mem)) {
                if (strncmp(line, "MemTotal", 8) == 0)
                    sscanf(line, "MemTotal: %lu kB", &status->ram_total_kb);
                if (strncmp(line, "MemAvailable", 12) == 0)
                    sscanf(line, "MemAvailable: %lu kB", &status->ram_free_kb);
            }
            fclose(mem);
            status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
            status->ram_usage_percent = (status->ram_used_kb * 100.0f) / status->ram_total_kb;
        }
    }
    
    // Obtener información detallada de memoria
    // get_detailed_memory_info(status);
    // get_ram_modules_info(status);
    // get_cache_info(status);

    // Obtener información de GPU
    // get_gpu_info(status);
    
    // Obtener información de sensores
    get_sensor_info(status);
    
    // Obtener información de la motherboard
    // get_motherboard_info(status);
    
    // Obtener información de discos
    // get_disk_info(status);
    
    // Obtener información de red
    get_network_info(status);
    
    // Información de energía (simplificado)
    status->power_consumption = -1.0f;
    status->battery_percent = -1.0f;
    status->ac_connected = -1;
    
    // Obtener información de carga del sistema
    get_load_average(status);
    
    // Obtener información de procesos
    get_process_info(status);
    
    // Timestamp y tiempo de actividad
    status->timestamp = time(NULL);
    // get_uptime_info(status);
    
    // Inicializar campos que no se están llenando
    status->num_sensors = 0;
    status->num_disks = 0;
    status->num_network_interfaces = 0;
    status->load_average_1min = 0.0f;
    status->load_average_5min = 0.0f;
    status->load_average_15min = 0.0f;
    status->total_processes = 0;
    status->running_processes = 0;
    status->sleeping_processes = 0;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
    status->uptime_seconds = 0;
    status->idle_time_seconds = 0;
    status->load_1min_percent = 0.0f;
    status->load_5min_percent = 0.0f;
    status->load_15min_percent = 0.0f;
}
