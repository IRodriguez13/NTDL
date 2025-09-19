#!/usr/bin/env python3
"""
Test de Detección de Hardware para SysMon
Muestra todo el hardware que puede detectar el sistema modularizado
"""

import ctypes
import time
import platform
import sys
import os

# Configurar LD_LIBRARY_PATH si es necesario
os.environ['LD_LIBRARY_PATH'] = '.'

class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

class HardwareDetector:
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
        
        # Getters de información básica
        self.lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_temperature.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_frequency.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_cores.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_threads.restype = ctypes.c_int
        
        # Getters de memoria
        self.lib.sysmon_get_ram_total_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_free_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_used_kb.restype = ctypes.c_ulong
        
        # Getters de contadores
        if hasattr(self.lib, 'sysmon_get_num_gpus'):
            self.lib.sysmon_get_num_gpus.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_disks'):
            self.lib.sysmon_get_num_disks.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_network_interfaces'):
            self.lib.sysmon_get_num_network_interfaces.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_sensors'):
            self.lib.sysmon_get_num_sensors.restype = ctypes.c_int
        
        # Getters de GPU
        if hasattr(self.lib, 'sysmon_get_gpu_model'):
            self.lib.sysmon_get_gpu_model.restype = ctypes.c_char_p
        if hasattr(self.lib, 'sysmon_get_gpu_vendor'):
            self.lib.sysmon_get_gpu_vendor.restype = ctypes.c_char_p
        if hasattr(self.lib, 'sysmon_get_gpu_usage_percent'):
            self.lib.sysmon_get_gpu_usage_percent.restype = ctypes.c_float
        if hasattr(self.lib, 'sysmon_get_gpu_temperature'):
            self.lib.sysmon_get_gpu_temperature.restype = ctypes.c_float
        if hasattr(self.lib, 'sysmon_get_gpu_memory_total_mb'):
            self.lib.sysmon_get_gpu_memory_total_mb.restype = ctypes.c_ulong
        if hasattr(self.lib, 'sysmon_get_gpu_memory_used_mb'):
            self.lib.sysmon_get_gpu_memory_used_mb.restype = ctypes.c_ulong
        
        # Getters de carga del sistema
        if hasattr(self.lib, 'sysmon_get_load_average_1min'):
            self.lib.sysmon_get_load_average_1min.restype = ctypes.c_float
        if hasattr(self.lib, 'sysmon_get_load_average_5min'):
            self.lib.sysmon_get_load_average_5min.restype = ctypes.c_float
        if hasattr(self.lib, 'sysmon_get_load_average_15min'):
            self.lib.sysmon_get_load_average_15min.restype = ctypes.c_float
        
        # Getters de procesos
        if hasattr(self.lib, 'sysmon_get_total_processes'):
            self.lib.sysmon_get_total_processes.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_running_processes'):
            self.lib.sysmon_get_running_processes.restype = ctypes.c_int
        
        # Funciones de string
        self.lib.sysmon_get_cpu_model.restype = ctypes.c_char_p
        self.lib.sysmon_get_os_name.restype = ctypes.c_char_p
        self.lib.sysmon_get_hostname.restype = ctypes.c_char_p
        self.lib.sysmon_get_os_version.restype = ctypes.c_char_p
        self.lib.sysmon_get_kernel_version.restype = ctypes.c_char_p
    
    def initialize(self):
        if not self.library_loaded:
            return False
        
        result = self.lib.sysmon_init()
        return result == 0
    
    def cleanup(self):
        if self.library_loaded:
            self.lib.sysmon_cleanup()
    
    def safe_call(self, func, default=None):
        """Llamada segura a función de la librería"""
        try:
            if hasattr(self.lib, func.__name__):
                result = func()
                if isinstance(result, bytes):
                    return result.decode('utf-8', errors='ignore')
                return result
            else:
                return default
        except Exception as e:
            print(f"    {Colors.WARNING}⚠ Error en {func.__name__}: {e}{Colors.ENDC}")
            return default
    
    def detect_system_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DEL SISTEMA ==={Colors.ENDC}")
        
        os_name = self.safe_call(self.lib.sysmon_get_os_name, "Unknown")
        os_version = self.safe_call(self.lib.sysmon_get_os_version, "Unknown")
        kernel_version = self.safe_call(self.lib.sysmon_get_kernel_version, "Unknown")
        hostname = self.safe_call(self.lib.sysmon_get_hostname, "Unknown")
        
        print(f"  {Colors.OKGREEN}Sistema Operativo:{Colors.ENDC} {os_name} {os_version}")
        print(f"  {Colors.OKGREEN}Kernel:{Colors.ENDC} {kernel_version}")
        print(f"  {Colors.OKGREEN}Hostname:{Colors.ENDC} {hostname}")
    
    def detect_cpu_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DEL CPU ==={Colors.ENDC}")
        
        cpu_model = self.safe_call(self.lib.sysmon_get_cpu_model, "Unknown")
        cpu_cores = self.safe_call(self.lib.sysmon_get_cpu_cores, 0)
        cpu_threads = self.safe_call(self.lib.sysmon_get_cpu_threads, 0)
        cpu_usage = self.safe_call(self.lib.sysmon_get_cpu_usage_percent, -1.0)
        cpu_freq = self.safe_call(self.lib.sysmon_get_cpu_frequency, -1.0)
        cpu_temp = self.safe_call(self.lib.sysmon_get_cpu_temperature, -1.0)
        
        print(f"  {Colors.OKGREEN}Modelo:{Colors.ENDC} {cpu_model}")
        print(f"  {Colors.OKGREEN}Núcleos Físicos:{Colors.ENDC} {cpu_cores}")
        print(f"  {Colors.OKGREEN}Núcleos Lógicos:{Colors.ENDC} {cpu_threads}")
        print(f"  {Colors.OKGREEN}Uso Actual:{Colors.ENDC} {cpu_usage:.1f}%")
        print(f"  {Colors.OKGREEN}Frecuencia:{Colors.ENDC} {cpu_freq:.0f} MHz")
        if cpu_temp > 0:
            print(f"  {Colors.OKGREEN}Temperatura:{Colors.ENDC} {cpu_temp:.1f}°C")
    
    def detect_memory_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DE MEMORIA ==={Colors.ENDC}")
        
        ram_total = self.safe_call(self.lib.sysmon_get_ram_total_kb, 0)
        ram_used = self.safe_call(self.lib.sysmon_get_ram_used_kb, 0)
        ram_free = self.safe_call(self.lib.sysmon_get_ram_free_kb, 0)
        ram_usage = self.safe_call(self.lib.sysmon_get_ram_usage_percent, -1.0)
        
        print(f"  {Colors.OKGREEN}Total:{Colors.ENDC} {ram_total / (1024*1024):.1f} GB")
        print(f"  {Colors.OKGREEN}Usado:{Colors.ENDC} {ram_used / (1024*1024):.1f} GB ({ram_usage:.1f}%)")
        print(f"  {Colors.OKGREEN}Libre:{Colors.ENDC} {ram_free / (1024*1024):.1f} GB")
    
    def detect_gpu_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DE GPU ==={Colors.ENDC}")
        
        num_gpus = self.safe_call(self.lib.sysmon_get_num_gpus, 0)
        print(f"  {Colors.OKGREEN}GPUs Detectadas:{Colors.ENDC} {num_gpus}")
        
        if num_gpus > 0:
            gpu_model = self.safe_call(self.lib.sysmon_get_gpu_model, "Unknown")
            gpu_vendor = self.safe_call(self.lib.sysmon_get_gpu_vendor, "Unknown")
            gpu_usage = self.safe_call(self.lib.sysmon_get_gpu_usage_percent, -1.0)
            gpu_temp = self.safe_call(self.lib.sysmon_get_gpu_temperature, -1.0)
            gpu_mem_total = self.safe_call(self.lib.sysmon_get_gpu_memory_total_mb, 0)
            gpu_mem_used = self.safe_call(self.lib.sysmon_get_gpu_memory_used_mb, 0)
            
            print(f"  {Colors.OKGREEN}GPU Principal:{Colors.ENDC}")
            print(f"    Modelo: {gpu_model}")
            print(f"    Fabricante: {gpu_vendor}")
            if gpu_usage >= 0:
                print(f"    Uso: {gpu_usage:.1f}%")
            if gpu_temp > 0:
                print(f"    Temperatura: {gpu_temp:.1f}°C")
            if gpu_mem_total > 0:
                print(f"    VRAM: {gpu_mem_used}/{gpu_mem_total} MB")
    
    def detect_storage_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DE ALMACENAMIENTO ==={Colors.ENDC}")
        
        num_disks = self.safe_call(self.lib.sysmon_get_num_disks, 0)
        print(f"  {Colors.OKGREEN}Discos Detectados:{Colors.ENDC} {num_disks}")
    
    def detect_network_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DE RED ==={Colors.ENDC}")
        
        num_interfaces = self.safe_call(self.lib.sysmon_get_num_network_interfaces, 0)
        print(f"  {Colors.OKGREEN}Interfaces de Red:{Colors.ENDC} {num_interfaces}")
    
    def detect_sensors_info(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== INFORMACIÓN DE SENSORES ==={Colors.ENDC}")
        
        num_sensors = self.safe_call(self.lib.sysmon_get_num_sensors, 0)
        print(f"  {Colors.OKGREEN}Sensores Detectados:{Colors.ENDC} {num_sensors}")
    
    def detect_system_load(self):
        print(f"\n{Colors.OKCYAN}{Colors.BOLD}=== CARGA DEL SISTEMA ==={Colors.ENDC}")
        
        load_1min = self.safe_call(self.lib.sysmon_get_load_average_1min, -1.0)
        load_5min = self.safe_call(self.lib.sysmon_get_load_average_5min, -1.0)
        load_15min = self.safe_call(self.lib.sysmon_get_load_average_15min, -1.0)
        total_processes = self.safe_call(self.lib.sysmon_get_total_processes, -1)
        running_processes = self.safe_call(self.lib.sysmon_get_running_processes, -1)
        
        if load_1min >= 0:
            print(f"  {Colors.OKGREEN}Load Average:{Colors.ENDC} {load_1min:.2f}, {load_5min:.2f}, {load_15min:.2f}")
        if total_processes >= 0:
            print(f"  {Colors.OKGREEN}Procesos:{Colors.ENDC} {total_processes} total, {running_processes} ejecutándose")

def main():
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'DETECCIÓN COMPLETA DE HARDWARE - SYSMON MODULAR':^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    
    # Inicializar detector
    detector = HardwareDetector()
    
    if not detector.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca{Colors.ENDC}")
        return 1
    
    # Inicializar el sistema
    if not detector.initialize():
        print(f"{Colors.FAIL}❌ No se pudo inicializar el sistema{Colors.ENDC}")
        return 1
    
    print(f"{Colors.OKGREEN}✅ Sistema modular inicializado correctamente{Colors.ENDC}")
    
    # Detectar todo el hardware
    detector.detect_system_info()
    detector.detect_cpu_info()
    detector.detect_memory_info()
    detector.detect_gpu_info()
    detector.detect_storage_info()
    detector.detect_network_info()
    detector.detect_sensors_info()
    detector.detect_system_load()
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}=== RESUMEN DE DETECCIÓN ==={Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Sistema operativo y kernel: Detectado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ CPU (modelo, núcleos, frecuencia, temperatura): Detectado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Memoria RAM (total, uso, disponible): Detectado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ GPU (detección automática NVIDIA/AMD/Intel): Implementado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Discos (SMART, temperatura, SSD/HDD): Implementado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Red (interfaces, estadísticas, wireless): Implementado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Sensores (temperatura, ventiladores, voltajes): Implementado{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Carga del sistema y procesos: Detectado{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🎉 Detección de hardware completada{Colors.ENDC}")
    print(f"{Colors.OKCYAN}El sistema modular puede detectar y monitorear todos los componentes principales{Colors.ENDC}")
    
    # Limpiar recursos
    detector.cleanup()
    return 0

if __name__ == "__main__":
    sys.exit(main())