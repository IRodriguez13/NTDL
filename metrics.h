/**
 * @file metrics.h
 * @brief Definiciones de estructuras de datos para métricas del sistema
 * @version 1.0.0
 * @date 2024
 */

#ifndef METRICS_H
#define METRICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

/* ============================================================================
 * CONSTANTES Y LÍMITES
 * ============================================================================ */

#define MAX_STRING_LEN 256
#define MAX_SHORT_STRING_LEN 64
#define MAX_CORES 128
#define MAX_DISKS 16
#define MAX_NETWORK_INTERFACES 32
#define MAX_SENSORS 64
#define MAX_GPUS 8
#define MAX_DISPLAYS 8

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE CPU
 * ============================================================================ */

typedef struct {
    int core_id;                        // ID del núcleo
    float usage_percent;                // Uso en porcentaje
    float frequency_mhz;                // Frecuencia actual en MHz
    float temperature;                  // Temperatura en °C (-1 si no disponible)
} CoreInfo;

typedef struct {
    char vendor[MAX_SHORT_STRING_LEN];          // Fabricante (Intel, AMD, etc.)
    char model[MAX_STRING_LEN];                 // Modelo del procesador
    char architecture[MAX_SHORT_STRING_LEN];    // Arquitectura (x86_64, arm64, etc.)
    int physical_cores;                         // Núcleos físicos
    int logical_cores;                          // Núcleos lógicos (threads)
    float base_frequency_mhz;                   // Frecuencia base en MHz
    float max_frequency_mhz;                    // Frecuencia máxima en MHz
    float current_frequency_mhz;                // Frecuencia actual promedio
    float usage_percent;                        // Uso total en porcentaje
    float temperature;                          // Temperatura promedio (-1 si no disponible)
    int cache_l1_kb;                           // Cache L1 en KB (-1 si no disponible)
    int cache_l2_kb;                           // Cache L2 en KB (-1 si no disponible)
    int cache_l3_kb;                           // Cache L3 en KB (-1 si no disponible)
    CoreInfo cores[MAX_CORES];                 // Información por núcleo
    int num_cores_info;                        // Número de núcleos con información
} CpuMetrics;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE MEMORIA
 * ============================================================================ */

typedef struct {
    uint64_t total_kb;                  // Memoria total en KB
    uint64_t free_kb;                   // Memoria libre en KB
    uint64_t available_kb;              // Memoria disponible en KB
    uint64_t used_kb;                   // Memoria usada en KB
    uint64_t cached_kb;                 // Memoria en caché en KB
    uint64_t buffers_kb;                // Memoria en buffers en KB
    uint64_t shared_kb;                 // Memoria compartida en KB
    float usage_percent;                // Porcentaje de uso
} DetailedRamInfo;

typedef struct {
    uint64_t total_kb;                  // Swap total en KB
    uint64_t free_kb;                   // Swap libre en KB
    uint64_t used_kb;                   // Swap usado en KB
    float usage_percent;                // Porcentaje de uso de swap
} SwapInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE GPU
 * ============================================================================ */

typedef struct {
    char name[MAX_STRING_LEN];          // Nombre del modelo
    char vendor[MAX_SHORT_STRING_LEN];  // Fabricante (NVIDIA, AMD, Intel)
    char driver_version[MAX_SHORT_STRING_LEN]; // Versión del driver
    uint64_t memory_total_mb;           // VRAM total en MB
    uint64_t memory_used_mb;            // VRAM usada en MB
    uint64_t memory_free_mb;            // VRAM libre en MB
    float memory_usage_percent;         // Porcentaje de uso de VRAM
    float usage_percent;                // Utilización de GPU en %
    float temperature;                  // Temperatura en °C (-1 si no disponible)
    int fan_speed_rpm;                  // Velocidad del ventilador en RPM (-1 si no disponible)
    int power_usage_w;                  // Consumo de energía en Watts (-1 si no disponible)
    int core_clock_mhz;                 // Frecuencia del núcleo en MHz (-1 si no disponible)
    int memory_clock_mhz;               // Frecuencia de memoria en MHz (-1 si no disponible)
} GpuMetrics;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE DISCOS
 * ============================================================================ */

