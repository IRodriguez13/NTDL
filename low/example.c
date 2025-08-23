#include "sysmon.h"
#include <stdio.h>
#include <unistd.h>

int main() 
{
    // Inicializar el sistema de monitoreo
    if (sysmon_init() != 0) {
        printf("Error al inicializar sysmon\n");
        return 1;
    }
    
    printf("=== Monitoreo del Sistema ===\n\n");
    
    // Obtener métricas específicas
    printf("Sistema Operativo: %s\n", sysmon_get_os_name());
    printf("CPU Modelo: %s\n", sysmon_get_cpu_model());
    printf("Núcleos CPU: %d\n", sysmon_get_cpu_cores());
    printf("Uso de CPU: %.2f%%\n", sysmon_get_cpu_usage());
    
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
    
    printf("\nDisco:\n");
    printf("  Modelo: %s\n", sysmon_get_disk_model());
    printf("  Salud: %d%%\n", sysmon_get_disk_health());
    
    // Obtener todas las métricas como estructura
    SystemStatus status;
    if (sysmon_get_metrics(&status) == 0) {
        printf("\n=== Métricas Completas ===\n");
        printf("OS: %s\n", status.os_name);
        printf("CPU: %s (%d cores, %.2f%%)\n", 
               status.cpu_model, status.cpu_cores, status.cpu_usage_percent);
        printf("RAM: %lu/%lu KB (%.2f%%)\n", 
               status.ram_total_kb - status.ram_free_kb,
               status.ram_total_kb,
               ((float)(status.ram_total_kb - status.ram_free_kb) / status.ram_total_kb) * 100.0f);
        printf("Disk: %s (health: %d%%)\n", status.disk_model, status.disk_health);
    }
    

    
    // Ejemplo de monitoreo continuo
    printf("\n=== Monitoreo Continuo (5 segundos) ===\n");
    
    for (int i = 0; i < 5; i++) 
    {
        printf("CPU: %.2f%% | RAM: %.2f%%\n", 
              
              sysmon_get_cpu_usage(), 
              sysmon_get_ram_usage_percent());

        sleep(1);
    }
    
    // Limpiar recursos
    sysmon_cleanup();
    
    return 0;
}
