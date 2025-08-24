#include "sysmon.h"
#include <stdio.h>

int main() {
    printf("=== Test de Inicialización de SysMon ===\n");
    
    // Inicializar la librería
    printf("1. Inicializando librería...\n");
    if (sysmon_init() != 0) {
        printf("❌ Error al inicializar la librería\n");
        return -1;
    }
    printf("✅ Librería inicializada correctamente\n");
    
    // Obtener métricas del sistema
    printf("\n2. Obteniendo métricas del sistema...\n");
    SystemStatus status;
    if (sysmon_get_metrics(&status) != 0) {
        printf("❌ Error al obtener métricas\n");
        return -1;
    }
    printf("✅ Métricas obtenidas correctamente\n");
    
    // Mostrar información básica
    printf("\n3. Información del sistema:\n");
    printf("   OS: %s\n", status.os_name);
    printf("   Kernel: %s\n", status.kernel_version);
    printf("   Hostname: %s\n", status.hostname);
    printf("   CPU: %s\n", status.cpu_model);
    printf("   Núcleos: %d\n", status.cpu_cores);
    printf("   RAM: %.1f%% usado\n", status.ram_usage_percent);
    printf("   Load: %.2f, %.2f, %.2f\n", 
           status.load_average_1min,
           status.load_average_5min, 
           status.load_average_15min);
    
    // Mostrar información de motherboard
    printf("   Motherboard: %s %s\n", 
           status.motherboard.manufacturer,
           status.motherboard.model);
    printf("   BIOS: %s\n", status.motherboard.bios_version);
    
    // Mostrar información de núcleos
    printf("   Núcleos detectados: %d\n", status.num_cores);
    if (status.num_cores > 0) {
        printf("   Frecuencia promedio: %.0f MHz\n", status.cpu_frequency);
        printf("   Temperatura: %.1f°C\n", status.cpu_temperature);
    }
    
    printf("\n✅ Test completado exitosamente\n");
    printf("La librería está lista para usar\n");
    
    return 0;
}
