#include "sysmon.h"
#include <stdio.h>

int main() {
    printf("=== Test Minimal ===\n");
    
    // Solo probar una función muy básica
    printf("Inicializando...\n");
    int result = sysmon_init();
    printf("Init result: %d\n", result);
    
    if (result == 0) {
        printf("OS Name: %s\n", sysmon_get_os_name());
    }
    
    printf("=== Test Completado ===\n");
    return 0;
}
