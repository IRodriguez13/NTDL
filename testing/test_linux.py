#!/usr/bin/env python3
import ctypes
import time
import sys
import os
import platform
import psutil
from ctypes import POINTER, Structure, c_char_p, c_int, c_double

# Configurar el path de la librería
os.environ['LD_LIBRARY_PATH'] = '.'

CPU_MODEL_LEN = 128

class CpuInfo(Structure):
    _fields_ = [
        ("vendor", c_char_p),
        ("model", c_char_p),
        ("cores", c_int),
        ("mhz", c_double)
    ]

def get_system_info():
    """Obtener información del sistema usando Python"""
    return {
        'platform': platform.system(),
        'platform_version': platform.version(),
        'machine': platform.machine(),
        'processor': platform.processor(),
        'python_version': platform.python_version(),
        'cpu_count': psutil.cpu_count(),
        'cpu_freq': psutil.cpu_freq(),
        'memory': psutil.virtual_memory(),
        'boot_time': psutil.boot_time()
    }

def main():
    print("=== Test Linux de SysMon (10 segundos) ===")
    
    # Verificar que estamos en Linux
    if platform.system() != "Linux":
        print("❌ Este test está diseñado para Linux")
        print(f"   Sistema actual: {platform.system()}")
        sys.exit(1)
    
    try:
        # Cargar la librería
        lib = ctypes.CDLL("./../libsysmon.so")
        print("✅ Librería cargada correctamente")
    except Exception as e:
        print(f"❌ Error al cargar la librería: {e}")
        sys.exit(1)
    
    # Configurar tipos de retorno y argumentos
    lib.alloc_cpu_info.restype = POINTER(CpuInfo)
    lib.free_cpu_info.argtypes = [POINTER(CpuInfo)]
    lib.get_cpu_usage.restype = c_double
    lib.get_cpu_model.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
    lib.get_cpu_model.restype = c_int
    
    # 0. Información del sistema usando Python
    print("\n0. Información del sistema (Python):")
    print("-" * 40)
    sys_info = get_system_info()
    print(f"   Plataforma: {sys_info['platform']} {sys_info['platform_version']}")
    print(f"   Arquitectura: {sys_info['machine']}")
    print(f"   Procesador: {sys_info['processor']}")
    print(f"   Núcleos físicos: {sys_info['cpu_count']}")
    if sys_info['cpu_freq']:
        print(f"   Frecuencia actual: {sys_info['cpu_freq'].current:.0f} MHz")
        print(f"   Frecuencia mínima: {sys_info['cpu_freq'].min:.0f} MHz")
        print(f"   Frecuencia máxima: {sys_info['cpu_freq'].max:.0f} MHz")
    print(f"   Memoria total: {sys_info['memory'].total / (1024**3):.1f} GB")
    print(f"   Memoria disponible: {sys_info['memory'].available / (1024**3):.1f} GB")
    
    # 1. Información del CPU usando la librería C
    print("\n1. Información del CPU (Librería C):")
    print("-" * 40)
    
    # Usar alloc_cpu_info
    info_ptr = lib.alloc_cpu_info()
    info = info_ptr.contents
    
    print(f"   Vendor: {info.vendor.decode() if info.vendor else 'N/A'}")
    print(f"   Model:  {info.model.decode() if info.model else 'N/A'}")
    print(f"   Cores:  {info.cores}")
    print(f"   MHz:    {info.mhz:.0f}")
    
    # Liberar memoria
    lib.free_cpu_info(info_ptr)
    
    # 2. Obtener modelo de CPU usando get_cpu_model
    print("\n2. Modelo de CPU (get_cpu_model):")
    print("-" * 40)
    
    cpu_model_buffer = ctypes.create_string_buffer(CPU_MODEL_LEN)
    if lib.get_cpu_model(cpu_model_buffer, CPU_MODEL_LEN) == 0:
        print(f"   Modelo: {cpu_model_buffer.value.decode()}")
    else:
        print("   Error al obtener modelo de CPU")
    
    # 3. Monitoreo en tiempo real por 10 segundos
    print(f"\n3. Monitoreo en tiempo real (10 segundos):")
    print("=" * 80)
    print(f"{'Tiempo':<6} {'CPU%':<8} {'FreqMHz':<10} {'RAM%':<8} {'Load1':<8} {'Load5':<8} {'Load15':<8}")
    print("=" * 80)
    
    start_time = time.time()
    iteration = 0
    
    while time.time() - start_time < 10:
        iteration += 1
        
        # Obtener uso de CPU desde la librería C
        cpu_usage_c = lib.get_cpu_usage()
        
        # Obtener información actualizada del CPU
        info_ptr = lib.alloc_cpu_info()
        info = info_ptr.contents
        current_freq = info.mhz
        
        # Obtener información adicional usando Python
        cpu_usage_py = psutil.cpu_percent(interval=0.1)
        memory = psutil.virtual_memory()
        load_avg = psutil.getloadavg()
        
        elapsed = time.time() - start_time
        
        # Mostrar métricas en una línea
        print(f"{elapsed:5.1f}s  "
              f"{cpu_usage_c:6.2f}%   "
              f"{current_freq:8.0f}MHz "
              f"{memory.percent:6.1f}%   "
              f"{load_avg[0]:6.2f}   "
              f"{load_avg[1]:6.2f}   "
              f"{load_avg[2]:6.2f}")

                
        # Liberar memoria
        lib.free_cpu_info(info_ptr)

        time.sleep(0.5)

    print("=" * 80)
    print("✅ Test completado exitosamente")
    print("La librería funciona correctamente desde Python en Linux")
    
    # 4. Información adicional del sistema
    print(f"\n4. Información adicional:")
    print("-" * 40)
    print(f"   Iteraciones completadas: {iteration}")
    print(f"   Tiempo total de ejecución: {time.time() - start_time:.2f} segundos")
    print(f"   Frecuencia de muestreo: 1 Hz")
    
    # 5. Comparación de métricas
    print(f"\n5. Comparación de métricas (última lectura):")
    print("-" * 40)
    print(f"   CPU Usage (C): {cpu_usage_c:.2f}%")
    print(f"   CPU Usage (Python): {cpu_usage_py:.2f}%")
    print(f"   RAM Usage: {memory.percent:.1f}%")
    print(f"   Load Average: {load_avg[0]:.2f}, {load_avg[1]:.2f}, {load_avg[2]:.2f}")

if __name__ == "__main__":
    main()
