#!/usr/bin/env python3
"""
Ejemplo de uso de la biblioteca sysmon desde Python usando cffi
"""

import os
import sys
from cffi import FFI

# Detectar la plataforma y cargar la biblioteca correspondiente
if sys.platform.startswith('linux'):
    lib_path = "./libsysmon.so"
elif sys.platform.startswith('darwin'):
    lib_path = "./libsysmon.dylib"
elif sys.platform.startswith('win'):
    lib_path = "./sysmon.dll"
else:
    print("Plataforma no soportada")
    sys.exit(1)

# Verificar que la biblioteca existe
if not os.path.exists(lib_path):
    print(f"Error: No se encontró la biblioteca {lib_path}")
    print("Ejecuta 'make' primero para compilar la biblioteca")
    sys.exit(1)

# Crear instancia de FFI
ffi = FFI()

# Definir las estructuras y funciones en C
ffi.cdef("""
    typedef struct {
        char os_name[32];
        char cpu_model[128];
        int cpu_cores;
        float cpu_usage_percent;
        unsigned long ram_total_kb;
        unsigned long ram_free_kb;
        char disk_model[128];
        int disk_health;
    } SystemStatus;

    // Funciones de inicialización
    int sysmon_init(void);
    void sysmon_cleanup(void);

    // Funciones de métricas generales
    int sysmon_get_metrics(SystemStatus *status);

    // Funciones de métricas específicas
    float sysmon_get_cpu_usage(void);
    int sysmon_get_cpu_cores(void);
    const char* sysmon_get_cpu_model(void);
    unsigned long sysmon_get_ram_total(void);
    unsigned long sysmon_get_ram_free(void);
    unsigned long sysmon_get_ram_used(void);
    float sysmon_get_ram_usage_percent(void);
    const char* sysmon_get_os_name(void);
    const char* sysmon_get_disk_model(void);
    int sysmon_get_disk_health(void);

    // Función para liberar memoria JSON
    void free_json(char *json);
""")

# Cargar la biblioteca
try:
    C = ffi.dlopen(lib_path)
except Exception as e:
    print(f"Error al cargar la biblioteca: {e}")
    sys.exit(1)

def main():
    print("=== SysMon desde Python (cffi) ===\n")
    
    # Inicializar
    if C.sysmon_init() != 0:
        print("Error al inicializar sysmon")
        return
    
    try:
        # Obtener métricas específicas
        print(f"Sistema Operativo: {ffi.string(C.sysmon_get_os_name()).decode('utf-8')}")
        print(f"CPU Modelo: {ffi.string(C.sysmon_get_cpu_model()).decode('utf-8')}")
        print(f"Núcleos CPU: {C.sysmon_get_cpu_cores()}")
        print(f"Uso de CPU: {C.sysmon_get_cpu_usage():.2f}%")
        
        print(f"\nMemoria RAM:")
        ram_total = C.sysmon_get_ram_total()
        ram_free = C.sysmon_get_ram_free()
        ram_used = C.sysmon_get_ram_used()
        ram_percent = C.sysmon_get_ram_usage_percent()
        
        print(f"  Total: {ram_total:,} KB ({ram_total / (1024*1024):.2f} GB)")
        print(f"  Libre: {ram_free:,} KB ({ram_free / (1024*1024):.2f} GB)")
        print(f"  Usada: {ram_used:,} KB ({ram_used / (1024*1024):.2f} GB)")
        print(f"  Uso: {ram_percent:.2f}%")
        
        print(f"\nDisco:")
        print(f"  Modelo: {ffi.string(C.sysmon_get_disk_model()).decode('utf-8')}")
        print(f"  Salud: {C.sysmon_get_disk_health()}%")
        
        # Obtener todas las métricas como estructura
        status = ffi.new("SystemStatus *")
        if C.sysmon_get_metrics(status) == 0:
            print(f"\n=== Métricas Completas ===")
            print(f"OS: {ffi.string(status.os_name).decode('utf-8')}")
            print(f"CPU: {ffi.string(status.cpu_model).decode('utf-8')} ({status.cpu_cores} cores, {status.cpu_usage_percent:.2f}%)")
            print(f"RAM: {status.ram_total_kb - status.ram_free_kb:,}/{status.ram_total_kb:,} KB ({(status.ram_total_kb - status.ram_free_kb) / status.ram_total_kb * 100:.2f}%)")
            print(f"Disk: {ffi.string(status.disk_model).decode('utf-8')} (health: {status.disk_health}%)")
        

        
        # Ejemplo de monitoreo continuo
        print(f"\n=== Monitoreo Continuo (5 segundos) ===")
        import time
        for i in range(5):
            cpu_usage = C.sysmon_get_cpu_usage()
            ram_usage = C.sysmon_get_ram_usage_percent()
            print(f"CPU: {cpu_usage:.2f}% | RAM: {ram_usage:.2f}%")
            time.sleep(1)
            
    finally:
        # Limpiar recursos
        C.sysmon_cleanup()

if __name__ == "__main__":
    main()
