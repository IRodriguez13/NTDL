#!/usr/bin/env python3
"""
Ejemplo simple de uso del wrapper de Python para sysmon
"""

import ctypes
import os
import sys

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

# Cargar la biblioteca
try:
    sysmon = ctypes.CDLL(lib_path)
except Exception as e:
    print(f"Error al cargar la biblioteca: {e}")
    sys.exit(1)

# Definir tipos de retorno y argumentos para las funciones básicas
sysmon.sysmon_init.restype = ctypes.c_int
sysmon.sysmon_get_cpu_usage.restype = ctypes.c_float
sysmon.sysmon_get_cpu_cores.restype = ctypes.c_int
sysmon.sysmon_get_cpu_model.restype = ctypes.c_char_p
sysmon.sysmon_get_ram_total.restype = ctypes.c_ulong
sysmon.sysmon_get_ram_usage_percent.restype = ctypes.c_float
sysmon.sysmon_get_os_name.restype = ctypes.c_char_p

def main():
    print("=== Ejemplo Simple de SysMon ===\n")
    
    # Inicializar
    if sysmon.sysmon_init() != 0:
        print("Error al inicializar sysmon")
        return
    
    try:
        # Obtener métricas básicas
        print("Sistema Operativo:", sysmon.sysmon_get_os_name().decode('utf-8'))
        print("CPU Modelo:", sysmon.sysmon_get_cpu_model().decode('utf-8'))
        print("Núcleos CPU:", sysmon.sysmon_get_cpu_cores())
        print("Uso de CPU: {:.2f}%".format(sysmon.sysmon_get_cpu_usage()))
        
        ram_total = sysmon.sysmon_get_ram_total()
        ram_usage = sysmon.sysmon_get_ram_usage_percent()
        
        print("RAM Total: {:,} KB ({:.2f} GB)".format(ram_total, ram_total / (1024*1024)))
        print("Uso de RAM: {:.2f}%".format(ram_usage))
        
        # Ejemplo de monitoreo continuo
        print("\n=== Monitoreo Continuo (3 segundos) ===")
        import time
        for i in range(3):
            cpu_usage = sysmon.sysmon_get_cpu_usage()
            ram_usage = sysmon.sysmon_get_ram_usage_percent()
            print("Tiempo {}: CPU {:.1f}% | RAM {:.1f}%".format(i+1, cpu_usage, ram_usage))
            time.sleep(1)
            
    finally:
        # Limpiar recursos
        sysmon.sysmon_cleanup()
    
    print("\n¡Listo!")

if __name__ == "__main__":
    main()
