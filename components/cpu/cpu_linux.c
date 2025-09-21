/**
 * @file cpu_linux.c
 * @brief Implementación de CPU para Linux
 */

#include "cpu_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static long prev_idle = 0, prev_total = 0;
static int first_call = 1;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int read_proc_stat(long *idle, long *total)
{
    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;

    long user, nice, system, idle_time, iowait, irq, softirq, steal;
    int ret = fscanf(f, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
                     &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);
    fclose(f);

    if (ret != 8)
        return -1;

    *idle = idle_time + iowait;
    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    return 0;
}

static int read_cpu_info_from_proc(CpuMetrics *metrics)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return -1;

    char line[512];
    int core_count = 0;
    int physical_cores = 0;
    int has_vendor = 0, has_model = 0;

    while (fgets(line, sizeof(line), f))
    {
        // Contar procesadores
        if (strncmp(line, "processor", 9) == 0)
        {
            core_count++;
        }

        // Obtener vendor_id
        if (!has_vendor && strncmp(line, "vendor_id", 9) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                colon += 2; // Saltar ": "
                char *newline = strchr(colon, '\n');
                if (newline)
                    *newline = '\0';

                // Limpiar espacios al final
                int len = strlen(colon);
                while (len > 0 && colon[len - 1] == ' ')
                {
                    colon[--len] = '\0';
                }

                strncpy(metrics->vendor, colon, sizeof(metrics->vendor) - 1);
                metrics->vendor[sizeof(metrics->vendor) - 1] = '\0';
                has_vendor = 1;
            }
        }

        // Obtener model name
        if (!has_model && strncmp(line, "model name", 10) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                colon += 2; // Saltar ": "
                char *newline = strchr(colon, '\n');
                if (newline)
                    *newline = '\0';

                // Limpiar espacios al final
                int len = strlen(colon);
                while (len > 0 && colon[len - 1] == ' ')
                {
                    colon[--len] = '\0';
                }

                strncpy(metrics->model, colon, sizeof(metrics->model) - 1);
                metrics->model[sizeof(metrics->model) - 1] = '\0';
                has_model = 1;
            }
        }

        // Obtener frecuencia
        if (strncmp(line, "cpu MHz", 7) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                metrics->current_frequency_mhz = atof(colon + 2);
                if (metrics->base_frequency_mhz == 0)
                {
                    metrics->base_frequency_mhz = metrics->current_frequency_mhz;
                }
            }
        }

        // Obtener información de caché
        if (strncmp(line, "cache size", 10) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon && metrics->cache_l3_kb == -1)
            {
                int cache_kb = atoi(colon + 2);
                metrics->cache_l3_kb = cache_kb; // Asumir que es L3
            }
        }

        // Contar núcleos físicos
        if (strncmp(line, "cpu cores", 9) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                physical_cores = atoi(colon + 2);
            }
        }
    }

    fclose(f);

    metrics->logical_cores = core_count;
    metrics->physical_cores = (physical_cores > 0) ? physical_cores : core_count;

    // Valores por defecto si no se encontraron
    if (!has_vendor)
    {
        strncpy(metrics->vendor, "Unknown", sizeof(metrics->vendor) - 1);
    }
    if (!has_model)
    {
        strncpy(metrics->model, "Unknown CPU", sizeof(metrics->model) - 1);
    }

    return 0;
}

static float read_cpu_temperature(void)
{
    glob_t glob_result;
    float temperature = -1.0f;

    // Buscar sensores de temperatura de CPU
    const char *temp_patterns[] = {
        "/sys/class/thermal/thermal_zone*/temp",
        "/sys/class/hwmon/hwmon*/temp*_input",
        NULL};

    for (int i = 0; temp_patterns[i]; i++)
    {
        if (glob(temp_patterns[i], GLOB_NOSORT, NULL, &glob_result) == 0)
        {
            for (size_t j = 0; j < glob_result.gl_pathc; j++)
            {
                FILE *f = fopen(glob_result.gl_pathv[j], "r");
                if (f)
                {
                    int temp_millicelsius;
                    if (fscanf(f, "%d", &temp_millicelsius) == 1)
                    {
                        float temp_celsius = temp_millicelsius / 1000.0f;
                        if (temp_celsius > 0 && temp_celsius < 150)
                        { // Rango razonable
                            temperature = temp_celsius;
                            fclose(f);
                            globfree(&glob_result);
                            return temperature;
                        }
                    }
                    fclose(f);
                }
            }
            globfree(&glob_result);
        }
    }

    return temperature;
}

