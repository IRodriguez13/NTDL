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
#include <mntent.h>
#include <ctype.h>

// Variables globales para calcular uso de CPU
static unsigned long last_total = 0;
static unsigned long last_idle = 0;

// Declaración manual de getloadavg
extern int getloadavg(double loadavg[], int nelem);

// Función segura para leer archivos del sistema
char *read_file_content_safe(const char *path)
{
    if (!path)
        return NULL;

    FILE *file = fopen(path, "r");
    if (!file)
        return NULL;

    // Obtener tamaño del archivo de forma segura
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    long length = ftell(file);
    if (length < 0 || length > 1024 * 1024)
    { // Limitar a 1MB para seguridad
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    char *content = malloc(length + 1);
    if (!content)
    {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(content, 1, length, file);
    fclose(file);

    content[bytes_read] = '\0';

    // Remover newline al final de forma segura
    if (bytes_read > 0 && content[bytes_read - 1] == '\n')
    {
        content[bytes_read - 1] = '\0';
    }

    return content;
}

// Inicializar variables de CPU para el cálculo de uso
void init_cpu_usage()
{
    FILE *fp = fopen("/proc/stat", "r");
    if (fp)
    {
        unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
        if (fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 8)
        {
            last_total = user + nice + system + idle + iowait + irq + softirq + steal;
            last_idle = idle;
        }
        fclose(fp);
    }
}

// Obtener uso de CPU global
float get_cpu_usage()
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp)
        return 0.0f;

    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    if (fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8)
    {
        fclose(fp);
        return 0.0f;
    }
    fclose(fp);

    unsigned long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long total_diff = total - last_total;
    unsigned long idle_diff = idle - last_idle;

    float cpu_usage = 0.0f;
    if (total_diff > 0)
    {
        cpu_usage = 100.0f * (1.0f - ((float)idle_diff / total_diff));
    }

    last_total = total;
    last_idle = idle;

    return cpu_usage;
}

// Obtener información básica de memoria RAM
void get_memory_info(SystemStatus *status)
{
    if (!status) {
        return;
    }
    
    // Obtener información básica de memoria usando sysinfo
    struct sysinfo si;
    if (sysinfo(&si) == 0)
    {
        status->ram_total_kb = si.totalram / 1024;
        status->ram_free_kb = si.freeram / 1024;
        status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
        if (status->ram_total_kb > 0)
        {
            status->ram_usage_percent = (status->ram_used_kb * 100.0f) / status->ram_total_kb;
        }
    }
    else
    {
        // Fallback: intentar leer desde /proc/meminfo
        FILE *fp = fopen("/proc/meminfo", "r");
        if (fp)
        {
            char line[256];
            unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
            
            while (fgets(line, sizeof(line), fp))
            {
                if (strncmp(line, "MemTotal:", 9) == 0)
                {
                    sscanf(line, "MemTotal: %lu", &mem_total);
                }
                else if (strncmp(line, "MemFree:", 8) == 0)
                {
                    sscanf(line, "MemFree: %lu", &mem_free);
                }
                else if (strncmp(line, "MemAvailable:", 13) == 0)
                {
                    sscanf(line, "MemAvailable: %lu", &mem_available);
                }
            }
            fclose(fp);
            
            status->ram_total_kb = mem_total;
            status->ram_free_kb = mem_available > 0 ? mem_available : mem_free;
            status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
            if (status->ram_total_kb > 0)
            {
                status->ram_usage_percent = (status->ram_used_kb * 100.0f) / status->ram_total_kb;
            }
        }
    }
}

