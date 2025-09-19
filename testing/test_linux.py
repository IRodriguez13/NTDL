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

# ------------------ MAIN ------------------
def main():
    print("=== SysMonitor Test Linux ===")

    if platform.system() != "Linux":
        print("Este script solo funciona en Linux")
        return

    # Cargar la librería
    lib = ctypes.CDLL("./../libsysmon.so")

    # ------------------ CONFIGURACIÓN TIPOS ------------------
    lib.alloc_cpu_info.restype = POINTER(CpuInfo)
    lib.get_cpu_usage.restype = c_double
    lib.get_cpu_model.argtypes = [c_char_p, ctypes.c_size_t]
    lib.get_cpu_model.restype = c_int

    lib.alloc_disk_info.restype = POINTER(DiskInfo)
    lib.get_disk_smart.argtypes = [c_char_p, POINTER(DiskInfo)]
    lib.get_disk_smart.restype = c_int

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
    cpu_ptr = lib.alloc_cpu_info()
    cpu = cpu_ptr.contents
    print("\nCPU Info (C):")
    print(f"   Vendor: {cpu.vendor.decode(errors='ignore')}")
    print(f"   Model:  {cpu.model.decode(errors='ignore')}")
    print(f"   Cores:  {cpu.cores}")
    print(f"   MHz:    {cpu.mhz:.0f}")

    # ------------------ DISCO ------------------
    disk_ptr = lib.alloc_disk_info()
    res = lib.get_disk_smart(DISK_DEVICE, disk_ptr)
    disk = disk_ptr.contents
    print("\nDisk Info (C):")
    print(f"   Name: {disk.name.decode(errors='ignore')}")
    if res == 0:
        print(f"   Temperature: {disk.temperature} °C")
        print(f"   Power On Hours: {disk.power_on_hours} h")
    else:
        print("   SMART no disponible (fallback a info básica)")

    # ------------------ MONITOREO EN TIEMPO REAL ------------------
    print("\nMonitoreo en tiempo real (10s):")
    print(f"{'Tiempo':<6} {'CPU%':<8} {'FreqMHz':<10} {'RAM%':<8}")
    start_time = time.time()
    while time.time() - start_time < 10:
        cpu_usage = lib.get_cpu_usage()
        cpu_ptr = lib.alloc_cpu_info()
        freq = cpu_ptr.contents.mhz
        mem = psutil.virtual_memory()
        elapsed = time.time() - start_time
        print(f"{elapsed:5.1f}s  {cpu_usage:6.2f}%   {freq:8.0f}MHz {mem.percent:6.1f}%")
        time.sleep(0.5)

if __name__ == "__main__":
    main()