typedef struct {
    char device[MAX_SHORT_STRING_LEN];  // Dispositivo (/dev/sda, C:, etc.)
    char name[MAX_STRING_LEN];          // Nombre/modelo del disco
    char filesystem[MAX_SHORT_STRING_LEN]; // Sistema de archivos
    char mount_point[MAX_STRING_LEN];   // Punto de montaje
    uint64_t total_gb;                  // Espacio total en GB
    uint64_t used_gb;                   // Espacio usado en GB
    uint64_t free_gb;                   // Espacio libre en GB
    float usage_percent;                // Porcentaje de uso
    int temperature;                    // Temperatura en °C (-1 si no disponible)
    uint64_t read_bytes;                // Bytes leídos
    uint64_t write_bytes;               // Bytes escritos
    uint32_t read_ops;                  // Operaciones de lectura
    uint32_t write_ops;                 // Operaciones de escritura
    int power_on_hours;                 // Horas de funcionamiento (-1 si no disponible)
    int is_ssd;                         // 1 si es SSD, 0 si es HDD, -1 si desconocido
} DiskInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE RED
 * ============================================================================ */

typedef struct {
    char interface[MAX_SHORT_STRING_LEN]; // Nombre de la interfaz
    char ip_address[46];                // Dirección IP (IPv4/IPv6)
    char mac_address[18];               // Dirección MAC
    int is_up;                          // 1 si está activa, 0 si no
    int is_wireless;                    // 1 si es inalámbrica, 0 si no
    uint64_t bytes_sent;                // Bytes enviados
    uint64_t bytes_recv;                // Bytes recibidos
    uint64_t packets_sent;              // Paquetes enviados
    uint64_t packets_recv;              // Paquetes recibidos
    uint32_t errors_in;                 // Errores de entrada
    uint32_t errors_out;                // Errores de salida
    uint32_t drops_in;                  // Paquetes perdidos de entrada
    uint32_t drops_out;                 // Paquetes perdidos de salida
    int speed_mbps;                     // Velocidad en Mbps (-1 si no disponible)
} NetworkInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE SENSORES
 * ============================================================================ */

typedef enum {
    SENSOR_TYPE_TEMPERATURE,
    SENSOR_TYPE_VOLTAGE,
    SENSOR_TYPE_FAN_SPEED,
    SENSOR_TYPE_POWER,
    SENSOR_TYPE_CURRENT,
    SENSOR_TYPE_HUMIDITY,
    SENSOR_TYPE_UNKNOWN
} SensorType;

typedef struct {
    char name[MAX_STRING_LEN];          // Nombre del sensor
    char label[MAX_SHORT_STRING_LEN];   // Etiqueta del sensor
    SensorType type;                    // Tipo de sensor
    float value;                        // Valor actual
    float min_value;                    // Valor mínimo (-1 si no disponible)
    float max_value;                    // Valor máximo (-1 si no disponible)
    float critical_value;               // Valor crítico (-1 si no disponible)
    char unit[16];                      // Unidad (°C, V, RPM, W, etc.)
} SensorInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE PANTALLAS
 * ============================================================================ */

typedef struct {
    char name[MAX_STRING_LEN];          // Nombre de la pantalla
    char manufacturer[MAX_SHORT_STRING_LEN]; // Fabricante
    char model[MAX_STRING_LEN];         // Modelo
    int width_pixels;                   // Ancho en píxeles
    int height_pixels;                  // Alto en píxeles
    int width_mm;                       // Ancho físico en mm
    int height_mm;                      // Alto físico en mm
    float refresh_rate_hz;              // Frecuencia de refresco en Hz
    int color_depth_bits;               // Profundidad de color en bits
    int is_primary;                     // 1 si es pantalla principal
    int is_connected;                   // 1 si está conectada
    float dpi_x;                        // DPI horizontal
    float dpi_y;                        // DPI vertical
    char connection_type[MAX_SHORT_STRING_LEN]; // Tipo de conexión (HDMI, DP, etc.)
} DisplayInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE MOTHERBOARD
 * ============================================================================ */

typedef struct {
    char manufacturer[MAX_STRING_LEN];  // Fabricante
    char model[MAX_STRING_LEN];         // Modelo
    char version[MAX_SHORT_STRING_LEN]; // Versión
    char serial_number[MAX_STRING_LEN]; // Número de serie
    char bios_vendor[MAX_STRING_LEN];   // Fabricante del BIOS
    char bios_version[MAX_SHORT_STRING_LEN]; // Versión del BIOS
    char bios_date[MAX_SHORT_STRING_LEN]; // Fecha del BIOS
} MotherboardInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE BATERÍA
 * ============================================================================ */