// Obtener información de carga del sistema
void get_load_average(SystemStatus *status)
{
    if (!status) {
        return;
    }
    
    double loadavg[3] = {0.0, 0.0, 0.0};
    if (getloadavg(loadavg, 3) == 3)
    {
        status->load_average_1min = (float)loadavg[0];
        status->load_average_5min = (float)loadavg[1];
        status->load_average_15min = (float)loadavg[2];
        
        // Calcular porcentajes de carga (simplificado)
        status->load_1min_percent = (float)(loadavg[0] * 100.0 / status->cpu_cores);
        status->load_5min_percent = (float)(loadavg[1] * 100.0 / status->cpu_cores);
        status->load_15min_percent = (float)(loadavg[2] * 100.0 / status->cpu_cores);
    }
    else
    {
        status->load_average_1min = 0.0f;
        status->load_average_5min = 0.0f;
        status->load_average_15min = 0.0f;
        status->load_1min_percent = 0.0f;
        status->load_5min_percent = 0.0f;
        status->load_15min_percent = 0.0f;
    }
}

// Obtener información de la motherboard
void get_motherboard_info(SystemStatus *status)
{
    if (!status) {
        return;
    }
    
    // Inicializar valores por defecto
    strcpy(status->motherboard.manufacturer, "Unknown");
    strcpy(status->motherboard.model, "Unknown");
    strcpy(status->motherboard.version, "Unknown");
    strcpy(status->motherboard.serial, "Unknown");
    strcpy(status->motherboard.bios_version, "Unknown");
    strcpy(status->motherboard.chipset, "Unknown");
    
    // Leer información del DMI usando read_file_content_safe
    char *manufacturer = read_file_content_safe("/sys/class/dmi/id/board_vendor");
    if (manufacturer) {
        strncpy(status->motherboard.manufacturer, manufacturer, 63);
        status->motherboard.manufacturer[63] = '\0';
        free(manufacturer);
    }

    char *model = read_file_content_safe("/sys/class/dmi/id/board_name");
    if (model) {
        strncpy(status->motherboard.model, model, 127);
        status->motherboard.model[127] = '\0';
        free(model);
    }

    char *version = read_file_content_safe("/sys/class/dmi/id/board_version");
    if (version) {
        strncpy(status->motherboard.version, version, 31);
        status->motherboard.version[31] = '\0';
        free(version);
    }

    char *serial = read_file_content_safe("/sys/class/dmi/id/board_serial");
    if (serial) {
        strncpy(status->motherboard.serial, serial, 63);
        status->motherboard.serial[63] = '\0';
        free(serial);
    }

    char *bios = read_file_content_safe("/sys/class/dmi/id/bios_version");
    if (bios) {
        strncpy(status->motherboard.bios_version, bios, 63);
        status->motherboard.bios_version[63] = '\0';
        free(bios);
    }

    char *chipset = read_file_content_safe("/sys/class/dmi/id/chassis_type");
    if (chipset) {
        strncpy(status->motherboard.chipset, chipset, 63);
        status->motherboard.chipset[63] = '\0';
        free(chipset);
    }
}

