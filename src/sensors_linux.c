// src/sensors_linux.c
#include "sensors_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>

// Función para leer temperatura desde hwmon
int read_hwmon_temperature(const char *hwmon_path, const char *temp_file, float *temperature)
{
    char full_path[512];
    FILE *fp;
    int temp_millicelsius;

    snprintf(full_path, sizeof(full_path), "%s/%s", hwmon_path, temp_file);

    fp = fopen(full_path, "r");
    if (!fp)
    {
        return -1;
    }

    if (fscanf(fp, "%d", &temp_millicelsius) != 1)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *temperature = temp_millicelsius / 1000.0f;
    return 0;
}

// Función para leer etiqueta del sensor
int read_sensor_label(const char *hwmon_path, const char *label_file, char *label, size_t label_size)
{
    char full_path[512];
    FILE *fp;

    snprintf(full_path, sizeof(full_path), "%s/%s", hwmon_path, label_file);

    fp = fopen(full_path, "r");
    if (!fp)
    {
        return -1;
    }

    if (fgets(label, label_size, fp))
    {
        // Quitar salto de línea
        char *newline = strchr(label, '\n');
        if (newline)
            *newline = '\0';
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

// Función para detectar tipo de sensor basado en el nombre
void determine_sensor_type(const char *name, const char *label, char *type, size_t type_size)
{
    // Convertir a minúsculas para comparación
    char lower_name[128], lower_label[128];
    strncpy(lower_name, name, sizeof(lower_name) - 1);
    strncpy(lower_label, label, sizeof(lower_label) - 1);

    for (int i = 0; lower_name[i]; i++)
    {
        lower_name[i] = tolower(lower_name[i]);
    }
    for (int i = 0; lower_label[i]; i++)
    {
        lower_label[i] = tolower(lower_label[i]);
    }

    if (strstr(lower_name, "coretemp") || strstr(lower_label, "core") || strstr(lower_label, "cpu"))
    {
        strncpy(type, "CPU", type_size - 1);
    }
    else if (strstr(lower_name, "amdgpu") || strstr(lower_name, "radeon") || strstr(lower_name, "nouveau") || strstr(lower_label, "gpu"))
    {
        strncpy(type, "GPU", type_size - 1);
    }
    else if (strstr(lower_name, "acpi") || strstr(lower_label, "motherboard") || strstr(lower_label, "system"))
    {
        strncpy(type, "Motherboard", type_size - 1);
    }
    else if (strstr(lower_name, "nvme") || strstr(lower_name, "ata") || strstr(lower_label, "disk") || strstr(lower_label, "drive"))
    {
        strncpy(type, "Storage", type_size - 1);
    }
    else
    {
        strncpy(type, "Other", type_size - 1);
    }
}

// Función principal para obtener información de sensores
TemperatureInfo *alloc_temperature_info()
{
    TemperatureInfo *info = (TemperatureInfo *)malloc(sizeof(TemperatureInfo));
    if (!info)
    {
        perror("malloc failed for TemperatureInfo");
        return NULL;
    }

    memset(info, 0, sizeof(TemperatureInfo));

    glob_t glob_result;
    int sensor_count = 0;

    // Buscar todos los dispositivos hwmon
    int glob_status = glob("/sys/class/hwmon/hwmon*", GLOB_NOSORT, NULL, &glob_result);
    if (glob_status != 0)
    {
        printf("DEBUG: No se encontraron dispositivos hwmon\n");
        free(info);
        return NULL;
    }

    for (size_t i = 0; i < glob_result.gl_pathc && sensor_count < MAX_SENSORS; i++)
    {
        char name_path[512];
        char name[64] = "Unknown";
        FILE *fp;

        // Obtener nombre del sensor
        snprintf(name_path, sizeof(name_path), "%s/name", glob_result.gl_pathv[i]);
        fp = fopen(name_path, "r");
        if (fp)
        {
            if (fgets(name, sizeof(name), fp))
            {
                char *newline = strchr(name, '\n');
                if (newline)
                    *newline = '\0';
            }
            fclose(fp);
        }

        printf("DEBUG: Procesando hwmon: %s (name: %s)\n", glob_result.gl_pathv[i], name);

        // Buscar archivos de temperatura en este hwmon
        glob_t temp_glob;
        char temp_pattern[512];
        snprintf(temp_pattern, sizeof(temp_pattern), "%s/temp*_input", glob_result.gl_pathv[i]);

        if (glob(temp_pattern, GLOB_NOSORT, NULL, &temp_glob) == 0)
        {
            for (size_t j = 0; j < temp_glob.gl_pathc && sensor_count < MAX_SENSORS; j++)
            {
                TemperatureSensor *sensor = &info->sensors[sensor_count];
                float temperature;

                // Leer temperatura
                if (read_hwmon_temperature(glob_result.gl_pathv[i],
                                           strrchr(temp_glob.gl_pathv[j], '/') + 1,
                                           &temperature) == 0)
                {

                    // Extraer número del sensor (temp1_input -> 1)
                    char *temp_file = strrchr(temp_glob.gl_pathv[j], '/') + 1;
                    int temp_num = 0;
                    sscanf(temp_file, "temp%d_input", &temp_num);

                    // Intentar obtener etiqueta específica
                    char label[64];
                    char label_file[32];
                    snprintf(label_file, sizeof(label_file), "temp%d_label", temp_num);

                    if (read_sensor_label(glob_result.gl_pathv[i], label_file, label, sizeof(label)) != 0)
                    {
                        // Si no hay etiqueta específica, usar nombre genérico
                        if (temp_num == 1)
                        {
                            snprintf(label, sizeof(label), "%s", name);
                        }
                        else
                        {
                            snprintf(label, sizeof(label), "%s Temp%d", name, temp_num);
                        }
                    }

                    // Llenar información del sensor
                    strncpy(sensor->sensor_name, label, sizeof(sensor->sensor_name) - 1);
                    determine_sensor_type(name, label, sensor->sensor_type, sizeof(sensor->sensor_type));
                    sensor->temperature = temperature;
                    sensor->max_temperature = -1.0f;      // Podríamos leer temp*_max
                    sensor->critical_temperature = -1.0f; // Podríamos leer temp*_crit

                    // Intentar leer temperaturas máximas y críticas
                    char max_file[32], crit_file[32];
                    float max_temp, crit_temp;

                    snprintf(max_file, sizeof(max_file), "temp%d_max", temp_num);
                    if (read_hwmon_temperature(glob_result.gl_pathv[i], max_file, &max_temp) == 0)
                    {
                        sensor->max_temperature = max_temp;
                    }

                    snprintf(crit_file, sizeof(crit_file), "temp%d_crit", temp_num);
                    if (read_hwmon_temperature(glob_result.gl_pathv[i], crit_file, &crit_temp) == 0)
                    {
                        sensor->critical_temperature = crit_temp;
                    }

                    printf("DEBUG: Sensor añadido: %s (%s) = %.1f°C\n",
                           sensor->sensor_name, sensor->sensor_type, sensor->temperature);

                    sensor_count++;
                }
            }
            globfree(&temp_glob);
        }
    }

    globfree(&glob_result);

    info->sensor_count = sensor_count;
    printf("DEBUG: Total sensores encontrados: %d\n", sensor_count);

    return info;
}

// Función para liberar memoria de información de temperatura
void free_temperature_info(TemperatureInfo *info)
{
    if (info)
    {
        free(info);
    }
}

// Función para imprimir información de temperatura
void print_temperature_info(const TemperatureInfo *info)
{
    if (!info || info->sensor_count == 0)
    {
        printf("\n==== Temperature Sensors ====\n");
        printf("No temperature sensors found\n");
        printf("=============================\n");
        return;
    }

    printf("\n==== Temperature Sensors ====\n");
    printf("Found %d temperature sensors:\n\n", info->sensor_count);

    // Agrupar por tipo
    const char *types[] = {"CPU", "GPU", "Motherboard", "Storage", "Other"};
    int type_count = sizeof(types) / sizeof(types[0]);

    for (int t = 0; t < type_count; t++)
    {
        int found_type = 0;

        for (int i = 0; i < info->sensor_count; i++)
        {
            if (strcmp(info->sensors[i].sensor_type, types[t]) == 0)
            {
                if (!found_type)
                {
                    printf("--- %s Sensors ---\n", types[t]);
                    found_type = 1;
                }

                const TemperatureSensor *sensor = &info->sensors[i];
                printf("  %s: %.1f°C", sensor->sensor_name, sensor->temperature);

                if (sensor->max_temperature > 0)
                {
                    printf(" (max: %.1f°C)", sensor->max_temperature);
                }
                if (sensor->critical_temperature > 0)
                {
                    printf(" (crit: %.1f°C)", sensor->critical_temperature);
                }
                printf("\n");
            }
        }

        if (found_type)
        {
            printf("\n");
        }
    }

    printf("=============================\n");
}