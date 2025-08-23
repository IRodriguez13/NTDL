#include "../metrics.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <dirent.h>

// Variables globales para calcular uso de CPU
static unsigned long last_total = 0;
static unsigned long last_idle = 0;

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

void get_disk_info(char *model, int *health) {
    // Intentar obtener información del primer disco
    DIR *dir = opendir("/sys/block");
    if (!dir) {
        strcpy(model, "Unknown Disk");
        *health = -1;
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "sd", 2) == 0 || 
            strncmp(entry->d_name, "hd", 2) == 0 ||
            strncmp(entry->d_name, "nvme", 4) == 0) {
            
            char path[256];
            snprintf(path, sizeof(path), "/sys/block/%s/device/model", entry->d_name);
            
            FILE *fp = fopen(path, "r");
            if (fp) {
                if (fgets(model, 128, fp)) {
                    // Remover newline
                    model[strcspn(model, "\n")] = 0;
                } else {
                    strcpy(model, "Generic Disk");
                }
                fclose(fp);
            } else {
                strcpy(model, "Generic Disk");
            }
            
            // Para simplificar, asumimos que el disco está saludable
            *health = 100;
            break;
        }
    }
    closedir(dir);
    
    if (strlen(model) == 0) {
        strcpy(model, "Unknown Disk");
        *health = -1;
    }
}

void collect_metrics(SystemStatus *status) {
    strcpy(status->os_name, "Linux");

    // Obtener información de CPU
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
    status->cpu_usage_percent = get_cpu_usage();

    // Obtener información de memoria
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        status->ram_total_kb = si.totalram / 1024;
        status->ram_free_kb = si.freeram / 1024;
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
        }
    }

    // Obtener información de disco
    get_disk_info(status->disk_model, &status->disk_health);
}
