#!/usr/bin/env python3
"""
Test Simple Modular para SysMon
Prueba básica del sistema modularizado sin estructuras complejas
"""

import ctypes
import time
import platform
import sys
import os

# Configurar LD_LIBRARY_PATH si es necesario
os.environ['LD_LIBRARY_PATH'] = '.'

class Colors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

class SysMonSimple:
    def __init__(self):
        self.lib = None
        self.library_loaded = False
        
        library_path = "./libsysmon.so"
        
        try:
            self.lib = ctypes.CDLL(library_path)
            self._configure_library()
            self.library_loaded = True
            print(f"{Colors.OKGREEN}✅ Biblioteca {library_path} cargada correctamente{Colors.ENDC}")
        except Exception as e:
            print(f"{Colors.FAIL}❌ Error cargando biblioteca: {e}{Colors.ENDC}")
            self.library_loaded = False
    
    def _configure_library(self):
        if not self.lib:
            return
        
        # Funciones básicas
        self.lib.sysmon_init.restype = ctypes.c_int
        self.lib.sysmon_cleanup.restype = None
        
        # Getters simples
        self.lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_temperature.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_frequency.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_cores.restype = ctypes.c_int
        
        # Funciones de string
        self.lib.sysmon_get_cpu_model.restype = ctypes.c_char_p
        self.lib.sysmon_get_os_name.restype = ctypes.c_char_p
        self.lib.sysmon_get_hostname.restype = ctypes.c_char_p
    
    def initialize(self):
        if not self.library_loaded:
            return False
        
        result = self.lib.sysmon_init()
        return result == 0
    
    def cleanup(self):
        if self.library_loaded:
            self.lib.sysmon_cleanup()
    
    def get_basic_info(self):
        if not self.library_loaded:
            return None
        
        try:
            info = {
                'cpu_usage': self.lib.sysmon_get_cpu_usage_percent(),
                'ram_usage': self.lib.sysmon_get_ram_usage_percent(),
                'cpu_temp': self.lib.sysmon_get_cpu_temperature(),
                'cpu_freq': self.lib.sysmon_get_cpu_frequency(),
                'cpu_cores': self.lib.sysmon_get_cpu_cores(),
                'cpu_model': self.lib.sysmon_get_cpu_model().decode('utf-8') if self.lib.sysmon_get_cpu_model() else "Unknown",
                'os_name': self.lib.sysmon_get_os_name().decode('utf-8') if self.lib.sysmon_get_os_name() else "Unknown",
                'hostname': self.lib.sysmon_get_hostname().decode('utf-8') if self.lib.sysmon_get_hostname() else "Unknown"
            }
            return info
        except Exception as e:
            print(f"{Colors.WARNING}⚠ Error obteniendo información: {e}{Colors.ENDC}")
            return None

def main():
    print(f"{Colors.BOLD}=== TEST SIMPLE MODULAR ==={Colors.ENDC}")
    
    # Inicializar SysMon
    sysmon = SysMonSimple()
    
    if not sysmon.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca{Colors.ENDC}")
        return 1
    
    # Inicializar el sistema
    if not sysmon.initialize():
        print(f"{Colors.FAIL}❌ No se pudo inicializar el sistema{Colors.ENDC}")
        return 1
    
    print(f"{Colors.OKGREEN}✅ Sistema inicializado correctamente{Colors.ENDC}")
    
    # Obtener información básica
    print(f"\n{Colors.BOLD}Información básica del sistema:{Colors.ENDC}")
    
    for i in range(5):
        info = sysmon.get_basic_info()
        if info:
            print(f"  Iteración {i+1}:")
            print(f"    OS: {info['os_name']}")
            print(f"    Hostname: {info['hostname']}")
            print(f"    CPU Model: {info['cpu_model']}")
            print(f"    CPU Cores: {info['cpu_cores']}")
            print(f"    CPU Usage: {info['cpu_usage']:.1f}%")
            print(f"    RAM Usage: {info['ram_usage']:.1f}%")
            print(f"    CPU Freq: {info['cpu_freq']:.0f} MHz")
            if info['cpu_temp'] > 0:
                print(f"    CPU Temp: {info['cpu_temp']:.1f}°C")
            print()
        else:
            print(f"  {Colors.WARNING}⚠ Error en iteración {i+1}{Colors.ENDC}")
        
        time.sleep(1)
    
    print(f"{Colors.OKGREEN}✅ Test simple completado exitosamente{Colors.ENDC}")
    
    # Limpiar recursos
    sysmon.cleanup()
    return 0

if __name__ == "__main__":
    sys.exit(main())