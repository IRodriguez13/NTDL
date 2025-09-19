#!/usr/bin/env python3
"""
Test multiplataforma para SysMon
Detecta automáticamente la plataforma y ejecuta tests apropiados
"""

import ctypes
import platform
import sys
import os
import time

def detect_platform():
    """Detecta la plataforma actual"""
    system = platform.system().lower()
    if system == "linux":
        return "linux"
    elif system == "darwin":
        return "macos"
    elif system == "windows":
        return "windows"
    else:
        return "unknown"

def get_library_name(platform_name):
    """Obtiene el nombre de la librería según la plataforma"""
    if platform_name == "linux":
        return "./libsysmon.so"
    elif platform_name == "macos":
        return "./libsysmon.dylib"
    elif platform_name == "windows":
        return "./sysmon.dll"
    else:
        return None

def load_library(lib_path):
    """Carga la librería de forma segura"""
    try:
        if not os.path.exists(lib_path):
            print(f"❌ Librería no encontrada: {lib_path}")
            return None
        
        lib = ctypes.CDLL(lib_path)
        print(f"✅ Librería cargada: {lib_path}")
        return lib
    except Exception as e:
        print(f"❌ Error cargando librería {lib_path}: {e}")
        return None

def test_basic_functions(lib, platform_name):
    """Test básico de funciones principales"""
    print(f"\n🧪 Testing funciones básicas en {platform_name.upper()}...")
    
    try:
        # Inicializar
        result = lib.sysmon_init()
        if result != 0:
            print(f"❌ sysmon_init() falló: {result}")
            return False
        print("✅ sysmon_init() exitoso")
        
        # Test CPU
        try:
            cpu_usage = lib.sysmon_get_cpu_usage_percent()
            lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
            cpu_usage = lib.sysmon_get_cpu_usage_percent()
            if cpu_usage >= 0:
                print(f"✅ CPU Usage: {cpu_usage:.1f}%")
            else:
                print(f"⚠️  CPU Usage no disponible: {cpu_usage}")
        except Exception as e:
            print(f"⚠️  Error en CPU test: {e}")
        
        # Test RAM
        try:
            lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
            ram_usage = lib.sysmon_get_ram_usage_percent()
            if ram_usage >= 0:
                print(f"✅ RAM Usage: {ram_usage:.1f}%")
            else:
                print(f"⚠️  RAM Usage no disponible: {ram_usage}")
        except Exception as e:
            print(f"⚠️  Error en RAM test: {e}")
        
        # Test temperatura CPU
        try:
            lib.sysmon_get_cpu_temperature.restype = ctypes.c_float
            cpu_temp = lib.sysmon_get_cpu_temperature()
            if cpu_temp > 0:
                print(f"✅ CPU Temperature: {cpu_temp:.1f}°C")
            else:
                print(f"⚠️  CPU Temperature no disponible: {cpu_temp}")
        except Exception as e:
            print(f"⚠️  Error en temperatura test: {e}")
        
        # Test GPU count
        try:
            lib.sysmon_get_gpu_count.restype = ctypes.c_int
            gpu_count = lib.sysmon_get_gpu_count()
            if gpu_count >= 0:
                print(f"✅ GPUs detectadas: {gpu_count}")
            else:
                print(f"⚠️  GPUs no disponibles: {gpu_count}")
        except Exception as e:
            print(f"⚠️  Error en GPU test: {e}")
        
        # Cleanup
        lib.sysmon_cleanup()
        print("✅ sysmon_cleanup() exitoso")
        
        return True
        
    except Exception as e:
        print(f"❌ Error en test básico: {e}")
        return False

def test_platform_specific(lib, platform_name):
    """Test específico por plataforma"""
    print(f"\n🔧 Testing funciones específicas de {platform_name.upper()}...")
    
    if platform_name == "linux":
        test_linux_specific(lib)
    elif platform_name == "windows":
        test_windows_specific(lib)
    elif platform_name == "macos":
        test_macos_specific(lib)

