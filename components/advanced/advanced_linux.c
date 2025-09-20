/**
 * @file advanced_linux.c
 * @brief Implementación de componentes avanzados para Linux
 */

#include "advanced_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int sensors_initialized = 0;
static int memory_initialized = 0;
static int network_initialized = 0;

/* ============================================================================
 * IMPLEMENTACIÓN DE SENSORES AVANZADOS
 * ============================================================================ */

static float read_hwmon_sensor(const char *pattern, const char *sensor_type)
{
    glob_t glob_result;
    char search_pattern[512];

    snprintf(search_pattern, sizeof(search_pattern), "/sys/class/hwmon/hwmon*/%s", pattern);

    if (glob(search_pattern, GLOB_NOSORT, NULL, &glob_result) == 0)
    {
        for (size_t i = 0; i < glob_result.gl_pathc; i++)
        {
            FILE *f = fopen(glob_result.gl_pathv[i], "r");
            if (f)
            {
                int value;
                if (fscanf(f, "%d", &value) == 1)
                {
                    fclose(f);
                    globfree(&glob_result);
                    return value / 1000.0f; // Convertir de mili-unidades
                }
                fclose(f);
            }
        }
        globfree(&glob_result);
    }

    return -1.0f;
}

static int read_hwmon_fan(const char *pattern)
{
    glob_t glob_result;
    char search_pattern[512];

    snprintf(search_pattern, sizeof(search_pattern), "/sys/class/hwmon/hwmon*/%s", pattern);

    if (glob(search_pattern, GLOB_NOSORT, NULL, &glob_result) == 0)
    {
        for (size_t i = 0; i < glob_result.gl_pathc; i++)
        {
            FILE *f = fopen(glob_result.gl_pathv[i], "r");
            if (f)
            {
                int value;
                if (fscanf(f, "%d", &value) == 1)
                {
                    fclose(f);
                    globfree(&glob_result);
                    return value; // RPM directo
                }
                fclose(f);
            }
        }
        globfree(&glob_result);
    }

    return -1;
}

int advanced_sensors_init(void)
{
    if (sensors_initialized)
        return 0;
    sensors_initialized = 1;
    return 0;
}

void advanced_sensors_cleanup(void)
{
    sensors_initialized = 0;
}

int advanced_sensors_get_info(AdvancedSensorsInfo *sensors_info)
{
    if (!sensors_info)
        return -1;

    if (!sensors_initialized && advanced_sensors_init() != 0)
    {
        return -1;
    }

    memset(sensors_info, 0, sizeof(AdvancedSensorsInfo));

    // Temperaturas específicas
    sensors_info->cpu_package_temp = read_hwmon_sensor("temp1_input", "Package");
    sensors_info->cpu_core_max_temp = read_hwmon_sensor("temp2_input", "Core");
    sensors_info->motherboard_temp = read_hwmon_sensor("temp3_input", "Motherboard");
    sensors_info->chipset_temp = read_hwmon_sensor("temp4_input", "Chipset");

    // Temperaturas NVMe
    for (int i = 0; i < 4; i++)
    {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "temp%d_input", i + 1);
        sensors_info->nvme_temp[i] = read_hwmon_sensor(pattern, "NVMe");
    }

    // Ventiladores
    sensors_info->cpu_fan_rpm = read_hwmon_fan("fan1_input");
    sensors_info->psu_fan_rpm = read_hwmon_fan("fan2_input");

    for (int i = 0; i < 6; i++)
    {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "fan%d_input", i + 3);
        sensors_info->case_fan_rpm[i] = read_hwmon_fan(pattern);
    }

    // Voltajes detallados
    sensors_info->cpu_vcore = read_hwmon_sensor("in0_input", "Vcore");
    sensors_info->cpu_vccio = read_hwmon_sensor("in1_input", "VCCIO");
    sensors_info->dram_voltage = read_hwmon_sensor("in2_input", "DRAM");
    sensors_info->psu_12v = read_hwmon_sensor("in3_input", "12V");
    sensors_info->psu_5v = read_hwmon_sensor("in4_input", "5V");
    sensors_info->psu_3v3 = read_hwmon_sensor("in5_input", "3.3V");

    // Potencias (estimadas o desde sensores)
    sensors_info->cpu_power_w = read_hwmon_sensor("power1_input", "CPU");
    sensors_info->gpu_power_w = read_hwmon_sensor("power2_input", "GPU");
    sensors_info->system_power_w = read_hwmon_sensor("power3_input", "System");

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE MEMORIA AVANZADA
 * ============================================================================ */

