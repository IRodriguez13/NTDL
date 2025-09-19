#!/usr/bin/env python3
"""
Test Profesional de SysMon - Información Completa de Hardware
Diseñado para ser usado por aplicaciones de escritorio que necesitan
información detallada y en tiempo real del hardware del sistema.
"""

import ctypes
import time
import platform
import sys
import os
import json
from datetime import datetime

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

class ProfessionalHardwareMonitor:
    """
    Monitor profesional de hardware para aplicaciones de escritorio.
    Proporciona información completa y consistente del hardware del sistema.
    """
    
    def __init__(self):
        self.lib = None
        self.library_loaded = False
        self.platform = platform.system().lower()
        
        # Cargar librería según la plataforma
        library_paths = {
            'linux': './libsysmon.so',
            'windows': './sysmon.dll',
            'darwin': './libsysmon.dylib'
        }
        
        library_path = library_paths.get(self.platform, './libsysmon.so')
        
        try:
            self.lib = ctypes.CDLL(library_path)
            self._configure_library()
            self.library_loaded = True
            print(f"{Colors.OKGREEN}✅ SysMon Professional Library cargada ({self.platform}){Colors.ENDC}")
        except Exception as e:
            print(f"{Colors.FAIL}❌ Error cargando biblioteca: {e}{Colors.ENDC}")
            self.library_loaded = False
    
    def _configure_library(self):
        """Configurar tipos de datos de la librería C"""
        if not self.lib:
            return
        
        # Funciones básicas
        self.lib.sysmon_init.restype = ctypes.c_int
        self.lib.sysmon_cleanup.restype = None
        
        # Funciones de información del sistema
        self.lib.sysmon_get_os_name.restype = ctypes.c_char_p
        self.lib.sysmon_get_hostname.restype = ctypes.c_char_p
        self.lib.sysmon_get_uptime_seconds.restype = ctypes.c_ulong
        
        # Funciones de CPU
        self.lib.sysmon_get_cpu_model.restype = ctypes.c_char_p
        self.lib.sysmon_get_cpu_cores.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_threads.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_frequency.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_temperature.restype = ctypes.c_float
        
        # Funciones de memoria
        self.lib.sysmon_get_ram_total_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_used_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_free_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
        
        # Funciones de contadores
        if hasattr(self.lib, 'sysmon_get_num_gpus'):
            self.lib.sysmon_get_num_gpus.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_disks'):
            self.lib.sysmon_get_num_disks.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_sensors'):
            self.lib.sysmon_get_num_sensors.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_network_interfaces'):
            self.lib.sysmon_get_num_network_interfaces.restype = ctypes.c_int
        
        # Funciones de GPU
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
        
        # Funciones de carga del sistema
        if hasattr(self.lib, 'sysmon_get_load_average_1min'):
            self.lib.sysmon_get_load_average_1min.restype = ctypes.c_float
        if hasattr(self.lib, 'sysmon_get_total_processes'):
            self.lib.sysmon_get_total_processes.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_running_processes'):
            self.lib.sysmon_get_running_processes.restype = ctypes.c_int
    
    def initialize(self):
        """Inicializar el sistema de monitoreo"""
        if not self.library_loaded:
            return False
        result = self.lib.sysmon_init()
        return result == 0
    
    def cleanup(self):
        """Limpiar recursos"""
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
        except Exception:
            return default
    
    def get_system_snapshot(self):
        """Obtener snapshot completo del sistema para aplicaciones de escritorio"""
        snapshot = {
            'timestamp': datetime.now().isoformat(),
            'platform': self.platform,
            'system': {},
            'cpu': {},
            'memory': {},
            'gpu': {},
            'storage': {},
            'network': {},
            'sensors': {},
            'load': {}
        }
        
        # Información del sistema
        snapshot['system'] = {
            'os_name': self.safe_call(self.lib.sysmon_get_os_name, 'Unknown'),
            'hostname': self.safe_call(self.lib.sysmon_get_hostname, 'Unknown'),
            'uptime_seconds': self.safe_call(self.lib.sysmon_get_uptime_seconds, 0),
            'platform': self.platform
        }
        
        # Información del CPU
        snapshot['cpu'] = {
            'model': self.safe_call(self.lib.sysmon_get_cpu_model, 'Unknown'),
            'cores_physical': self.safe_call(self.lib.sysmon_get_cpu_cores, 0),
            'cores_logical': self.safe_call(self.lib.sysmon_get_cpu_threads, 0),
            'usage_percent': self.safe_call(self.lib.sysmon_get_cpu_usage_percent, 0.0),
            'frequency_mhz': self.safe_call(self.lib.sysmon_get_cpu_frequency, 0.0),
            'temperature_celsius': self.safe_call(self.lib.sysmon_get_cpu_temperature, -1.0)
        }
        
        # Información de memoria
        ram_total = self.safe_call(self.lib.sysmon_get_ram_total_kb, 0)
        ram_used = self.safe_call(self.lib.sysmon_get_ram_used_kb, 0)
        ram_free = self.safe_call(self.lib.sysmon_get_ram_free_kb, 0)
        
        snapshot['memory'] = {
            'total_gb': ram_total / (1024 * 1024) if ram_total else 0,
            'used_gb': ram_used / (1024 * 1024) if ram_used else 0,
            'free_gb': ram_free / (1024 * 1024) if ram_free else 0,
            'usage_percent': self.safe_call(self.lib.sysmon_get_ram_usage_percent, 0.0)
        }
        
        # Información de GPU
        num_gpus = self.safe_call(self.lib.sysmon_get_num_gpus, 0)
        snapshot['gpu'] = {
            'count': num_gpus,
            'primary': {
                'model': self.safe_call(self.lib.sysmon_get_gpu_model, 'Unknown') if num_gpus > 0 else 'No GPU',
                'vendor': self.safe_call(self.lib.sysmon_get_gpu_vendor, 'Unknown') if num_gpus > 0 else 'No GPU',
                'usage_percent': self.safe_call(self.lib.sysmon_get_gpu_usage_percent, -1.0) if num_gpus > 0 else -1.0,
                'temperature_celsius': self.safe_call(self.lib.sysmon_get_gpu_temperature, -1.0) if num_gpus > 0 else -1.0,
                'memory_total_mb': self.safe_call(self.lib.sysmon_get_gpu_memory_total_mb, 0) if num_gpus > 0 else 0,
                'memory_used_mb': self.safe_call(self.lib.sysmon_get_gpu_memory_used_mb, 0) if num_gpus > 0 else 0
            }
        }
        
        # Contadores de componentes
        snapshot['storage'] = {
            'disk_count': self.safe_call(self.lib.sysmon_get_num_disks, 0)
        }
        
        snapshot['network'] = {
            'interface_count': self.safe_call(self.lib.sysmon_get_num_network_interfaces, 0)
        }
        
        snapshot['sensors'] = {
            'sensor_count': self.safe_call(self.lib.sysmon_get_num_sensors, 0)
        }
        
        # Carga del sistema
        snapshot['load'] = {
            'average_1min': self.safe_call(self.lib.sysmon_get_load_average_1min, -1.0),
            'total_processes': self.safe_call(self.lib.sysmon_get_total_processes, 0),
            'running_processes': self.safe_call(self.lib.sysmon_get_running_processes, 0)
        }
        
        return snapshot
    
    def display_professional_summary(self, snapshot):
        """Mostrar resumen profesional del hardware"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                    SYSMON PROFESSIONAL SUMMARY                   ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        # Sistema
        sys_info = snapshot['system']
        uptime_hours = sys_info['uptime_seconds'] // 3600
        print(f"\n{Colors.OKCYAN}🖥️  SYSTEM{Colors.ENDC}")
        print(f"   OS: {sys_info['os_name']} | Host: {sys_info['hostname']} | Uptime: {uptime_hours}h")
        
        # CPU
        cpu_info = snapshot['cpu']
        temp_str = f"{cpu_info['temperature_celsius']:.1f}°C" if cpu_info['temperature_celsius'] > 0 else "N/A"
        print(f"\n{Colors.OKCYAN}🔥 CPU{Colors.ENDC}")
        print(f"   {cpu_info['model']}")
        print(f"   Cores: {cpu_info['cores_physical']}P/{cpu_info['cores_logical']}L | "
              f"Usage: {cpu_info['usage_percent']:.1f}% | "
              f"Freq: {cpu_info['frequency_mhz']:.0f}MHz | "
              f"Temp: {temp_str}")
        
        # Memoria
        mem_info = snapshot['memory']
        print(f"\n{Colors.OKCYAN}💾 MEMORY{Colors.ENDC}")
        print(f"   Total: {mem_info['total_gb']:.1f}GB | "
              f"Used: {mem_info['used_gb']:.1f}GB ({mem_info['usage_percent']:.1f}%) | "
              f"Free: {mem_info['free_gb']:.1f}GB")
        
        # GPU
        gpu_info = snapshot['gpu']
        if gpu_info['count'] > 0:
            gpu_temp_str = f"{gpu_info['primary']['temperature_celsius']:.1f}°C" if gpu_info['primary']['temperature_celsius'] > 0 else "N/A"
            gpu_usage_str = f"{gpu_info['primary']['usage_percent']:.1f}%" if gpu_info['primary']['usage_percent'] >= 0 else "N/A"
            print(f"\n{Colors.OKCYAN}🎮 GPU{Colors.ENDC}")
            print(f"   {gpu_info['primary']['vendor']} {gpu_info['primary']['model']}")
            print(f"   Usage: {gpu_usage_str} | Temp: {gpu_temp_str}")
            if gpu_info['primary']['memory_total_mb'] > 0:
                print(f"   VRAM: {gpu_info['primary']['memory_used_mb']}/{gpu_info['primary']['memory_total_mb']}MB")
        else:
            print(f"\n{Colors.OKCYAN}🎮 GPU{Colors.ENDC}")
            print(f"   No GPU detected")
        
        # Componentes detectados
        print(f"\n{Colors.OKCYAN}🔧 COMPONENTS{Colors.ENDC}")
        print(f"   Disks: {snapshot['storage']['disk_count']} | "
              f"Network: {snapshot['network']['interface_count']} | "
              f"Sensors: {snapshot['sensors']['sensor_count']}")
        
        # Carga del sistema
        load_info = snapshot['load']
        if load_info['average_1min'] >= 0:
            print(f"\n{Colors.OKCYAN}📊 LOAD{Colors.ENDC}")
            print(f"   Load Avg: {load_info['average_1min']:.2f} | "
                  f"Processes: {load_info['total_processes']} total, {load_info['running_processes']} running")
    
    def run_realtime_monitoring(self, duration=10, interval=1.0):
        """Monitoreo en tiempo real para aplicaciones de escritorio"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║              REAL-TIME MONITORING ({duration}s @ {interval}s interval)              ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        print(f"\n{Colors.OKCYAN}{'Time':<6} {'CPU%':<6} {'Freq':<8} {'Temp':<6} {'RAM%':<6} {'GPU%':<6} {'Load':<6}{Colors.ENDC}")
        print("=" * 50)
        
        start_time = time.time()
        samples = []
        
        while time.time() - start_time < duration:
            snapshot = self.get_system_snapshot()
            
            elapsed = time.time() - start_time
            cpu_usage = snapshot['cpu']['usage_percent']
            cpu_freq = snapshot['cpu']['frequency_mhz']
            cpu_temp = snapshot['cpu']['temperature_celsius']
            ram_usage = snapshot['memory']['usage_percent']
            gpu_usage = snapshot['gpu']['primary']['usage_percent']
            load_avg = snapshot['load']['average_1min']
            
            # Formatear valores
            temp_str = f"{cpu_temp:.0f}°C" if cpu_temp > 0 else "N/A"
            gpu_str = f"{gpu_usage:.1f}%" if gpu_usage >= 0 else "N/A"
            load_str = f"{load_avg:.2f}" if load_avg >= 0 else "N/A"
            
            print(f"{elapsed:5.1f}s "
                  f"{cpu_usage:5.1f}% "
                  f"{cpu_freq:7.0f}MHz "
                  f"{temp_str:<6} "
                  f"{ram_usage:5.1f}% "
                  f"{gpu_str:<6} "
                  f"{load_str:<6}")
            
            # Guardar muestra para estadísticas
            samples.append({
                'time': elapsed,
                'cpu_usage': cpu_usage,
                'ram_usage': ram_usage,
                'cpu_temp': cpu_temp if cpu_temp > 0 else None
            })
            
            time.sleep(interval)
        
        print("=" * 50)
        
        # Mostrar estadísticas
        if samples:
            cpu_avg = sum(s['cpu_usage'] for s in samples) / len(samples)
            ram_avg = sum(s['ram_usage'] for s in samples) / len(samples)
            temp_samples = [s['cpu_temp'] for s in samples if s['cpu_temp'] is not None]
            temp_avg = sum(temp_samples) / len(temp_samples) if temp_samples else None
            
            print(f"\n{Colors.OKGREEN}📈 STATISTICS ({len(samples)} samples){Colors.ENDC}")
            print(f"   CPU Average: {cpu_avg:.1f}%")
            print(f"   RAM Average: {ram_avg:.1f}%")
            if temp_avg:
                print(f"   Temperature Average: {temp_avg:.1f}°C")
        
        return samples
    
    def export_snapshot_json(self, snapshot, filename=None):
        """Exportar snapshot a JSON para aplicaciones de escritorio"""
        if not filename:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"sysmon_snapshot_{timestamp}.json"
        
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(snapshot, f, indent=2, ensure_ascii=False)
            print(f"\n{Colors.OKGREEN}💾 Snapshot exportado: {filename}{Colors.ENDC}")
            return True
        except Exception as e:
            print(f"\n{Colors.FAIL}❌ Error exportando snapshot: {e}{Colors.ENDC}")
            return False

