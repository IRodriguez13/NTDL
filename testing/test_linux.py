#!/usr/bin/env python3
import ctypes
import time
import platform
import psutil
import sys
from ctypes import POINTER, Structure, c_char, c_char_p, c_int, c_double

# Configurar LD_LIBRARY_PATH si es necesario
import os
os.environ['LD_LIBRARY_PATH'] = '.'

def ensure_root():
    if os.geteuid() != 0:
        print("⚠ Atención: Para leer información SMART del disco se requieren permisos de root.")
        ans = input("¿Desea continuar con permisos elevados? [s/N]: ").strip().lower()
        if ans == 's':
            # Re-ejecutar el script con sudo
            print("Intentando ejecutar con sudo...")
            os.execvp("sudo", ["sudo", sys.executable] + sys.argv)
        else:
            print("Continuando sin permisos de root (solo información básica del disco).")

ensure_root()

# ------------------ CONSTANTES ------------------
CPU_MODEL_LEN = 128
DISK_NAME_LEN = 64
DISK_DEVICE = b"/dev/sda"  # disco a consultar

# ------------------ STRUCTS ------------------
class CpuInfo(Structure):
    _fields_ = [
        ("vendor", c_char_p),  # puntero a char
        ("model", c_char_p),   # puntero a char
        ("cores", c_int),
        ("mhz", c_double)
    ]

class DiskInfo(Structure):
    _fields_ = [
        ("name", c_char * 128),  # array fijo
        ("temperature", c_int),
        ("power_on_hours", c_int)
    ]

# ------------------ FUNCIONES ------------------
def get_system_info():
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

def test_smart_methods(lib, device):
    """Testea diferentes métodos SMART disponibles y retorna el mejor resultado"""
    # Probar método principal primero
    disk_ptr = lib.alloc_disk_info()
    res = lib.get_disk_smart(device, disk_ptr)
    
    if res == 0:
        return disk_ptr.contents, 0
    
    # Si el método principal falla, probar smartctl si existe
    if hasattr(lib, 'get_disk_smart_via_smartctl'):
        time.sleep(0.1)  # Pequeño delay
        disk_ptr2 = lib.alloc_disk_info()
        res2 = lib.get_disk_smart_via_smartctl(device, disk_ptr2)
        if res2 == 0:
            return disk_ptr2.contents, 0
    
    # Si todo falla, retornar el resultado original
    return disk_ptr.contents, res

# ------------------ MAIN ------------------
def main():
    print("=== SysMonitor Test Linux ===")

    if platform.system() != "Linux":
        print("Este script solo funciona en Linux")
        return

    # Cargar la librería
    try:
        lib = ctypes.CDLL("./../libsysmon.so")
    except OSError as e:
        print(f"Error cargando librería: {e}")
        return

    # ------------------ CONFIGURACIÓN TIPOS ------------------
    lib.alloc_cpu_info.restype = POINTER(CpuInfo)
    lib.get_cpu_usage.restype = c_double
    lib.get_cpu_model.argtypes = [c_char_p, ctypes.c_size_t]
    lib.get_cpu_model.restype = c_int

    lib.alloc_disk_info.restype = POINTER(DiskInfo)
    lib.get_disk_smart.argtypes = [c_char_p, POINTER(DiskInfo)]
    lib.get_disk_smart.restype = c_int
    
    # Configurar funciones adicionales si existen
    if hasattr(lib, 'get_disk_smart_via_smartctl'):
        lib.get_disk_smart_via_smartctl.argtypes = [c_char_p, POINTER(DiskInfo)]
        lib.get_disk_smart_via_smartctl.restype = c_int
        
    if hasattr(lib, 'check_smart_enabled'):
        lib.check_smart_enabled.argtypes = [c_char_p]
        lib.check_smart_enabled.restype = c_int

    # ------------------ INFORMACIÓN DEL SISTEMA ------------------
    sys_info = get_system_info()
    print("\nSistema:")
    print(f"   Plataforma: {sys_info['platform']} {sys_info['platform_version']}")
    print(f"   Arquitectura: {sys_info['machine']}")
    print(f"   Procesador: {sys_info['processor']}")
    print(f"   Núcleos: {sys_info['cpu_count']}")
    if sys_info['cpu_freq']:
        print(f"   Frecuencia CPU: {sys_info['cpu_freq'].current:.0f} MHz")
    print(f"   Memoria total: {sys_info['memory'].total / (1024**3):.1f} GB")
    print(f"   Memoria disponible: {sys_info['memory'].available / (1024**3):.1f} GB")

    # ------------------ CPU ------------------
    print("\nObteniendo información de CPU...")
    time.sleep(0.5)  # Delay visual
    cpu_ptr = lib.alloc_cpu_info()
    if cpu_ptr:
        cpu = cpu_ptr.contents
        print("CPU Info (C):")
        print(f"   Vendor: {cpu.vendor.decode(errors='ignore') if cpu.vendor else 'N/A'}")
        print(f"   Model:  {cpu.model.decode(errors='ignore') if cpu.model else 'N/A'}")
        print(f"   Cores:  {cpu.cores}")
        print(f"   MHz:    {cpu.mhz:.0f}")
    else:
        print("\nError: No se pudo obtener información de CPU")

    # ------------------ DISCO ------------------
    print("\nObteniendo información de disco...")
    time.sleep(0.5)  # Delay visual
    disk, res = test_smart_methods(lib, DISK_DEVICE)
    print("Disk Info (C):")
    print(f"   Name: {disk.name.decode(errors='ignore')}")
    if res == 0:
        print(f"   Temperature: {disk.temperature} °C")
        print(f"   Power On Hours: {disk.power_on_hours} h")
    else:
        print("   SMART no disponible (fallback a info básica)")
    
    # Verificar otros discos silenciosamente
    other_devices = [b"/dev/nvme0n1", b"/dev/sdb"]
    for device in other_devices:
        if os.path.exists(device.decode()):
            time.sleep(0.3)  # Pequeño delay entre discos
            disk_other, res_other = test_smart_methods(lib, device)
            if res_other == 0:
                print(f"\nDisk adicional encontrado ({device.decode()}):")
                print(f"   Name: {disk_other.name.decode(errors='ignore')}")
                print(f"   Temperature: {disk_other.temperature} °C")
                print(f"   Power On Hours: {disk_other.power_on_hours} h")

    # ------------------ MONITOREO EN TIEMPO REAL (OPCIONAL) ------------------
    print("\n¿Ejecutar monitoreo en tiempo real por 10s? [s/N]: ", end="")
    if input().strip().lower() == 's':
        print("\nMonitoreo en tiempo real (10s):")
        print(f"{'Tiempo':<6} {'CPU%':<8} {'FreqMHz':<10} {'RAM%':<8}")
        start_time = time.time()
        while time.time() - start_time < 10:
            try:
                cpu_usage = lib.get_cpu_usage()
                cpu_ptr = lib.alloc_cpu_info()
                freq = cpu_ptr.contents.mhz if cpu_ptr else 0
                mem = psutil.virtual_memory()
                elapsed = time.time() - start_time
                print(f"{elapsed:5.1f}s  {cpu_usage:6.2f}%   {freq:8.0f}MHz {mem.percent:6.1f}%")
                time.sleep(0.5)
            except KeyboardInterrupt:
                print("\nMonitoreo interrumpido por usuario")
                break
            except Exception as e:
                print(f"Error en monitoreo: {e}")
                break

if __name__ == "__main__":
    main()