static int read_cpu_frequencies(CpuMetrics *metrics)
{
    glob_t glob_result;
    float total_freq = 0.0f;
    int freq_count = 0;

    // Leer frecuencias actuales de cada núcleo
    if (glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq", GLOB_NOSORT, NULL, &glob_result) == 0)
    {
        for (size_t i = 0; i < glob_result.gl_pathc && i < MAX_CORES; i++)
        {
            FILE *f = fopen(glob_result.gl_pathv[i], "r");
            if (f)
            {
                int freq_khz;
                if (fscanf(f, "%d", &freq_khz) == 1)
                {
                    float freq_mhz = freq_khz / 1000.0f;
                    total_freq += freq_mhz;
                    freq_count++;

                    // Guardar frecuencia por núcleo si tenemos espacio
                    if ((int)i < metrics->num_cores_info)
                    {
                        metrics->cores[i].frequency_mhz = freq_mhz;
                    }
                }
                fclose(f);
            }
        }
        globfree(&glob_result);
    }

    if (freq_count > 0)
    {
        metrics->current_frequency_mhz = total_freq / freq_count;
    }

    // Leer frecuencia máxima
    if (glob("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", GLOB_NOSORT, NULL, &glob_result) == 0)
    {
        if (glob_result.gl_pathc > 0)
        {
            FILE *f = fopen(glob_result.gl_pathv[0], "r");
            if (f)
            {
                int max_freq_khz;
                if (fscanf(f, "%d", &max_freq_khz) == 1)
                {
                    metrics->max_frequency_mhz = max_freq_khz / 1000.0f;
                }
                fclose(f);
            }
        }
        globfree(&glob_result);
    }

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int cpu_init(void)
{
    if (initialized)
        return 0;

    // Inicializar variables estáticas
    prev_idle = 0;
    prev_total = 0;
    first_call = 1;

    initialized = 1;
    return 0;
}

void cpu_cleanup(void)
{
    initialized = 0;
}

int cpu_get_metrics(CpuMetrics *metrics)
{
    if (!metrics)
        return -1;

    // Inicializar estructura
    memset(metrics, 0, sizeof(CpuMetrics));

    // Valores por defecto
    metrics->cache_l1_kb = -1;
    metrics->cache_l2_kb = -1;
    metrics->cache_l3_kb = -1;
    metrics->temperature = -1.0f;
    metrics->base_frequency_mhz = 0.0f;
    metrics->max_frequency_mhz = 0.0f;
    metrics->current_frequency_mhz = 0.0f;

    // Obtener información básica del CPU
    if (read_cpu_info_from_proc(metrics) != 0)
    {
        return -1;
    }

    // Obtener uso de CPU
    metrics->usage_percent = cpu_get_usage_percent();

    // Obtener temperatura
    metrics->temperature = read_cpu_temperature();

    // Obtener frecuencias
    read_cpu_frequencies(metrics);

    // Configurar arquitectura
    strncpy(metrics->architecture, "x86_64", sizeof(metrics->architecture) - 1);

    // Inicializar información por núcleo
    metrics->num_cores_info = (metrics->logical_cores < MAX_CORES) ? metrics->logical_cores : MAX_CORES;

    for (int i = 0; i < metrics->num_cores_info; i++)
    {
        metrics->cores[i].core_id = i;
        metrics->cores[i].usage_percent = -1.0f; // No implementado aún
        metrics->cores[i].temperature = -1.0f;   // No implementado aún
        // frequency_mhz se establece en read_cpu_frequencies
    }

    return 0;
}

int cpu_get_model(char *buffer, size_t size)
{
    if (!buffer || size == 0)
        return -1;

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return -1;

    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "model name", 10) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                colon += 2; // Saltar ": "
                char *newline = strchr(colon, '\n');
                if (newline)
                    *newline = '\0';

                // Limpiar espacios al final
                int len = strlen(colon);
                while (len > 0 && colon[len - 1] == ' ')
                {
                    colon[--len] = '\0';
                }

                strncpy(buffer, colon, size - 1);
                buffer[size - 1] = '\0';
                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return -1;
}

float cpu_get_usage_percent(void)
{
    long idle, total;

    if (read_proc_stat(&idle, &total) != 0)
    {
        return -1.0f;
    }

    // En la primera llamada, solo guardamos los valores iniciales
    if (first_call)
    {
        prev_idle = idle;
        prev_total = total;
        first_call = 0;
        return 0.0f;
    }

    long diff_idle = idle - prev_idle;
    long diff_total = total - prev_total;

    prev_idle = idle;
    prev_total = total;

    if (diff_total == 0)
    {
        return 0.0f;
    }

    float cpu_usage = (float)(diff_total - diff_idle) / diff_total * 100.0f;

    // Validar rango
    if (cpu_usage < 0.0f)
        cpu_usage = 0.0f;
    if (cpu_usage > 100.0f)
        cpu_usage = 100.0f;

    return cpu_usage;
}

float cpu_get_temperature(void)
{
    return read_cpu_temperature();
}

float cpu_get_frequency_mhz(void)
{
    // Intentar leer desde cpufreq
    FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
    if (f)
    {
        int freq_khz;
        if (fscanf(f, "%d", &freq_khz) == 1)
        {
            fclose(f);
            return freq_khz / 1000.0f;
        }
        fclose(f);
    }

    // Fallback a /proc/cpuinfo
    f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return -1.0f;

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "cpu MHz", 7) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                float freq = atof(colon + 2);
                fclose(f);
                return freq;
            }
        }
    }

    fclose(f);
    return -1.0f;
}

int cpu_get_core_info(int core_id, CoreInfo *core_info)
{
    if (!core_info || core_id < 0)
        return -1;

    core_info->core_id = core_id;
    core_info->usage_percent = -1.0f; // No implementado
    core_info->temperature = -1.0f;   // No implementado
    core_info->frequency_mhz = -1.0f;

    // Intentar obtener frecuencia del núcleo específico
    char freq_path[256];
    snprintf(freq_path, sizeof(freq_path),
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", core_id);

    FILE *f = fopen(freq_path, "r");
    if (f)
    {
        int freq_khz;
        if (fscanf(f, "%d", &freq_khz) == 1)
        {
            core_info->frequency_mhz = freq_khz / 1000.0f;
        }
        fclose(f);
    }

    return 0;
}

int cpu_get_physical_cores(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return -1;

    char line[256];
    int physical_cores = 0;

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "cpu cores", 9) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                physical_cores = atoi(colon + 2);
                break;
            }
        }
    }

    fclose(f);

    // Si no encontramos "cpu cores", contar procesadores únicos
    if (physical_cores == 0)
    {
        physical_cores = cpu_get_logical_cores();
    }

    return physical_cores;
}

int cpu_get_logical_cores(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
    {
        return -1;
    }

    char line[256];
    int logical_cores = 0;

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "processor", 9) == true)
        {
            logical_cores++;
        }
    }

    fclose(f);
    return logical_cores;
}