typedef struct {
    int is_present;                     // 1 si hay batería, 0 si no
    float percent;                      // Porcentaje de carga (0-100)
    int is_charging;                    // 1 si está cargando, 0 si no
    int is_plugged;                     // 1 si está conectado a AC, 0 si no
    int time_remaining_minutes;         // Tiempo restante en minutos (-1 si no disponible)
    float voltage;                      // Voltaje actual (-1 si no disponible)
    float current_ma;                   // Corriente en mA (-1 si no disponible)
    float power_w;                      // Potencia en W (-1 si no disponible)
    int cycle_count;                    // Número de ciclos (-1 si no disponible)
    char technology[MAX_SHORT_STRING_LEN]; // Tecnología (Li-ion, etc.)
    char manufacturer[MAX_SHORT_STRING_LEN]; // Fabricante de la batería
    char model[MAX_STRING_LEN];         // Modelo de la batería
    float design_capacity_wh;           // Capacidad de diseño en Wh
    float full_charge_capacity_wh;      // Capacidad actual máxima en Wh
    float wear_level_percent;           // Nivel de desgaste en %
} BatteryInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE AUDIO
 * ============================================================================ */

#define MAX_AUDIO_DEVICES 16

typedef struct {
    char name[MAX_STRING_LEN];          // Nombre del dispositivo
    char driver[MAX_STRING_LEN];        // Driver del dispositivo
    int is_default_playback;            // 1 si es dispositivo de reproducción por defecto
    int is_default_recording;           // 1 si es dispositivo de grabación por defecto
    int is_enabled;                     // 1 si está habilitado
    int volume_percent;                 // Volumen en % (0-100)
    int is_muted;                       // 1 si está silenciado
    int sample_rate_hz;                 // Frecuencia de muestreo en Hz
    int bit_depth;                      // Profundidad de bits
    int channels;                       // Número de canales
    char format[MAX_SHORT_STRING_LEN];  // Formato de audio (PCM, etc.)
} AudioDeviceInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE RED AVANZADA
 * ============================================================================ */

typedef struct {
    char public_ip[46];                 // IP pública (IPv4/IPv6)
    char gateway_ip[46];                // IP del gateway
    char dns_primary[46];               // DNS primario
    char dns_secondary[46];             // DNS secundario
    int ping_latency_ms;                // Latencia de ping (-1 si no disponible)
    float download_speed_mbps;          // Velocidad de descarga estimada
    float upload_speed_mbps;            // Velocidad de subida estimada
    char connection_type[MAX_SHORT_STRING_LEN]; // Tipo de conexión (Ethernet, WiFi, etc.)
    int signal_strength_percent;        // Fuerza de señal WiFi (0-100, -1 si no aplica)
    char wifi_ssid[MAX_STRING_LEN];     // SSID de WiFi (si aplica)
} NetworkAdvancedInfo;

/* ============================================================================
 * ESTRUCTURAS DE INFORMACIÓN DE MEMORIA AVANZADA
 * ============================================================================ */

#define MAX_MEMORY_SLOTS 8

typedef struct {
    char memory_type[MAX_SHORT_STRING_LEN]; // DDR3, DDR4, DDR5, etc.
    int memory_speed_mhz;               // Velocidad de la RAM en MHz
    int memory_slots_used;              // Slots de memoria ocupados
    int memory_slots_total;             // Total de slots disponibles
    uint64_t memory_per_slot_mb[MAX_MEMORY_SLOTS]; // Memoria por slot en MB
    char slot_manufacturer[MAX_MEMORY_SLOTS][MAX_SHORT_STRING_LEN]; // Fabricante por slot
    char slot_part_number[MAX_MEMORY_SLOTS][MAX_STRING_LEN]; // Número de parte por slot
    int memory_voltage_mv;              // Voltaje de la memoria en mV
    int memory_timings[4];              // Timings principales (CL, tRCD, tRP, tRAS)
} MemoryAdvancedInfo;

/* ============================================================================
 * ESTRUCTURAS DE SENSORES AVANZADOS
 * ============================================================================ */

