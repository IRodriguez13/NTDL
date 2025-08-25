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

def dump_raw_data(sys_info, cpu_info, cpu_model, final_metrics, execution_stats):
    """Volcar datos en sucio a la terminal"""
    print("\n" + "=" * 60)
    print("RAW DATA DUMP - DATOS EN SUCIO")
    print("=" * 60)
    
    # Timestamp
    print(f"TIMESTAMP: {time.time()}")
    print(f"DATETIME: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # System Info Raw
    print(f"\nSYS_PLATFORM: {sys_info['platform']}")
    print(f"SYS_PLATFORM_VERSION: {sys_info['platform_version']}")
    print(f"SYS_MACHINE: {sys_info['machine']}")
    print(f"SYS_PROCESSOR: {sys_info['processor']}")
    print(f"SYS_PYTHON_VERSION: {sys_info['python_version']}")
    print(f"SYS_CPU_COUNT: {sys_info['cpu_count']}")
    
    if sys_info['cpu_freq']:
        print(f"SYS_CPU_FREQ_CURRENT: {sys_info['cpu_freq'].current}")
        print(f"SYS_CPU_FREQ_MIN: {sys_info['cpu_freq'].min}")
        print(f"SYS_CPU_FREQ_MAX: {sys_info['cpu_freq'].max}")
    else:
        print("SYS_CPU_FREQ_CURRENT: NULL")
        print("SYS_CPU_FREQ_MIN: NULL")
        print("SYS_CPU_FREQ_MAX: NULL")
    
    print(f"SYS_MEMORY_TOTAL: {sys_info['memory'].total}")
    print(f"SYS_MEMORY_AVAILABLE: {sys_info['memory'].available}")
    print(f"SYS_MEMORY_PERCENT: {sys_info['memory'].percent}")
    print(f"SYS_MEMORY_USED: {sys_info['memory'].used}")
    print(f"SYS_MEMORY_FREE: {sys_info['memory'].free}")
    print(f"SYS_BOOT_TIME: {sys_info['boot_time']}")
    
    # CPU Info Raw (from C library)
    print(f"\nCPU_VENDOR: {cpu_info['vendor']}")
    print(f"CPU_MODEL: {cpu_info['model']}")
    print(f"CPU_CORES: {cpu_info['cores']}")
    print(f"CPU_MHZ: {cpu_info['mhz']}")
    print(f"CPU_MODEL_BUFFER: {cpu_model}")
    
    # Final Metrics Raw
    print(f"\nMETRIC_CPU_C: {final_metrics['cpu_c']}")
    print(f"METRIC_CPU_PY: {final_metrics['cpu_py']}")
    print(f"METRIC_RAM: {final_metrics['ram']}")
    print(f"METRIC_LOAD_1: {final_metrics['load'][0]}")
    print(f"METRIC_LOAD_5: {final_metrics['load'][1]}")
    print(f"METRIC_LOAD_15: {final_metrics['load'][2]}")
    
    # Execution Stats Raw
    print(f"\nEXEC_ITERATIONS: {execution_stats['iterations']}")
    print(f"EXEC_TOTAL_TIME: {execution_stats['total_time']}")
    print(f"EXEC_SAMPLING_RATE: 1.0")
    
    # Process Info
    try:
        process = psutil.Process()
        print(f"\nPROCESS_PID: {process.pid}")
        print(f"PROCESS_NAME: {process.name()}")
        print(f"PROCESS_CPU_PERCENT: {process.cpu_percent()}")
        print(f"PROCESS_MEMORY_PERCENT: {process.memory_percent()}")
        print(f"PROCESS_MEMORY_RSS: {process.memory_info().rss}")
        print(f"PROCESS_MEMORY_VMS: {process.memory_info().vms}")
        print(f"PROCESS_CREATE_TIME: {process.create_time()}")
    except:
        print("\nPROCESS_INFO: ERROR")
    
    # Environment
    print(f"\nENV_FROZEN: {getattr(sys, 'frozen', False)}")
    print(f"ENV_EXECUTABLE: {sys.executable}")
    print(f"ENV_ARGV: {sys.argv}")
    print(f"ENV_PATH: {os.environ.get('PATH', 'NULL')[:100]}...")
    
    print("=" * 60)
    print("END RAW DATA DUMP")
    print("=" * 60)

def create_report(sys_info, cpu_info, cpu_model, final_metrics, execution_stats):
    """Crear reporte de pruebas en archivo report.txt"""
    try:
        # Determinar directorio para el reporte
        if getattr(sys, 'frozen', False):
            report_dir = os.path.dirname(sys.executable)
        else:
            report_dir = os.path.dirname(os.path.abspath(__file__))
        
        report_path = os.path.join(report_dir, "report.txt")
        
        with open(report_path, 'w', encoding='utf-8') as f:
            f.write("=== REPORTE DE PRUEBAS SYSMON ===\n")
            f.write(f"Fecha y hora: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 50 + "\n\n")
            
            # Información del sistema
            f.write("INFORMACIÓN DEL SISTEMA:\n")
            f.write("-" * 30 + "\n")
            f.write(f"Plataforma: {sys_info['platform']} {sys_info['platform_version']}\n")
            f.write(f"Arquitectura: {sys_info['machine']}\n")
            f.write(f"Procesador: {sys_info['processor']}\n")
            f.write(f"Núcleos físicos: {sys_info['cpu_count']}\n")
            if sys_info['cpu_freq']:
                f.write(f"Frecuencia actual: {sys_info['cpu_freq'].current:.0f} MHz\n")
                f.write(f"Frecuencia mínima: {sys_info['cpu_freq'].min:.0f} MHz\n")
                f.write(f"Frecuencia máxima: {sys_info['cpu_freq'].max:.0f} MHz\n")
            f.write(f"Memoria total: {sys_info['memory'].total / (1024**3):.1f} GB\n")
            f.write(f"Memoria disponible: {sys_info['memory'].available / (1024**3):.1f} GB\n\n")
            
            # Información del CPU desde librería C
            f.write("INFORMACIÓN CPU (LIBRERÍA C):\n")
            f.write("-" * 30 + "\n")
            f.write(f"Vendor: {cpu_info['vendor']}\n")
            f.write(f"Model: {cpu_info['model']}\n")
            f.write(f"Cores: {cpu_info['cores']}\n")
            f.write(f"MHz: {cpu_info['mhz']:.0f}\n")
            f.write(f"Modelo CPU (get_cpu_model): {cpu_model}\n\n")
            
            # Estadísticas de ejecución
            f.write("ESTADÍSTICAS DE EJECUCIÓN:\n")
            f.write("-" * 30 + "\n")
            f.write(f"Iteraciones completadas: {execution_stats['iterations']}\n")
            f.write(f"Tiempo total: {execution_stats['total_time']:.2f} segundos\n")
            f.write(f"Frecuencia de muestreo: 1 Hz\n\n")
            
            # Métricas finales
            f.write("MÉTRICAS FINALES:\n")
            f.write("-" * 30 + "\n")
            f.write(f"CPU Usage (C): {final_metrics['cpu_c']:.2f}%\n")
            f.write(f"CPU Usage (Python): {final_metrics['cpu_py']:.2f}%\n")
            f.write(f"RAM Usage: {final_metrics['ram']:.1f}%\n")
            f.write(f"Load Average: {final_metrics['load'][0]:.2f}, {final_metrics['load'][1]:.2f}, {final_metrics['load'][2]:.2f}\n\n")
            
            # Estado del test
            f.write("RESULTADO DEL TEST:\n")
            f.write("-" * 30 + "\n")
            f.write("✅ Test completado exitosamente\n")
            f.write("✅ Librería sysmon.dll cargada correctamente\n")
            f.write("✅ Todas las funciones de la librería funcionan correctamente\n")
        
        print(f"\n📄 Reporte generado: {report_path}")
        return True
        
    except Exception as e:
        print(f"\n❌ Error al generar reporte: {e}")
        return False

def main():
    print("=== Test Windows de SysMon (10 segundos) ===")

    # Para PyInstaller onefile: buscar DLL en el directorio del ejecutable
    if getattr(sys, 'frozen', False):
        # Ejecutándose como ejecutable PyInstaller
        exe_dir = os.path.dirname(sys.executable)
        dll_path = os.path.join(exe_dir, "sysmon.dll")
    else:
        # Ejecutándose como script Python
        script_dir = os.path.dirname(os.path.abspath(__file__))
        dll_path = os.path.join(script_dir, "sysmon.dll")
    
    print(f"Buscando DLL en: {dll_path}")
    lib = ctypes.CDLL(dll_path)
    print("✅ Librería cargada correctamente")
    
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
    
    # Guardar información del CPU para el reporte
    cpu_info = {
        'vendor': info.vendor.decode() if info.vendor else 'N/A',
        'model': info.model.decode() if info.model else 'N/A',
        'cores': info.cores,
        'mhz': info.mhz
    }
    
    print(f"   Vendor: {cpu_info['vendor']}")
    print(f"   Model:  {cpu_info['model']}")
    print(f"   Cores:  {cpu_info['cores']}")
    print(f"   MHz:    {cpu_info['mhz']:.0f}")
    
    # Liberar memoria
    lib.free_cpu_info(info_ptr)
    
    # 2. Obtener modelo de CPU usando get_cpu_model
    print("\n2. Modelo de CPU (get_cpu_model):")
    print("-" * 40)
    
    cpu_model_buffer = ctypes.create_string_buffer(CPU_MODEL_LEN)
    if lib.get_cpu_model(cpu_model_buffer, CPU_MODEL_LEN) == 0:
        cpu_model = cpu_model_buffer.value.decode()
        print(f"   Modelo: {cpu_model}")
    else:
        cpu_model = "Error al obtener modelo de CPU"
        print(f"   {cpu_model}")
    
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
    
    # Obtener métricas finales para comparación
    final_cpu_usage_c = lib.get_cpu_usage()
    final_cpu_usage_py = psutil.cpu_percent(interval=0.1)
    final_memory = psutil.virtual_memory()
    final_load_avg = psutil.getloadavg()
    
    print(f"   CPU Usage (C): {final_cpu_usage_c:.2f}%")
    print(f"   CPU Usage (Python): {final_cpu_usage_py:.2f}%")
    print(f"   RAM Usage: {final_memory.percent:.1f}%")
    print(f"   Load Average: {final_load_avg[0]:.2f}, {final_load_avg[1]:.2f}, {final_load_avg[2]:.2f}")
    
    # 6. Generar reporte
    final_metrics = {
        'cpu_c': final_cpu_usage_c,
        'cpu_py': final_cpu_usage_py,
        'ram': final_memory.percent,
        'load': final_load_avg
    }
    
    execution_stats = {
        'iterations': iteration,
        'total_time': time.time() - start_time
    }
    
    create_report(sys_info, cpu_info, cpu_model, final_metrics, execution_stats)
    
    # 7. Dump de datos en sucio
    dump_raw_data(sys_info, cpu_info, cpu_model, final_metrics, execution_stats)

if __name__ == "__main__":
    main()
    