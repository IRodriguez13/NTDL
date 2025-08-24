# API Reference - SysMon

## 📋 Índice

1. [Inicialización y Limpieza](#inicialización-y-limpieza)
2. [Recolección de Métricas](#recolección-de-métricas)
3. [Información del Sistema](#información-del-sistema)
4. [CPU](#cpu)
5. [Memoria RAM](#memoria-ram)
6. [GPU](#gpu)
7. [Discos](#discos)
8. [Red](#red)
9. [Sensores](#sensores)
10. [Motherboard](#motherboard)
11. [Energía](#energía)
12. [Carga del Sistema](#carga-del-sistema)
13. [Procesos](#procesos)
14. [Tiempo](#tiempo)
15. [Utilidades](#utilidades)

## 🔧 Inicialización y Limpieza

### `sysmon_init()`
Inicializa la biblioteca sysmon.

**Prototipo:**
```c
int sysmon_init(void);
```

**Retorna:**
- `0` en éxito
- `-1` en error

**Descripción:**
Esta función debe ser llamada antes de usar cualquier otra función de la biblioteca. Configura las estructuras internas necesarias para recolectar métricas del sistema.

**Ejemplo:**
```c
if (sysmon_init() != 0) {
    fprintf(stderr, "Error al inicializar sysmon\n");
    return -1;
}
```

### `sysmon_cleanup()`
Limpia los recursos de la biblioteca sysmon.

**Prototipo:**
```c
void sysmon_cleanup(void);
```

**Descripción:**
Libera memoria y recursos utilizados por la biblioteca. Debe ser llamada al finalizar el uso de la biblioteca.

**Ejemplo:**
```c
sysmon_cleanup();
```

## 📊 Recolección de Métricas

### `sysmon_collect_metrics()`
Recolecta todas las métricas del sistema.

**Prototipo:**
```c
int sysmon_collect_metrics(SystemStatus *status);
```

**Parámetros:**
- `status`: Puntero a estructura `SystemStatus` donde se almacenarán las métricas

**Retorna:**
- `0` en éxito
- `-1` en error

**Descripción:**
Esta es la función principal que recolecta toda la información del sistema: CPU, RAM, discos, red, GPU, sensores, etc.

**Ejemplo:**
```c
SystemStatus status;
if (sysmon_collect_metrics(&status) == 0) {
    printf("CPU Usage: %.2f%%\n", status.cpu_usage_percent);
    printf("RAM Usage: %.2f%%\n", status.ram_usage_percent);
}
```

## 💻 Información del Sistema

### `sysmon_get_os_name()`
Obtiene el nombre del sistema operativo.

**Prototipo:**
```c
const char* sysmon_get_os_name(void);
```

**Retorna:**
- Puntero a string con el nombre del OS (ej: "Linux", "Windows", "macOS")

### `sysmon_get_os_version()`
Obtiene la versión del sistema operativo.

**Prototipo:**
```c
const char* sysmon_get_os_version(void);
```

**Retorna:**
- Puntero a string con la versión del OS (ej: "6.8.0-78-generic", "10.0.19045", "14.0")

### `sysmon_get_kernel_version()`
Obtiene la versión del kernel.

**Prototipo:**
```c
const char* sysmon_get_kernel_version(void);
```

**Retorna:**
- Puntero a string con la versión del kernel

### `sysmon_get_hostname()`
Obtiene el hostname del sistema.

**Prototipo:**
```c
const char* sysmon_get_hostname(void);
```

**Retorna:**
- Puntero a string con el hostname

## 🖥️ CPU

### `sysmon_get_cpu_model()`
Obtiene el modelo de CPU.

**Prototipo:**
```c
const char* sysmon_get_cpu_model(void);
```

**Retorna:**
- Puntero a string con el modelo de CPU

### `sysmon_get_cpu_cores()`
Obtiene el número de núcleos físicos.

**Prototipo:**
```c
int sysmon_get_cpu_cores(void);
```

**Retorna:**
- Número de núcleos CPU

### `sysmon_get_cpu_threads()`
Obtiene el número de threads lógicos.

**Prototipo:**
```c
int sysmon_get_cpu_threads(void);
```

**Retorna:**
- Número de threads CPU

### `sysmon_get_cpu_usage_percent()`
Obtiene el porcentaje de uso de CPU.

**Prototipo:**
```c
float sysmon_get_cpu_usage_percent(void);
```

**Retorna:**
- Porcentaje de uso (0.0 - 100.0)

### `sysmon_get_cpu_temperature()`
Obtiene la temperatura promedio de la CPU.

**Prototipo:**
```c
float sysmon_get_cpu_temperature(void);
```

**Retorna:**
- Temperatura en grados Celsius (-1.0 si no disponible)

### `sysmon_get_cpu_frequency()`
Obtiene la frecuencia promedio de la CPU.

**Prototipo:**
```c
float sysmon_get_cpu_frequency(void);
```

**Retorna:**
- Frecuencia en MHz (-1.0 si no disponible)

### `sysmon_get_core_info()`
Obtiene información de un núcleo específico.

**Prototipo:**
```c
const CoreInfo* sysmon_get_core_info(int core_id);
```

**Parámetros:**
- `core_id`: ID del núcleo (0 a num_cores-1)

**Retorna:**
- Puntero a estructura `CoreInfo`, NULL si no válido

**Estructura CoreInfo:**
```c
typedef struct {
    int core_id;              // ID del núcleo
    float temperature;        // Temperatura en °C
    float frequency;          // Frecuencia en MHz
    float voltage;            // Voltaje en V
    float usage_percent;      // Uso en porcentaje
    int load;                 // Carga del núcleo
    float max_frequency;      // Frecuencia máxima en MHz
    float min_frequency;      // Frecuencia mínima en MHz
} CoreInfo;
```

### `sysmon_get_num_cores()`
Obtiene el número total de núcleos.

**Prototipo:**
```c
int sysmon_get_num_cores(void);
```

**Retorna:**
- Número total de núcleos

## 🧠 Memoria RAM

### `sysmon_get_ram_total_kb()`
Obtiene la memoria RAM total en KB.

**Prototipo:**
```c
unsigned long sysmon_get_ram_total_kb(void);
```

**Retorna:**
- Memoria total en KB

### `sysmon_get_ram_free_kb()`
Obtiene la memoria RAM libre en KB.

**Prototipo:**
```c
unsigned long sysmon_get_ram_free_kb(void);
```

**Retorna:**
- Memoria libre en KB

### `sysmon_get_ram_used_kb()`
Obtiene la memoria RAM usada en KB.

**Prototipo:**
```c
unsigned long sysmon_get_ram_used_kb(void);
```

**Retorna:**
- Memoria usada en KB

### `sysmon_get_ram_usage_percent()`
Obtiene el porcentaje de uso de RAM.

**Prototipo:**
```c
float sysmon_get_ram_usage_percent(void);
```

**Retorna:**
- Porcentaje de uso (0.0 - 100.0)

### `sysmon_get_detailed_ram_info()`
Obtiene información detallada de RAM.

**Prototipo:**
```c
const DetailedRamInfo* sysmon_get_detailed_ram_info(void);
```

**Retorna:**
- Puntero a estructura `DetailedRamInfo`

**Estructura DetailedRamInfo:**
```c
typedef struct {
    char type[32];            // Tipo de RAM (DDR3/DDR4/DDR5)
    unsigned long total_kb;   // Total en KB
    unsigned long used_kb;    // Usado en KB
    unsigned long free_kb;    // Libre en KB
    unsigned long cached_kb;  // Cache en KB
    unsigned long swap_total_kb;  // Swap total en KB
    unsigned long swap_used_kb;   // Swap usado en KB
    int frequency_mhz;        // Frecuencia en MHz
    int num_sticks;           // Número de módulos
    unsigned long l1_cache_kb;    // Cache L1 en KB
    unsigned long l2_cache_kb;    // Cache L2 en KB
    unsigned long l3_cache_kb;    // Cache L3 en KB
} DetailedRamInfo;
```

## 🎮 GPU

### `sysmon_get_gpu_model()`
Obtiene el modelo de GPU.

**Prototipo:**
```c
const char* sysmon_get_gpu_model(void);
```

**Retorna:**
- Puntero a string con el modelo de GPU

### `sysmon_get_gpu_driver()`
Obtiene el driver de GPU.

**Prototipo:**
```c
const char* sysmon_get_gpu_driver(void);
```

**Retorna:**
- Puntero a string con el driver de GPU

### `sysmon_get_gpu_vendor()`
Obtiene el fabricante de GPU.

**Prototipo:**
```c
const char* sysmon_get_gpu_vendor(void);
```

**Retorna:**
- Puntero a string con el fabricante de GPU

### `sysmon_get_gpu_memory_total_mb()`
Obtiene la memoria total de GPU en MB.

**Prototipo:**
```c
unsigned long sysmon_get_gpu_memory_total_mb(void);
```

**Retorna:**
- Memoria total en MB

### `sysmon_get_gpu_memory_used_mb()`
Obtiene la memoria usada de GPU en MB.

**Prototipo:**
```c
unsigned long sysmon_get_gpu_memory_used_mb(void);
```

**Retorna:**
- Memoria usada en MB

### `sysmon_get_gpu_memory_free_mb()`
Obtiene la memoria libre de GPU en MB.

**Prototipo:**
```c
unsigned long sysmon_get_gpu_memory_free_mb(void);
```

**Retorna:**
- Memoria libre en MB

### `sysmon_get_gpu_usage_percent()`
Obtiene el porcentaje de uso de GPU.

**Prototipo:**
```c
float sysmon_get_gpu_usage_percent(void);
```

**Retorna:**
- Porcentaje de uso (0.0 - 100.0)

### `sysmon_get_gpu_temperature()`
Obtiene la temperatura de GPU.

**Prototipo:**
```c
float sysmon_get_gpu_temperature(void);
```

**Retorna:**
- Temperatura en grados Celsius (-1.0 si no disponible)

### `sysmon_get_gpu_memory_usage_percent()`
Obtiene el porcentaje de uso de memoria de GPU.

**Prototipo:**
```c
float sysmon_get_gpu_memory_usage_percent(void);
```

**Retorna:**
- Porcentaje de uso de memoria (0.0 - 100.0)

### `sysmon_get_gpu_fan_speed()`
Obtiene la velocidad del ventilador de GPU en RPM.

**Prototipo:**
```c
int sysmon_get_gpu_fan_speed(void);
```

**Retorna:**
- Velocidad en RPM (-1 si no disponible)

### `sysmon_get_gpu_power_usage_w()`
Obtiene el consumo de energía de GPU en Watts.

**Prototipo:**
```c
int sysmon_get_gpu_power_usage_w(void);
```

**Retorna:**
- Consumo en Watts (-1 si no disponible)

### `sysmon_get_gpu_clock_mhz()`
Obtiene la frecuencia de clock de GPU en MHz.

**Prototipo:**
```c
int sysmon_get_gpu_clock_mhz(void);
```

**Retorna:**
- Frecuencia en MHz (-1 si no disponible)

### `sysmon_get_gpu_memory_clock_mhz()`
Obtiene la frecuencia de memoria de GPU en MHz.

**Prototipo:**
```c
int sysmon_get_gpu_memory_clock_mhz(void);
```

**Retorna:**
- Frecuencia en MHz (-1 si no disponible)

## 💾 Discos

### `sysmon_get_disk_info()`
Obtiene información de un disco específico.

**Prototipo:**
```c
const DiskInfo* sysmon_get_disk_info(int disk_id);
```

**Parámetros:**
- `disk_id`: ID del disco (0 a num_disks-1)

**Retorna:**
- Puntero a estructura `DiskInfo`, NULL si no válido

**Estructura DiskInfo:**
```c
typedef struct {
    char name[64];            // Nombre del disco
    char device_path[64];     // Ruta del dispositivo
    char protocol[16];        // Protocolo (SATA/SSD/NVMe)
    char model[128];          // Modelo del disco
    char serial[64];          // Número de serie
    unsigned long total_kb;   // Capacidad total en KB
    unsigned long free_kb;    // Espacio libre en KB
    unsigned long used_kb;    // Espacio usado en KB
    float usage_percent;      // Porcentaje de uso
    float temperature;        // Temperatura en °C
    unsigned long sectors_read;   // Sectores leídos
    unsigned long sectors_written; // Sectores escritos
    unsigned long read_errors;     // Errores de lectura
    unsigned long write_errors;    // Errores de escritura
    unsigned long power_on_hours;  // Horas de funcionamiento
    unsigned long power_cycles;    // Ciclos de encendido
    int smart_status;         // Estado SMART
    int health;              // Salud del disco (0-100)
    char mount_point[256];   // Punto de montaje
    char filesystem[32];     // Sistema de archivos
} DiskInfo;
```

### `sysmon_get_num_disks()`
Obtiene el número total de discos.

**Prototipo:**
```c
int sysmon_get_num_disks(void);
```

**Retorna:**
- Número total de discos

## 🌐 Red

### `sysmon_get_network_info()`
Obtiene información de una interfaz de red específica.

**Prototipo:**
```c
const NetworkInfo* sysmon_get_network_info(int interface_id);
```

**Parámetros:**
- `interface_id`: ID de la interfaz (0 a num_network_interfaces-1)

**Retorna:**
- Puntero a estructura `NetworkInfo`, NULL si no válido

**Estructura NetworkInfo:**
```c
typedef struct {
    char name[32];            // Nombre de la interfaz
    char mac[18];             // Dirección MAC
    char ip[16];              // Dirección IP
    char netmask[16];         // Máscara de red
    char gateway[16];         // Gateway
    char dns1[16];            // DNS primario
    char dns2[16];            // DNS secundario
    char driver[64];          // Driver de la interfaz
    char type[16];            // Tipo de interfaz
    unsigned long rx_bytes;   // Bytes recibidos
    unsigned long tx_bytes;   // Bytes transmitidos
    unsigned long rx_packets; // Paquetes recibidos
    unsigned long tx_packets; // Paquetes transmitidos
    unsigned long rx_errors;  // Errores de recepción
    unsigned long tx_errors;  // Errores de transmisión
    unsigned long rx_dropped; // Paquetes descartados en recepción
    unsigned long tx_dropped; // Paquetes descartados en transmisión
    int speed_mbps;           // Velocidad en Mbps
    int duplex;               // Modo duplex
    int mtu;                  // MTU
} NetworkInfo;
```

### `sysmon_get_num_network_interfaces()`
Obtiene el número total de interfaces de red.

**Prototipo:**
```c
int sysmon_get_num_network_interfaces(void);
```

**Retorna:**
- Número total de interfaces

## 🌡️ Sensores

### `sysmon_get_sensor_info()`
Obtiene información de un sensor específico.

**Prototipo:**
```c
const SensorInfo* sysmon_get_sensor_info(int sensor_id);
```

**Parámetros:**
- `sensor_id`: ID del sensor (0 a num_sensors-1)

**Retorna:**
- Puntero a estructura `SensorInfo`, NULL si no válido

**Estructura SensorInfo:**
```c
typedef struct {
    char name[64];            // Nombre del sensor
    char type[16];            // Tipo (temp/voltage/fan)
    float value;              // Valor del sensor
    char unit[8];             // Unidad de medida
    int critical;             // Valor crítico
    int min_value;            // Valor mínimo
    int max_value;            // Valor máximo
} SensorInfo;
```

### `sysmon_get_num_sensors()`
Obtiene el número total de sensores.

**Prototipo:**
```c
int sysmon_get_num_sensors(void);
```

**Retorna:**
- Número total de sensores

## 🔌 Motherboard

### `sysmon_get_motherboard_info()`
Obtiene información de la motherboard.

**Prototipo:**
```c
const MotherboardInfo* sysmon_get_motherboard_info(void);
```

**Retorna:**
- Puntero a estructura `MotherboardInfo`

**Estructura MotherboardInfo:**
```c
typedef struct {
    char manufacturer[64];    // Fabricante
    char model[128];          // Modelo
    char version[32];         // Versión
    char bios_version[64];    // Versión del BIOS
} MotherboardInfo;
```

## ⚡ Energía

### `sysmon_get_power_consumption()`
Obtiene el consumo de energía del sistema en Watts.

**Prototipo:**
```c
float sysmon_get_power_consumption(void);
```

**Retorna:**
- Consumo en Watts (-1.0 si no disponible)

### `sysmon_get_battery_percent()`
Obtiene el porcentaje de batería.

**Prototipo:**
```c
float sysmon_get_battery_percent(void);
```

**Retorna:**
- Porcentaje de batería (0.0 - 100.0, -1.0 si no disponible)

### `sysmon_get_ac_connected()`
Verifica si el sistema está conectado a AC.

**Prototipo:**
```c
int sysmon_get_ac_connected(void);
```

**Retorna:**
- `1` si conectado a AC
- `0` si no conectado
- `-1` si no disponible

## 📈 Carga del Sistema

### `sysmon_get_load_average_1min()`
Obtiene la carga promedio de 1 minuto.

**Prototipo:**
```c
float sysmon_get_load_average_1min(void);
```

**Retorna:**
- Carga promedio de 1 minuto

### `sysmon_get_load_average_5min()`
Obtiene la carga promedio de 5 minutos.

**Prototipo:**
```c
float sysmon_get_load_average_5min(void);
```

**Retorna:**
- Carga promedio de 5 minutos

### `sysmon_get_load_average_15min()`
Obtiene la carga promedio de 15 minutos.

**Prototipo:**
```c
float sysmon_get_load_average_15min(void);
```

**Retorna:**
- Carga promedio de 15 minutos

### `sysmon_get_load_1min_percent()`
Obtiene la carga promedio de 1 minuto como porcentaje.

**Prototipo:**
```c
float sysmon_get_load_1min_percent(void);
```

**Retorna:**
- Carga promedio de 1 minuto como porcentaje

### `sysmon_get_load_5min_percent()`
Obtiene la carga promedio de 5 minutos como porcentaje.

**Prototipo:**
```c
float sysmon_get_load_5min_percent(void);
```

**Retorna:**
- Carga promedio de 5 minutos como porcentaje

### `sysmon_get_load_15min_percent()`
Obtiene la carga promedio de 15 minutos como porcentaje.

**Prototipo:**
```c
float sysmon_get_load_15min_percent(void);
```

**Retorna:**
- Carga promedio de 15 minutos como porcentaje

## 🔄 Procesos

### `sysmon_get_total_processes()`
Obtiene el número total de procesos.

**Prototipo:**
```c
int sysmon_get_total_processes(void);
```

**Retorna:**
- Número total de procesos

### `sysmon_get_running_processes()`
Obtiene el número de procesos ejecutándose.

**Prototipo:**
```c
int sysmon_get_running_processes(void);
```

**Retorna:**
- Número de procesos ejecutándose

### `sysmon_get_sleeping_processes()`
Obtiene el número de procesos durmiendo.

**Prototipo:**
```c
int sysmon_get_sleeping_processes(void);
```

**Retorna:**
- Número de procesos durmiendo

### `sysmon_get_stopped_processes()`
Obtiene el número de procesos detenidos.

**Prototipo:**
```c
int sysmon_get_stopped_processes(void);
```

**Retorna:**
- Número de procesos detenidos

### `sysmon_get_zombie_processes()`
Obtiene el número de procesos zombie.

**Prototipo:**
```c
int sysmon_get_zombie_processes(void);
```

**Retorna:**
- Número de procesos zombie

## ⏰ Tiempo

### `sysmon_get_timestamp()`
Obtiene el timestamp de la última recolección.

**Prototipo:**
```c
unsigned long sysmon_get_timestamp(void);
```

**Retorna:**
- Timestamp en segundos desde epoch

### `sysmon_get_uptime_seconds()`
Obtiene el tiempo de actividad del sistema en segundos.

**Prototipo:**
```c
unsigned long sysmon_get_uptime_seconds(void);
```

**Retorna:**
- Tiempo de actividad en segundos

### `sysmon_get_idle_time_seconds()`
Obtiene el tiempo idle del sistema en segundos.

**Prototipo:**
```c
unsigned long sysmon_get_idle_time_seconds(void);
```

**Retorna:**
- Tiempo idle en segundos (-1 si no disponible)

## 🛠️ Utilidades

### `sysmon_get_error_string()`
Obtiene una descripción de error.

**Prototipo:**
```c
const char* sysmon_get_error_string(int error_code);
```

**Parámetros:**
- `error_code`: Código de error

**Retorna:**
- Puntero a string con la descripción del error

### `sysmon_get_version()`
Obtiene la versión de la biblioteca.

**Prototipo:**
```c
const char* sysmon_get_version(void);
```

**Retorna:**
- Puntero a string con la versión

### `sysmon_is_function_available()`
Verifica si una función está disponible en la plataforma actual.

**Prototipo:**
```c
int sysmon_is_function_available(const char* function_name);
```

**Parámetros:**
- `function_name`: Nombre de la función a verificar

**Retorna:**
- `1` si disponible
- `0` si no disponible

## 📝 Ejemplo Completo

```c
#include "extern_api.h"
#include <stdio.h>

int main() {
    // Inicializar la biblioteca
    if (sysmon_init() != 0) {
        printf("Error al inicializar sysmon\n");
        return -1;
    }
    
    // Recolectar métricas
    SystemStatus status;
    if (sysmon_collect_metrics(&status) == 0) {
        printf("=== Información del Sistema ===\n");
        printf("OS: %s %s\n", sysmon_get_os_name(), sysmon_get_os_version());
        printf("Kernel: %s\n", sysmon_get_kernel_version());
        printf("Hostname: %s\n", sysmon_get_hostname());
        
        printf("\n=== CPU ===\n");
        printf("Modelo: %s\n", sysmon_get_cpu_model());
        printf("Núcleos: %d\n", sysmon_get_cpu_cores());
        printf("Threads: %d\n", sysmon_get_cpu_threads());
        printf("Uso: %.2f%%\n", sysmon_get_cpu_usage_percent());
        printf("Temperatura: %.1f°C\n", sysmon_get_cpu_temperature());
        printf("Frecuencia: %.0f MHz\n", sysmon_get_cpu_frequency());
        
        printf("\n=== Memoria ===\n");
        printf("Total: %lu KB\n", sysmon_get_ram_total_kb());
        printf("Libre: %lu KB\n", sysmon_get_ram_free_kb());
        printf("Usado: %lu KB\n", sysmon_get_ram_used_kb());
        printf("Uso: %.2f%%\n", sysmon_get_ram_usage_percent());
        
        printf("\n=== GPU ===\n");
        printf("Modelo: %s\n", sysmon_get_gpu_model());
        printf("Driver: %s\n", sysmon_get_gpu_driver());
        printf("Uso: %.2f%%\n", sysmon_get_gpu_usage_percent());
        printf("Temperatura: %.1f°C\n", sysmon_get_gpu_temperature());
        
        printf("\n=== Discos ===\n");
        for (int i = 0; i < sysmon_get_num_disks(); i++) {
            const DiskInfo* disk = sysmon_get_disk_info(i);
            if (disk) {
                printf("Disco %d: %s (%s) - %.2f%% usado\n", 
                       i, disk->name, disk->model, disk->usage_percent);
            }
        }
        
        printf("\n=== Red ===\n");
        for (int i = 0; i < sysmon_get_num_network_interfaces(); i++) {
            const NetworkInfo* net = sysmon_get_network_info(i);
            if (net) {
                printf("Interfaz %d: %s (%s)\n", i, net->name, net->ip);
            }
        }
        
        printf("\n=== Sistema ===\n");
        printf("Carga 1min: %.2f\n", sysmon_get_load_average_1min());
        printf("Carga 5min: %.2f\n", sysmon_get_load_average_5min());
        printf("Carga 15min: %.2f\n", sysmon_get_load_average_15min());
        printf("Procesos: %d\n", sysmon_get_total_processes());
        printf("Uptime: %lu segundos\n", sysmon_get_uptime_seconds());
    }
    
    // Limpiar recursos
    sysmon_cleanup();
    return 0;
}
```
