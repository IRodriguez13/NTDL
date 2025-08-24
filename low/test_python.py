#!/usr/bin/env python3
import ctypes
import time
import sys
import os

# Configurar el path de la librería
os.environ['LD_LIBRARY_PATH'] = '.'

try:
    # Cargar la librería
    lib = ctypes.CDLL('./libsysmon.so')
    print("✅ Librería cargada correctamente")
except Exception as e:
    print(f"❌ Error al cargar la librería: {e}")
    sys.exit(1)

# Definir la estructura SystemStatus (versión simplificada)
class SystemStatus(ctypes.Structure):
    _fields_ = [
        ("os_name", ctypes.c_char * 32),
        ("kernel_version", ctypes.c_char * 64),
        ("hostname", ctypes.c_char * 64),
        ("cpu_model", ctypes.c_char * 128),
        ("cpu_cores", ctypes.c_int),
        ("cpu_threads", ctypes.c_int),
        ("cpu_usage_percent", ctypes.c_float),
        ("cpu_temperature", ctypes.c_float),
        ("cpu_frequency", ctypes.c_float),
        ("ram_total_kb", ctypes.c_ulong),
        ("ram_free_kb", ctypes.c_ulong),
        ("ram_used_kb", ctypes.c_ulong),
        ("ram_usage_percent", ctypes.c_float),
        ("load_average_1min", ctypes.c_float),
        ("load_average_5min", ctypes.c_float),
        ("load_average_15min", ctypes.c_float),
        ("num_cores", ctypes.c_int),
        ("timestamp", ctypes.c_ulong),
        # Motherboard info
        ("motherboard_manufacturer", ctypes.c_char * 64),
        ("motherboard_model", ctypes.c_char * 128),
        ("motherboard_bios_version", ctypes.c_char * 64),
    ]

# Configurar tipos de retorno
lib.sysmon_init.restype = ctypes.c_int
lib.sysmon_get_metrics.argtypes = [ctypes.POINTER(SystemStatus)]
lib.sysmon_get_metrics.restype = ctypes.c_int

def format_bytes(bytes_value):
    """Convertir bytes a formato legible"""
    for unit in ['KB', 'MB', 'GB']:
        if bytes_value < 1024.0:
            return f"{bytes_value:.1f} {unit}"
        bytes_value /= 1024.0
    return f"{bytes_value:.1f} TB"

def main():
    print("=== Test Python de SysMon (10 segundos) ===")
    
    # Inicializar la librería
    print("1. Inicializando librería...")
    if lib.sysmon_init() != 0:
        print("❌ Error al inicializar la librería")
        return
    
    print("✅ Librería inicializada correctamente")
    
    # Obtener información inicial
    status = SystemStatus()
    if lib.sysmon_get_metrics(ctypes.byref(status)) != 0:
        print("❌ Error al obtener métricas")
        return
    
    print(f"\n2. Información del sistema:")
    print(f"   OS: {status.os_name.decode()}")
    print(f"   Kernel: {status.kernel_version.decode()}")
    print(f"   Hostname: {status.hostname.decode()}")
    print(f"   CPU: {status.cpu_model.decode()}")
    print(f"   Núcleos: {status.cpu_cores}")
    print(f"   Motherboard: {status.motherboard_manufacturer.decode()} {status.motherboard_model.decode()}")
    print(f"   BIOS: {status.motherboard_bios_version.decode()}")
    
    # Monitoreo en tiempo real por 10 segundos
    print(f"\n3. Monitoreo en tiempo real (10 segundos):")
    print("=" * 60)
    print(f"{'Tiempo':<6} {'CPU%':<6} {'RAM%':<6} {'Temp°C':<7} {'FreqMHz':<8} {'Load1':<6} {'Load5':<6} {'Load15':<6}")
    print("=" * 60)
    
    start_time = time.time()
    iteration = 0
    
    while time.time() - start_time < 10:
        iteration += 1
        
        # Obtener métricas actualizadas
        if lib.sysmon_get_metrics(ctypes.byref(status)) == 0:
            elapsed = time.time() - start_time
            
            # Mostrar métricas en una línea
            print(f"{elapsed:5.1f}s  "
                  f"{status.cpu_usage_percent:5.1f}% "
                  f"{status.ram_usage_percent:5.1f}% "
                  f"{status.cpu_temperature:6.1f}°C "
                  f"{status.cpu_frequency:7.0f}MHz "
                  f"{status.load_average_1min:5.2f} "
                  f"{status.load_average_5min:5.2f} "
                  f"{status.load_average_15min:5.2f}")
        
        time.sleep(1)
    
    print("=" * 60)
    print("✅ Test completado exitosamente")
    print("La librería funciona correctamente desde Python")

if __name__ == "__main__":
    main()