typedef struct {
    // Temperaturas específicas
    float cpu_package_temp;             // Temperatura del package CPU
    float cpu_core_max_temp;            // Temperatura máxima de núcleos
    float gpu_hotspot_temp;             // Punto más caliente de GPU
    float gpu_memory_temp;              // Temperatura memoria GPU
    float nvme_temp[4];                 // Temperatura SSDs NVMe
    float motherboard_temp;             // Temperatura placa base
    float vrm_temp;                     // Temperatura VRM
    float chipset_temp;                 // Temperatura chipset
    
    // Ventiladores
    int cpu_fan_rpm;                    // Ventilador CPU
    int case_fan_rpm[6];                // Ventiladores de caja
    int gpu_fan_rpm[3];                 // Ventiladores GPU
    int psu_fan_rpm;                    // Ventilador fuente
    
    // Voltajes detallados
    float cpu_vcore;                    // Voltaje núcleo CPU
    float cpu_vccio;                    // Voltaje I/O CPU
    float cpu_vccsa;                    // Voltaje SA CPU
    float dram_voltage;                 // Voltaje RAM
    float gpu_vcore;                    // Voltaje núcleo GPU
    float psu_12v;                      // Línea +12V fuente
    float psu_5v;                       // Línea +5V fuente
    float psu_3v3;                      // Línea +3.3V fuente
    
    // Potencias
    float cpu_power_w;                  // Consumo CPU en W
    float gpu_power_w;                  // Consumo GPU en W
    float system_power_w;               // Consumo total sistema en W
} AdvancedSensorsInfo;

/* ============================================================================
 * ESTRUCTURA PRINCIPAL DEL SISTEMA
 * ============================================================================ */

typedef struct {
    // Información básica del sistema
    char os_name[MAX_SHORT_STRING_LEN];         // Nombre del SO
    char os_version[MAX_STRING_LEN];            // Versión del SO
    char kernel_version[MAX_STRING_LEN];        // Versión del kernel
    char hostname[MAX_STRING_LEN];              // Nombre del host
    char architecture[MAX_SHORT_STRING_LEN];    // Arquitectura del sistema
    
    // Timestamp
    time_t timestamp;                           // Timestamp de la recolección
    uint64_t uptime_seconds;                    // Tiempo de actividad en segundos
    uint64_t idle_time_seconds;                 // Tiempo idle en segundos
    
    // Métricas de CPU
    CpuMetrics cpu;
    
    // Métricas de memoria
    DetailedRamInfo ram;
    SwapInfo swap;
    
    // Métricas de GPU
    GpuMetrics gpus[MAX_GPUS];
    int num_gpus;
    
    // Métricas de discos
    DiskInfo disks[MAX_DISKS];
    int num_disks;
    
    // Métricas de red
    NetworkInfo network_interfaces[MAX_NETWORK_INTERFACES];
    int num_network_interfaces;
    
    // Sensores
    SensorInfo sensors[MAX_SENSORS];
    int num_sensors;
    
    // Información de motherboard
    MotherboardInfo motherboard;
    
    // Información de batería
    BatteryInfo battery;
    
    // Información de pantallas
    DisplayInfo displays[MAX_DISPLAYS];
    int num_displays;
    
    // Información de audio
    AudioDeviceInfo audio_devices[MAX_AUDIO_DEVICES];
    int num_audio_devices;
    
    // Información de red avanzada
    NetworkAdvancedInfo network_advanced;
    
    // Información de memoria avanzada
    MemoryAdvancedInfo memory_advanced;
    
    // Sensores avanzados
    AdvancedSensorsInfo advanced_sensors;
    
    // Carga del sistema
    float load_average_1min;                    // Carga promedio 1 minuto
    float load_average_5min;                    // Carga promedio 5 minutos
    float load_average_15min;                   // Carga promedio 15 minutos
    float load_1min_percent;                    // Carga 1 min como porcentaje
    float load_5min_percent;                    // Carga 5 min como porcentaje
    float load_15min_percent;                   // Carga 15 min como porcentaje
    
    // Información de procesos
    int total_processes;                        // Total de procesos
    int running_processes;                      // Procesos ejecutándose
    int sleeping_processes;                     // Procesos durmiendo
    int stopped_processes;                      // Procesos detenidos
    int zombie_processes;                       // Procesos zombie
    
    // Información de energía
    float power_consumption_w;                  // Consumo total en Watts (-1 si no disponible)
    
    // Campos de compatibilidad con la API anterior
    float cpu_usage_percent;                    // Uso de CPU (compatibilidad)
    float ram_usage_percent;                    // Uso de RAM (compatibilidad)
    char cpu_model[MAX_STRING_LEN];             // Modelo de CPU (compatibilidad)
    char cpu_vendor[MAX_SHORT_STRING_LEN];      // Vendor de CPU (compatibilidad)
    int cpu_cores;                              // Núcleos de CPU (compatibilidad)
    float cpu_frequency_mhz;                    // Frecuencia de CPU (compatibilidad)
    float ram_total_gb;                         // RAM total en GB (compatibilidad)
    float ram_available_gb;                     // RAM disponible en GB (compatibilidad)
    float ram_used_gb;                          // RAM usada en GB (compatibilidad)
    
} SystemStatus;

#ifdef __cplusplus
}
#endif

#endif /* METRICS_H */