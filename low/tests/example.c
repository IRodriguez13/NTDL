#include "sysmon.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    // Inicializar el sistema de monitoreo
    if (sysmon_init() != 0) {
        printf("Error al inicializar sysmon\n");
        return 1;
    }
    
    printf("=== Monitoreo del Sistema ===\n\n");
    
    // Obtener métricas específicas
    printf("Sistema Operativo: %s\n", sysmon_get_os_name());
    printf("Kernel: %s\n", sysmon_get_kernel_version());
    printf("Hostname: %s\n", sysmon_get_hostname());
    printf("CPU Modelo: %s\n", sysmon_get_cpu_model());
    printf("Núcleos CPU: %d\n", sysmon_get_cpu_cores());
    printf("Threads CPU: %d\n", sysmon_get_cpu_threads());
    printf("Uso de CPU: %.2f%%\n", sysmon_get_cpu_usage());
    printf("Temperatura CPU: %.1f°C\n", sysmon_get_cpu_temperature());
    printf("Frecuencia CPU: %.0f MHz\n", sysmon_get_cpu_frequency());
    
    printf("\nMemoria RAM:\n");
    printf("  Total: %lu KB (%.2f GB)\n", 
           sysmon_get_ram_total(), 
           sysmon_get_ram_total() / (1024.0 * 1024.0));
    printf("  Libre: %lu KB (%.2f GB)\n", 
           sysmon_get_ram_free(), 
           sysmon_get_ram_free() / (1024.0 * 1024.0));
    printf("  Usada: %lu KB (%.2f GB)\n", 
           sysmon_get_ram_used(), 
           sysmon_get_ram_used() / (1024.0 * 1024.0));
    printf("  Uso: %.2f%%\n", sysmon_get_ram_usage_percent());
    
    printf("\nTiempo de Sesión:\n");
    unsigned long uptime = sysmon_get_timestamp(); // Placeholder por ahora
    printf("  Uptime: %lu segundos (%.1f horas)\n", uptime, uptime / 3600.0f);
    
    printf("\nGPU:\n");
    printf("  Modelo: %s\n", sysmon_get_gpu_model());
    printf("  Uso: %.1f%%\n", sysmon_get_gpu_usage());
    printf("  Temperatura: %.1f°C\n", sysmon_get_gpu_temperature());
    
    printf("\nDiscos: %d\n", sysmon_get_num_disks());
    printf("Interfaces de Red: %d\n", sysmon_get_num_network_interfaces());
    printf("Sensores: %d\n", sysmon_get_num_sensors());
    
    printf("\nCarga del Sistema:\n");
    printf("  1 min: %.2f\n", sysmon_get_load_average_1min());
    printf("  5 min: %.2f\n", sysmon_get_load_average_5min());
    printf("  15 min: %.2f\n", sysmon_get_load_average_15min());
    
    printf("\nProcesos:\n");
    printf("  Total: %d\n", sysmon_get_total_processes());
    printf("  Ejecutándose: %d\n", sysmon_get_running_processes());
    printf("  Durmiendo: %d\n", sysmon_get_sleeping_processes());
    
    // Obtener todas las métricas como estructura
    SystemStatus status;
    if (sysmon_get_metrics(&status) == 0) {
        printf("\n=== Métricas Completas ===\n");
        printf("OS: %s\n", status.os_name);
        printf("CPU: %s (%d cores, %.2f%%)\n", 
               status.cpu_model, status.cpu_cores, status.cpu_usage_percent);
        printf("RAM: %lu/%lu KB (%.2f%%)\n", 
               status.ram_used_kb,
               status.ram_total_kb,
               status.ram_usage_percent);
        printf("RAM Detallada: %s, %d MHz, %d módulos\n", 
               status.detailed_ram.architecture,
               status.detailed_ram.frequency_mhz,
               status.detailed_ram.num_sticks);
        printf("Caché: L1=%lu KB, L2=%lu KB, L3=%lu KB\n",
               status.detailed_ram.l1_cache_kb,
               status.detailed_ram.l2_cache_kb,
               status.detailed_ram.l3_cache_kb);
        printf("Uptime: %lu segundos (%.1f horas)\n", 
               status.uptime_seconds, 
               status.uptime_seconds / 3600.0f);
        printf("Discos: %d\n", status.num_disks);
        printf("Interfaces de Red: %d\n", status.num_network_interfaces);
        printf("Sensores: %d\n", status.num_sensors);
        
        // Mostrar información detallada de discos
        if (status.num_disks > 0) {
            printf("\n=== Discos ===\n");
            for (int i = 0; i < status.num_disks && i < 4; i++) {
                printf("Disco %d: %s (%s) - %s\n", i + 1, 
                       status.disks[i].model, 
                       status.disks[i].protocol, 
                       status.disks[i].device_path);
                printf("  Capacidad: %.2f GB, Uso: %.1f%%, Salud: %d%%\n", 
                       status.disks[i].total_kb / (1024.0 * 1024.0), 
                       status.disks[i].usage_percent, 
                       status.disks[i].health);
                if (status.disks[i].temperature > 0) {
                    printf("  Temperatura: %.1f°C\n", status.disks[i].temperature);
                }
                if (status.disks[i].power_on_hours > 0) {
                    printf("  Horas de uso: %lu, Ciclos: %lu\n", 
                           status.disks[i].power_on_hours, 
                           status.disks[i].power_cycles);
                }
                printf("  Montado en: %s (%s)\n", 
                       status.disks[i].mount_point, 
                       status.disks[i].filesystem);
            }
        }
        
        // Mostrar información detallada de red
        if (status.num_network_interfaces > 0) {
            printf("\n=== Interfaces de Red ===\n");
            for (int i = 0; i < status.num_network_interfaces && i < 4; i++) {
                printf("Interfaz %d: %s (%s) - %s\n", i + 1, 
                       status.network_interfaces[i].name, 
                       status.network_interfaces[i].type, 
                       status.network_interfaces[i].driver);
                printf("  MAC: %s, Estado: %s\n", 
                       status.network_interfaces[i].mac_address,
                       status.network_interfaces[i].status ? "UP" : "DOWN");
                if (status.network_interfaces[i].speed_mbps > 0) {
                    printf("  Velocidad: %d Mbps\n", status.network_interfaces[i].speed_mbps);
                }
                printf("  RX: %.2f MB/s, TX: %.2f MB/s\n", 
                       status.network_interfaces[i].rx_mbps / 8.0f,
                       status.network_interfaces[i].tx_mbps / 8.0f);
            }
        }
        
        // Mostrar información detallada de GPU
        printf("\n=== GPU ===\n");
        printf("Modelo: %s (%s)\n", status.gpu_model, status.gpu_vendor);
        printf("Driver: %s\n", status.gpu_driver);
        if (status.gpu_usage_percent >= 0) {
            printf("Uso: %.1f%%, Temperatura: %.1f°C\n", 
                   status.gpu_usage_percent, status.gpu_temperature);
        }
        if (status.gpu_memory_total_mb > 0) {
            printf("Memoria: %lu/%lu MB (%.1f%%)\n", 
                   status.gpu_memory_used_mb,
                   status.gpu_memory_total_mb,
                   status.gpu_memory_usage_percent);
        }
        if (status.gpu_clock_mhz > 0) {
            printf("Frecuencia: %d MHz, Memoria: %d MHz\n", 
                   status.gpu_clock_mhz, status.gpu_memory_clock_mhz);
        }
        if (status.gpu_power_usage_w > 0) {
            printf("Consumo: %d W\n", status.gpu_power_usage_w);
        }
        if (status.gpu_fan_speed > 0) {
            printf("Ventilador: %d RPM\n", status.gpu_fan_speed);
        }
        
        // Mostrar información detallada de módulos de RAM
        if (status.detailed_ram.num_sticks > 0) {
            printf("\n=== Módulos de RAM ===\n");
            for (int i = 0; i < status.detailed_ram.num_sticks && i < 4; i++) {
                printf("Slot %d: %s %s %lu MB %d MHz %s\n",
                       status.detailed_ram.sticks[i].slot_id,
                       status.detailed_ram.sticks[i].manufacturer,
                       status.detailed_ram.sticks[i].type,
                       status.detailed_ram.sticks[i].size_mb,
                       status.detailed_ram.sticks[i].frequency_mhz,
                       status.detailed_ram.sticks[i].form_factor);
            }
        }
        
        // Mostrar información por núcleo
        printf("\n=== Información por Núcleo ===\n");
        for (int i = 0; i < status.num_cores && i < 4; i++) { // Mostrar solo los primeros 4
            printf("Core %d: %.1f%% uso, %.1f°C, %.0f MHz, %.3fV (%.0f-%.0f MHz)\n", 
                   status.cores[i].core_id,
                   status.cores[i].usage_percent,
                   status.cores[i].temperature,
                   status.cores[i].frequency,
                   status.cores[i].voltage,
                   status.cores[i].min_frequency,
                   status.cores[i].max_frequency);
        }
        
        // Mostrar sensores del sistema
        printf("\n=== Sensores del Sistema ===\n");
        for (int i = 0; i < status.num_sensors && i < 10; i++) { // Mostrar solo los primeros 10
            printf("%s (%s): %.2f %s %s\n", 
                   status.sensors[i].name,
                   status.sensors[i].type,
                   status.sensors[i].value,
                   status.sensors[i].unit,
                   status.sensors[i].critical ? "[CRÍTICO]" : "");
        }
        
        // Mostrar información de la motherboard
        printf("\n=== Motherboard ===\n");
        printf("Fabricante: %s\n", status.motherboard.manufacturer);
        printf("Modelo: %s\n", status.motherboard.model);
        printf("Versión: %s\n", status.motherboard.version);
        printf("BIOS: %s\n", status.motherboard.bios_version);
    }
    
    // Ejemplo de monitoreo continuo
    printf("\n=== Monitoreo Continuo (5 segundos) ===\n");
    for (int i = 0; i < 5; i++) {
        printf("CPU: %.2f%% | RAM: %.2f%% | Temp: %.1f°C\n", 
               sysmon_get_cpu_usage(), 
               sysmon_get_ram_usage_percent(),
               sysmon_get_cpu_temperature());
        sleep(1);
    }
    
    // Limpiar recursos
    sysmon_cleanup();
    
    return 0;
}
