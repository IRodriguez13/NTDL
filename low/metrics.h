#ifndef METRICS_H
#define METRICS_H

#define MAX_CORES 64
#define MAX_SENSORS 32
#define MAX_DISKS 8
#define MAX_NETWORK_INTERFACES 16

typedef struct 
{
    int core_id;
    float temperature;      // Temperatura en °C
    float frequency;        // Frecuencia en MHz
    float voltage;          // Voltaje en V
    float usage_percent;    // Uso en porcentaje
    int load;              // Carga del núcleo
    float max_frequency;    // Frecuencia máxima en MHz
    float min_frequency;    // Frecuencia mínima en MHz
} CoreInfo;

typedef struct {
    char name[64];
    char type[32];         // "temperature", "voltage", "fan", "power"
    float value;
    char unit[16];         // "°C", "V", "RPM", "W"
    int critical;          // 1 si está en nivel crítico
} SensorInfo;

typedef struct {
    char manufacturer[64];
    char model[128];
    char version[32];
    char serial[64];
    char bios_version[64];
    char chipset[64];
} MotherboardInfo;

typedef struct {
    char name[64];
    char model[128];
    char serial[64];
    char protocol[16];     // SATA, NVMe, SSD, HDD, etc.
    char device_path[64];  // /dev/sda, /dev/nvme0n1, etc.
    unsigned long total_kb;
    unsigned long free_kb;
    unsigned long used_kb;
    float usage_percent;
    int health;            // Salud del disco (0-100)
    float temperature;     // Temperatura en °C (si está disponible)
    char filesystem[32];
    char mount_point[256];
    
    // Información de integridad
    unsigned long sectors_read;
    unsigned long sectors_written;
    unsigned long read_errors;
    unsigned long write_errors;
    unsigned long power_on_hours;
    unsigned long power_cycles;
    int smart_status;      // 0=OK, 1=Warning, 2=Critical
} DiskInfo;

typedef struct {
    int slot_id;
    char manufacturer[64];
    char part_number[64];
    char serial_number[64];
    unsigned long size_mb;     // Tamaño en MB
    int frequency_mhz;         // Frecuencia en MHz
    char type[16];             // DDR3, DDR4, DDR5, etc.
    int voltage_mv;            // Voltaje en mV
    char form_factor[32];      // DIMM, SO-DIMM, etc.
} RamStickInfo;

typedef struct 
{
    char architecture[16];     // DDR3, DDR4, DDR5, etc.
    unsigned long total_kb;
    unsigned long free_kb;
    unsigned long used_kb;
    unsigned long available_kb;
    unsigned long cached_kb;
    unsigned long buffers_kb;
    unsigned long shared_kb;
    unsigned long slab_kb;
    float usage_percent;
    int frequency_mhz;         // Frecuencia de RAM
    int num_sticks;            // Número de módulos instalados
    RamStickInfo sticks[16];   // Información de cada módulo
    
    // Información de caché por nivel
    unsigned long l1_cache_kb;
    unsigned long l2_cache_kb;
    unsigned long l3_cache_kb;
} DetailedRamInfo;

typedef struct 
{
    char name[32];
    char mac_address[18];
    char ip_address[16];
    char netmask[16];
    char gateway[16];
    char dns[16];
    char driver[64];
    char type[16];         // ethernet, wifi, loopback, etc.
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long rx_errors;
    unsigned long tx_errors;
    unsigned long rx_dropped;
    unsigned long tx_dropped;
    float rx_mbps;
    float tx_mbps;
    int status;            // 0=down, 1=up
    int speed_mbps;        // Velocidad de la interfaz
    int duplex;            // 0=half, 1=full
} NetworkInfo;

typedef struct {
    char os_name[32];
    char os_version[64];
    char kernel_version[64];
    char hostname[64];
    char cpu_model[128];
    int cpu_cores;
    int cpu_threads;
    float cpu_usage_percent;
    float cpu_temperature;     // Temperatura promedio de la CPU
    float cpu_frequency;       // Frecuencia promedio en MHz
    unsigned long ram_total_kb;
    unsigned long ram_free_kb;
    unsigned long ram_used_kb;
    float ram_usage_percent;
    
    // Información detallada de RAM
    DetailedRamInfo detailed_ram;
    char gpu_model[128];
    char gpu_driver[64];
    char gpu_vendor[32];
    unsigned long gpu_memory_total_mb;
    unsigned long gpu_memory_used_mb;
    unsigned long gpu_memory_free_mb;
    float gpu_usage_percent;
    float gpu_temperature;
    float gpu_memory_usage_percent;
    int gpu_fan_speed;     // RPM del ventilador
    int gpu_power_usage_w; // Consumo de energía en Watts
    int gpu_clock_mhz;     // Frecuencia del GPU
    int gpu_memory_clock_mhz; // Frecuencia de memoria
    
    // Información detallada por núcleo
    CoreInfo cores[MAX_CORES];
    int num_cores;
    
    // Sensores del sistema
    SensorInfo sensors[MAX_SENSORS];
    int num_sensors;
    
    // Información de la motherboard
    MotherboardInfo motherboard;
    
    // Información de discos
    DiskInfo disks[MAX_DISKS];
    int num_disks;
    
    // Información de red
    NetworkInfo network_interfaces[MAX_NETWORK_INTERFACES];
    int num_network_interfaces;
    
    // Información de energía
    float power_consumption;   // Consumo de energía en Watts
    float battery_percent;     // Porcentaje de batería (si aplica)
    int ac_connected;          // 1 si está conectado a AC
    
    // Información de carga del sistema
    float load_average_1min;
    float load_average_5min;
    float load_average_15min;
    
    // Información de procesos
    int total_processes;
    int running_processes;
    int sleeping_processes;
    int stopped_processes;
    int zombie_processes;
    
    // Timestamp y tiempo de sesión
    unsigned long timestamp;
    unsigned long uptime_seconds;    // Tiempo total de encendido en segundos
    unsigned long idle_time_seconds; // Tiempo total idle en segundos
    float load_1min_percent;         // Carga promedio 1 min como porcentaje
    float load_5min_percent;         // Carga promedio 5 min como porcentaje
    float load_15min_percent;        // Carga promedio 15 min como porcentaje
} SystemStatus;

void collect_metrics(SystemStatus *status);

#endif