// Obtener información detallada por núcleo
void get_core_info(SystemStatus *status)
{
    if (!status) {
        return;
    }
    
    status->num_cores = 0;
    int max_cores_to_check = (status->cpu_cores < MAX_CORES) ? status->cpu_cores : MAX_CORES;
    
    for (int i = 0; i < max_cores_to_check; i++)
    {
        status->cores[i].core_id = i;
        
        // Obtener temperatura del núcleo
        char temp_path[256];
        snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/thermal_zone%d/temp", i);
        char *temp_content = read_file_content_safe(temp_path);
        if (temp_content) {
            status->cores[i].temperature = atof(temp_content) / 1000.0f;
            free(temp_content);
        } else {
            status->cores[i].temperature = -1.0f;
        }
        
        // Obtener frecuencia del núcleo
        char freq_path[256];
        snprintf(freq_path, sizeof(freq_path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
        char *freq_content = read_file_content_safe(freq_path);
        if (freq_content) {
            status->cores[i].frequency = atof(freq_content) / 1000.0f; // Convertir de KHz a MHz
            free(freq_content);
        } else {
            status->cores[i].frequency = -1.0f;
        }
        
        // Obtener frecuencia máxima
        snprintf(freq_path, sizeof(freq_path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        freq_content = read_file_content_safe(freq_path);
        if (freq_content) {
            status->cores[i].max_frequency = atof(freq_content) / 1000.0f;
            free(freq_content);
        } else {
            status->cores[i].max_frequency = -1.0f;
        }
        
        // Obtener frecuencia mínima
        snprintf(freq_path, sizeof(freq_path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_min_freq", i);
        freq_content = read_file_content_safe(freq_path);
        if (freq_content) {
            status->cores[i].min_frequency = atof(freq_content) / 1000.0f;
            free(freq_content);
        } else {
            status->cores[i].min_frequency = -1.0f;
        }
        
        // Por ahora, uso por núcleo y voltaje se dejan en valores por defecto
        status->cores[i].usage_percent = 0.0f;
        status->cores[i].voltage = -1.0f;
        status->cores[i].load = 0.0f;
        
        status->num_cores++;
    }
    
    // Calcular temperatura y frecuencia promedio de CPU
    float total_temp = 0.0f, total_freq = 0.0f;
    int valid_temps = 0, valid_freqs = 0;

    for (int i = 0; i < status->num_cores; i++)
    {
        if (status->cores[i].temperature > 0)
        {
            total_temp += status->cores[i].temperature;
            valid_temps++;
        }
        if (status->cores[i].frequency > 0)
        {
            total_freq += status->cores[i].frequency;
            valid_freqs++;
        }
    }

    status->cpu_temperature = (valid_temps > 0) ? total_temp / valid_temps : -1.0f;
    status->cpu_frequency = (valid_freqs > 0) ? total_freq / valid_freqs : -1.0f;
}

// Función principal de recolección de métricas (versión mínima)
void collect_metrics(SystemStatus *status)
{
    // Validar puntero nulo
    if (!status) {
        fprintf(stderr, "Error: status pointer is NULL in collect_metrics\n");
        return;
    }

    // Limpiar estructura
    memset(status, 0, sizeof(SystemStatus));

    // Inicializar variables de CPU para el cálculo de uso
    init_cpu_usage();

    // 1. Información básica del sistema
    strcpy(status->os_name, "Linux");

    // Información del kernel
    struct utsname uts;
    if (uname(&uts) == 0)
    {
        strncpy(status->kernel_version, uts.release, 63);
        status->kernel_version[63] = '\0';
        strncpy(status->hostname, uts.nodename, 63);
        status->hostname[63] = '\0';
    }
    else
    {
        strcpy(status->kernel_version, "Unknown");
        strcpy(status->hostname, "Unknown");
    }

    // 2. Información básica de CPU
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp)
    {
        char line[256];
        status->cpu_model[0] = '\0';
        while (fgets(line, sizeof(line), fp))
        {
            if (strncmp(line, "model name", 10) == 0)
            {
                char *colon = strchr(line, ':');
                if (colon)
                {
                    colon++; // Saltar el ':'
                    while (*colon == ' ' || *colon == '\t')
                        colon++; // Saltar espacios
                    strncpy(status->cpu_model, colon, 127);
                    status->cpu_model[127] = '\0';
                    // Remover newline
                    status->cpu_model[strcspn(status->cpu_model, "\n")] = 0;
                    break;
                }
            }
        }
        fclose(fp);
    }
    if (status->cpu_model[0] == '\0')
    {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    status->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    status->cpu_threads = status->cpu_cores;
    status->cpu_usage_percent = get_cpu_usage();

    // 3. Obtener información de memoria RAM
    get_memory_info(status);

    // 4. Obtener información de carga del sistema
    get_load_average(status);

    // 5. Obtener información de la motherboard
    get_motherboard_info(status);

    // 6. Obtener información detallada por núcleo
    get_core_info(status);

    // Timestamp
    status->timestamp = time(NULL);
}