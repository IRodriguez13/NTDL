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

// Variables globales para calcular uso de CPU
static unsigned long last_total = 0;
static unsigned long last_idle = 0;

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

// Obtener uso de CPU
float get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0f;
    
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", 
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    
    unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long idle_total = idle + iowait;
    
    if (last_total == 0) {
        last_total = total;
        last_idle = idle_total;
        return 0.0f;
    }
    
    unsigned long total_diff = total - last_total;
    unsigned long idle_diff = idle_total - last_idle;
    
    last_total = total;
    last_idle = idle_total;
    
    if (total_diff == 0) return 0.0f;
    
    return 100.0f * (1.0f - ((float)idle_diff / total_diff));
}

// Función principal de recolección de métricas
void collect_metrics(SystemStatus *status) {
    // Información básica del sistema
    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(status->os_name, uts.sysname, 63);
        strncpy(status->kernel_version, uts.release, 63);
        strncpy(status->hostname, uts.nodename, 63);
    } else {
        strcpy(status->os_name, "Unknown");
        strcpy(status->kernel_version, "Unknown");
        strcpy(status->hostname, "Unknown");
    }
    
    // Información de CPU
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *model = strchr(line, ':');
                if (model) {
                    model += 2; // Saltar ": "
                    char *newline = strchr(model, '\n');
                    if (newline) *newline = '\0';
                    strncpy(status->cpu_model, model, 127);
                    break;
                }
            }
        }
        fclose(cpuinfo);
    } else {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    status->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    status->cpu_threads = status->cpu_cores;
    status->cpu_usage_percent = get_cpu_usage();
    
    // Información básica de memoria
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        status->ram_total_kb = si.totalram / 1024;
        status->ram_free_kb = si.freeram / 1024;
        status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
        status->ram_usage_percent = (status->ram_used_kb * 100.0f) / status->ram_total_kb;
    } else {
        status->ram_total_kb = 0;
        status->ram_free_kb = 0;
        status->ram_used_kb = 0;
        status->ram_usage_percent = 0.0f;
    }
    
    // Inicializar valores por defecto para campos nuevos
    status->cpu_temperature = -1.0f;
    status->cpu_frequency = -1.0f;
    status->gpu_usage_percent = -1.0f;
    status->gpu_temperature = -1.0f;
    status->gpu_memory_usage_percent = -1.0f;
    strcpy(status->gpu_model, "Unknown GPU");
    status->power_consumption = -1.0f;
    status->battery_percent = -1.0f;
    status->ac_connected = -1;
    status->load_average_1min = 0.0f;
    status->load_average_5min = 0.0f;
    status->load_average_15min = 0.0f;
    status->load_1min_percent = 0.0f;
    status->load_5min_percent = 0.0f;
    status->load_15min_percent = 0.0f;
    status->total_processes = 0;
    status->running_processes = 0;
    status->sleeping_processes = 0;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
    status->num_disks = 0;
    status->num_network_interfaces = 0;
    status->num_sensors = 0;
    status->num_cores = 0;
    status->uptime_seconds = 0;
    status->idle_time_seconds = 0;
    
    // Inicializar estructuras detalladas
    memset(&status->detailed_ram, 0, sizeof(DetailedRamInfo));
    memset(&status->motherboard, 0, sizeof(MotherboardInfo));
    
    // Timestamp
    status->timestamp = time(NULL);
}
