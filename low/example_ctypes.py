#!/usr/bin/env python3
"""
Ejemplo de uso de la biblioteca sysmon desde Python usando ctypes
"""

import ctypes
import ctypes.util
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

# Definir la estructura SystemStatus
class SystemStatus(ctypes.Structure):
    _fields_ = [
        ("os_name", ctypes.c_char * 32),
        ("cpu_model", ctypes.c_char * 128),
        ("cpu_cores", ctypes.c_int),
        ("cpu_usage_percent", ctypes.c_float),
        ("ram_total_kb", ctypes.c_ulong),
        ("ram_free_kb", ctypes.c_ulong),
        ("disk_model", ctypes.c_char * 128),
        ("disk_health", ctypes.c_int)
    ]

# Definir tipos de retorno y argumentos para las funciones
sysmon.sysmon_init.restype = ctypes.c_int
sysmon.sysmon_init.argtypes = []

sysmon.sysmon_get_metrics.restype = ctypes.c_int
sysmon.sysmon_get_metrics.argtypes = [ctypes.POINTER(SystemStatus)]

sysmon.sysmon_get_cpu_usage.restype = ctypes.c_float
sysmon.sysmon_get_cpu_usage.argtypes = []

sysmon.sysmon_get_cpu_cores.restype = ctypes.c_int
sysmon.sysmon_get_cpu_cores.argtypes = []

sysmon.sysmon_get_cpu_model.restype = ctypes.c_char_p
sysmon.sysmon_get_cpu_model.argtypes = []

sysmon.sysmon_get_ram_total.restype = ctypes.c_ulong
sysmon.sysmon_get_ram_total.argtypes = []

sysmon.sysmon_get_ram_free.restype = ctypes.c_ulong
sysmon.sysmon_get_ram_free.argtypes = []

sysmon.sysmon_get_ram_used.restype = ctypes.c_ulong
sysmon.sysmon_get_ram_used.argtypes = []

sysmon.sysmon_get_ram_usage_percent.restype = ctypes.c_float
sysmon.sysmon_get_ram_usage_percent.argtypes = []

sysmon.sysmon_get_os_name.restype = ctypes.c_char_p
sysmon.sysmon_get_os_name.argtypes = []

sysmon.sysmon_get_disk_model.restype = ctypes.c_char_p
sysmon.sysmon_get_disk_model.argtypes = []

sysmon.sysmon_get_disk_health.restype = ctypes.c_int
sysmon.sysmon_get_disk_health.argtypes = []



def main():
    print("=== SysMon desde Python (ctypes) ===\n")
    
    # Inicializar
    if sysmon.sysmon_init() != 0:
        print("Error al inicializar sysmon")
        return
    
    try:
        # Obtener métricas específicas
        print(f"Sistema Operativo: {sysmon.sysmon_get_os_name().decode('utf-8')}")
        print(f"CPU Modelo: {sysmon.sysmon_get_cpu_model().decode('utf-8')}")
        print(f"Núcleos CPU: {sysmon.sysmon_get_cpu_cores()}")
        print(f"Uso de CPU: {sysmon.sysmon_get_cpu_usage():.2f}%")
        
        print(f"\nMemoria RAM:")
        ram_total = sysmon.sysmon_get_ram_total()
        ram_free = sysmon.sysmon_get_ram_free()
        ram_used = sysmon.sysmon_get_ram_used()
        ram_percent = sysmon.sysmon_get_ram_usage_percent()
        
        print(f"  Total: {ram_total:,} KB ({ram_total / (1024*1024):.2f} GB)")
        print(f"  Libre: {ram_free:,} KB ({ram_free / (1024*1024):.2f} GB)")
        print(f"  Usada: {ram_used:,} KB ({ram_used / (1024*1024):.2f} GB)")
        print(f"  Uso: {ram_percent:.2f}%")
        
        print(f"\nDisco:")
        print(f"  Modelo: {sysmon.sysmon_get_disk_model().decode('utf-8')}")
        print(f"  Salud: {sysmon.sysmon_get_disk_health()}%")
        
        # Obtener todas las métricas como estructura
        status = SystemStatus()
        if sysmon.sysmon_get_metrics(ctypes.byref(status)) == 0:
            print(f"\n=== Métricas Completas ===")
            print(f"OS: {status.os_name.decode('utf-8')}")
            print(f"CPU: {status.cpu_model.decode('utf-8')} ({status.cpu_cores} cores, {status.cpu_usage_percent:.2f}%)")
            print(f"RAM: {status.ram_total_kb - status.ram_free_kb:,}/{status.ram_total_kb:,} KB ({(status.ram_total_kb - status.ram_free_kb) / status.ram_total_kb * 100:.2f}%)")
            print(f"Disk: {status.disk_model.decode('utf-8')} (health: {status.disk_health}%)")
        

        
        # Ejemplo de monitoreo continuo
        print(f"\n=== Monitoreo Continuo (5 segundos) ===")
        import time
        for i in range(5):
            cpu_usage = sysmon.sysmon_get_cpu_usage()
            ram_usage = sysmon.sysmon_get_ram_usage_percent()
            print(f"CPU: {cpu_usage:.2f}% | RAM: {ram_usage:.2f}%")
            time.sleep(1)
            
    finally:
        # Limpiar recursos
        sysmon.sysmon_cleanup()

if __name__ == "__main__":
    main()
