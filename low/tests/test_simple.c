#include "sysmon.h"
#include <stdio.h>

int main() {
    printf("=== Prueba Simple de SysMon ===\n");
    
    // Probar funciones básicas
    printf("CPU Model: %s\n", sysmon_get_cpu_model());
    printf("CPU Cores: %d\n", sysmon_get_cpu_cores());
    printf("CPU Usage: %.2f%%\n", sysmon_get_cpu_usage());
    
    printf("RAM Total: %lu KB\n", sysmon_get_ram_total());
    printf("RAM Used: %lu KB\n", sysmon_get_ram_used());
    printf("RAM Usage: %.2f%%\n", sysmon_get_ram_usage_percent());
    
    printf("OS: %s\n", sysmon_get_os_name());
    printf("Kernel: %s\n", sysmon_get_kernel_version());
    printf("Hostname: %s\n", sysmon_get_hostname());
    
    printf("Num Disks: %d\n", sysmon_get_num_disks());
    printf("Num Network Interfaces: %d\n", sysmon_get_num_network_interfaces());
    printf("Num Sensors: %d\n", sysmon_get_num_sensors());
    
    printf("=== Prueba Completada ===\n");
    return 0;
}