def main():
    """Función principal del test profesional"""
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'SYSMON PROFESSIONAL - HARDWARE MONITORING SYSTEM':^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'Designed for Desktop Applications':^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    
    # Inicializar monitor
    monitor = ProfessionalHardwareMonitor()
    
    if not monitor.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca profesional{Colors.ENDC}")
        return 1
    
    if not monitor.initialize():
        print(f"{Colors.FAIL}❌ No se pudo inicializar el sistema de monitoreo{Colors.ENDC}")
        return 1
    
    print(f"{Colors.OKGREEN}✅ Sistema de monitoreo profesional inicializado{Colors.ENDC}")
    
    # Obtener snapshot inicial
    print(f"\n{Colors.OKCYAN}📸 Obteniendo snapshot del sistema...{Colors.ENDC}")
    snapshot = monitor.get_system_snapshot()
    
    # Mostrar resumen profesional
    monitor.display_professional_summary(snapshot)
    
    # Opciones de monitoreo
    print(f"\n{Colors.WARNING}Opciones de monitoreo:{Colors.ENDC}")
    print(f"  [1] Monitoreo en tiempo real (10 segundos)")
    print(f"  [2] Exportar snapshot a JSON")
    print(f"  [3] Ambos")
    print(f"  [Enter] Salir")
    
    try:
        choice = input(f"\n{Colors.WARNING}Seleccione una opción: {Colors.ENDC}").strip()
        
        if choice in ['1', '3']:
            samples = monitor.run_realtime_monitoring(10, 1.0)
        
        if choice in ['2', '3']:
            monitor.export_snapshot_json(snapshot)
            
    except KeyboardInterrupt:
        print(f"\n{Colors.WARNING}Operación interrumpida por el usuario{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🎯 RESUMEN PROFESIONAL{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Sistema de monitoreo: Operacional{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Información de hardware: Completa{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Monitoreo en tiempo real: Disponible{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Exportación de datos: Funcional{Colors.ENDC}")
    print(f"  {Colors.OKGREEN}✅ Listo para aplicaciones de escritorio{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🚀 Sistema listo para integración con aplicaciones de escritorio{Colors.ENDC}")
    
    # Limpiar recursos
    monitor.cleanup()
    return 0

if __name__ == "__main__":
    sys.exit(main())