int advanced_memory_init(void)
{
    if (memory_initialized)
        return 0;
    memory_initialized = 1;
    return 0;
}

void advanced_memory_cleanup(void)
{
    memory_initialized = 0;
}

int advanced_memory_get_info(MemoryAdvancedInfo *memory_info)
{
    if (!memory_info)
        return -1;

    if (!memory_initialized && advanced_memory_init() != 0)
    {
        return -1;
    }

    memset(memory_info, 0, sizeof(MemoryAdvancedInfo));

    // Intentar obtener información de DMI
    FILE *fp = popen("dmidecode -t memory 2>/dev/null", "r");
    if (fp)
    {
        char line[512];
        int current_slot = -1;

        while (fgets(line, sizeof(line), fp))
        {
            // Detectar nuevo slot de memoria
            if (strstr(line, "Memory Device"))
            {
                current_slot++;
                if (current_slot >= MAX_MEMORY_SLOTS)
                    break;
            }

            if (current_slot >= 0 && current_slot < MAX_MEMORY_SLOTS)
            {
                // Tamaño de memoria
                if (strstr(line, "Size:") && !strstr(line, "No Module"))
                {
                    char *size_str = strstr(line, ":");
                    if (size_str)
                    {
                        int size_mb = 0;
                        if (sscanf(size_str + 1, "%d MB", &size_mb) == 1)
                        {
                            memory_info->memory_per_slot_mb[current_slot] = size_mb;
                            memory_info->memory_slots_used++;
                        }
                        else if (sscanf(size_str + 1, "%d GB", &size_mb) == 1)
                        {
                            memory_info->memory_per_slot_mb[current_slot] = size_mb * 1024;
                            memory_info->memory_slots_used++;
                        }
                    }
                }

                // Tipo de memoria
                if (strstr(line, "Type:") && strlen(memory_info->memory_type) == 0)
                {
                    char *type_str = strstr(line, ":");
                    if (type_str)
                    {
                        sscanf(type_str + 1, " %31s", memory_info->memory_type);
                    }
                }

                // Velocidad
                if (strstr(line, "Speed:") && memory_info->memory_speed_mhz == 0)
                {
                    char *speed_str = strstr(line, ":");
                    if (speed_str)
                    {
                        sscanf(speed_str + 1, " %d MHz", &memory_info->memory_speed_mhz);
                    }
                }

                // Fabricante
                if (strstr(line, "Manufacturer:"))
                {
                    char *mfg_str = strstr(line, ":");
                    if (mfg_str)
                    {
                        sscanf(mfg_str + 1, " %63s", memory_info->slot_manufacturer[current_slot]);
                    }
                }

                // Número de parte
                if (strstr(line, "Part Number:"))
                {
                    char *part_str = strstr(line, ":");
                    if (part_str)
                    {
                        sscanf(part_str + 1, " %255s", memory_info->slot_part_number[current_slot]);
                    }
                }
            }
        }
        pclose(fp);
    }

    // Valores por defecto si no se pudo obtener información
    if (strlen(memory_info->memory_type) == 0)
    {
        strncpy(memory_info->memory_type, "Unknown", sizeof(memory_info->memory_type) - 1);
    }

    // Estimar slots totales (típicamente 2 o 4)
    memory_info->memory_slots_total = (memory_info->memory_slots_used <= 2) ? 4 : 8;

    // Voltaje típico
    memory_info->memory_voltage_mv = 1350; // 1.35V típico para DDR4

    // Timings por defecto (no fácilmente disponibles en Linux)
    memory_info->memory_timings[0] = -1; // CL
    memory_info->memory_timings[1] = -1; // tRCD
    memory_info->memory_timings[2] = -1; // tRP
    memory_info->memory_timings[3] = -1; // tRAS

    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE RED AVANZADA
 * ============================================================================ */

int advanced_network_init(void)
{
    if (network_initialized)
        return 0;
    network_initialized = 1;
    return 0;
}

void advanced_network_cleanup(void)
{
    network_initialized = 0;
}

int advanced_network_get_info(NetworkAdvancedInfo *network_info)
{
    if (!network_info)
        return -1;

    if (!network_initialized && advanced_network_init() != 0)
    {
        return -1;
    }

    memset(network_info, 0, sizeof(NetworkAdvancedInfo));

    // Obtener IP pública (usando servicio externo)
    FILE *fp = popen("curl -s --connect-timeout 5 ifconfig.me 2>/dev/null", "r");
    if (fp)
    {
        if (fgets(network_info->public_ip, sizeof(network_info->public_ip), fp))
        {
            // Quitar newline
            char *newline = strchr(network_info->public_ip, '\n');
            if (newline)
                *newline = '\0';
        }
        pclose(fp);
    }

    // Obtener gateway
    fp = popen("ip route | grep default | awk '{print $3}' | head -1", "r");
    if (fp)
    {
        if (fgets(network_info->gateway_ip, sizeof(network_info->gateway_ip), fp))
        {
            char *newline = strchr(network_info->gateway_ip, '\n');
            if (newline)
                *newline = '\0';
        }
        pclose(fp);
    }

    // Obtener DNS
    fp = popen("grep nameserver /etc/resolv.conf | head -1 | awk '{print $2}'", "r");
    if (fp)
    {
        if (fgets(network_info->dns_primary, sizeof(network_info->dns_primary), fp))
        {
            char *newline = strchr(network_info->dns_primary, '\n');
            if (newline)
                *newline = '\0';
        }
        pclose(fp);
    }

    fp = popen("grep nameserver /etc/resolv.conf | head -2 | tail -1 | awk '{print $2}'", "r");
    if (fp)
    {
        if (fgets(network_info->dns_secondary, sizeof(network_info->dns_secondary), fp))
        {
            char *newline = strchr(network_info->dns_secondary, '\n');
            if (newline)
                *newline = '\0';
        }
        pclose(fp);
    }

    // Test de ping (solo si tenemos gateway)
    if (strlen(network_info->gateway_ip) > 0)
    {
        char ping_cmd[256];
        snprintf(ping_cmd, sizeof(ping_cmd),
                 "ping -c 1 -W 2 %s 2>/dev/null | grep 'time=' | sed 's/.*time=\\([0-9.]*\\).*/\\1/'",
                 network_info->gateway_ip);

        fp = popen(ping_cmd, "r");
        if (fp)
        {
            char ping_result[64];
            if (fgets(ping_result, sizeof(ping_result), fp))
            {
                float ping_time = atof(ping_result);
                network_info->ping_latency_ms = (int)ping_time;
            }
            pclose(fp);
        }
    }

    // Tipo de conexión (simplificado)
    fp = popen("iwconfig 2>/dev/null | grep -o 'IEEE 802.11' | head -1", "r");
    if (fp)
    {
        char wifi_check[64];
        if (fgets(wifi_check, sizeof(wifi_check), fp))
        {
            strncpy(network_info->connection_type, "WiFi", sizeof(network_info->connection_type) - 1);

            // Obtener SSID si es WiFi
            pclose(fp);
            fp = popen("iwconfig 2>/dev/null | grep ESSID | head -1 | sed 's/.*ESSID:\"\\(.*\\)\".*/\\1/'", "r");
            if (fp)
            {
                if (fgets(network_info->wifi_ssid, sizeof(network_info->wifi_ssid), fp))
                {
                    char *newline = strchr(network_info->wifi_ssid, '\n');
                    if (newline)
                        *newline = '\0';
                }
            }

            // Fuerza de señal WiFi
            if (fp)
            {
                pclose(fp);
                fp = popen("iwconfig 2>/dev/null | grep 'Signal level' | head -1 | sed 's/.*Signal level=\\(-[0-9]*\\).*/\\1/'", "r");
                if (fp)
                {
                    char signal_str[32];
                    if (fgets(signal_str, sizeof(signal_str), fp))
                    {
                        int signal_dbm = atoi(signal_str);
                        // Convertir dBm a porcentaje (aproximado)
                        if (signal_dbm >= -50)
                        {
                            network_info->signal_strength_percent = 100;
                        }
                        else if (signal_dbm <= -100)
                        {
                            network_info->signal_strength_percent = 0;
                        }
                        else
                        {
                            network_info->signal_strength_percent = 2 * (signal_dbm + 100);
                        }
                    }
                }
            }
        }
        else
        {
            strncpy(network_info->connection_type, "Ethernet", sizeof(network_info->connection_type) - 1);
            network_info->signal_strength_percent = -1; // No aplica
        }
        pclose(fp);
    }

    // Velocidades de red (estimadas, requieren herramientas específicas)
    network_info->download_speed_mbps = -1.0f;
    network_info->upload_speed_mbps = -1.0f;

    return 0;
}