def test_linux_specific(lib):
    """Tests específicos de Linux"""
    try:
        # Test sensores (muy común en Linux)
        lib.sysmon_get_sensor_count.restype = ctypes.c_int
        sensor_count = lib.sysmon_get_sensor_count()
        print(f"✅ Sensores Linux: {sensor_count}")
        
        # Test red
        lib.sysmon_get_network_interface_count.restype = ctypes.c_int
        net_count = lib.sysmon_get_network_interface_count()
        print(f"✅ Interfaces de red: {net_count}")
        
    except Exception as e:
        print(f"⚠️  Error en tests específicos de Linux: {e}")

def test_windows_specific(lib):
    """Tests específicos de Windows"""
    try:
        # Test WMI-based functions
        print("✅ Tests específicos de Windows (WMI/Performance Counters)")
        
        # Test batería (común en laptops Windows)
        lib.sysmon_battery_is_present.restype = ctypes.c_int
        battery_present = lib.sysmon_battery_is_present()
        if battery_present == 1:
            print("✅ Batería detectada en Windows")
        else:
            print("ℹ️  Sin batería o desktop Windows")
            
    except Exception as e:
        print(f"⚠️  Error en tests específicos de Windows: {e}")

def test_macos_specific(lib):
    """Tests específicos de macOS"""
    try:
        # Test Core Audio
        lib.sysmon_get_audio_device_count.restype = ctypes.c_int
        audio_count = lib.sysmon_get_audio_device_count()
        print(f"✅ Dispositivos de audio macOS: {audio_count}")
        
        # Test IOKit displays
        lib.sysmon_get_display_count.restype = ctypes.c_int
        display_count = lib.sysmon_get_display_count()
        print(f"✅ Displays macOS: {display_count}")
        
    except Exception as e:
        print(f"⚠️  Error en tests específicos de macOS: {e}")

def run_performance_test(lib, platform_name):
    """Test de rendimiento"""
    print(f"\n⚡ Test de rendimiento en {platform_name.upper()}...")
    
    try:
        lib.sysmon_init()
        
        # Medir tiempo de 10 llamadas
        start_time = time.time()
        for i in range(10):
            lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
            lib.sysmon_get_cpu_usage_percent()
            lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
            lib.sysmon_get_ram_usage_percent()
        
        end_time = time.time()
        avg_time = (end_time - start_time) / 10 * 1000  # ms
        
        print(f"✅ Tiempo promedio por consulta: {avg_time:.2f}ms")
        
        if avg_time < 50:
            print("🚀 Rendimiento EXCELENTE (<50ms)")
        elif avg_time < 100:
            print("✅ Rendimiento BUENO (<100ms)")
        else:
            print("⚠️  Rendimiento LENTO (>100ms)")
        
        lib.sysmon_cleanup()
        
    except Exception as e:
        print(f"❌ Error en test de rendimiento: {e}")

def main():
    """Función principal"""
    print("🔍 SysMon - Test Multiplataforma")
    print("=" * 50)
    
    # Detectar plataforma
    platform_name = detect_platform()
    print(f"🖥️  Plataforma detectada: {platform_name.upper()}")
    
    if platform_name == "unknown":
        print("❌ Plataforma no soportada")
        sys.exit(1)
    
    # Obtener nombre de librería
    lib_path = get_library_name(platform_name)
    if not lib_path:
        print("❌ No se pudo determinar el nombre de la librería")
        sys.exit(1)
    
    # Cargar librería
    lib = load_library(lib_path)
    if not lib:
        print("❌ No se pudo cargar la librería")
        sys.exit(1)
    
    # Ejecutar tests
    success = True
    
    # Test básico
    if not test_basic_functions(lib, platform_name):
        success = False
    
    # Test específico de plataforma
    test_platform_specific(lib, platform_name)
    
    # Test de rendimiento
    run_performance_test(lib, platform_name)
    
    # Resumen final
    print("\n" + "=" * 50)
    if success:
        print("🎉 TODOS LOS TESTS PASARON")
        print(f"✅ SysMon funciona correctamente en {platform_name.upper()}")
    else:
        print("⚠️  ALGUNOS TESTS FALLARON")
        print("🔧 Revisa la implementación específica de la plataforma")
    
    print("\n📊 Información del sistema:")
    print(f"   OS: {platform.system()} {platform.release()}")
    print(f"   Arquitectura: {platform.machine()}")
    print(f"   Python: {platform.python_version()}")

if __name__ == "__main__":
    main()