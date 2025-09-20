#!/usr/bin/env python3
"""
Test Detallado de Hardware para SysMon
Extrae la máxima información posible del hardware como CrystalDiskInfo, HWiNFO, etc.
"""

import ctypes
import time
import platform
import sys
import os
import subprocess

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

class DetailedHardwareMonitor:
    def __init__(self):
        self.lib = None
        self.library_loaded = False
        
        library_path = "./libsysmon.so"
        
        try:
            self.lib = ctypes.CDLL(library_path)
            self._configure_library()
            self.library_loaded = True
            print(f"{Colors.OKGREEN}✅ SysMon Library cargada correctamente{Colors.ENDC}")
        except Exception as e:
            print(f"{Colors.FAIL}❌ Error cargando biblioteca: {e}{Colors.ENDC}")
            self.library_loaded = False
    
    def _configure_library(self):
        if not self.lib:
            return
        
        # Funciones básicas
        self.lib.sysmon_init.restype = ctypes.c_int
        self.lib.sysmon_cleanup.restype = None
        
        # CPU
        self.lib.sysmon_get_cpu_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_temperature.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_frequency.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_cores.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_threads.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_model.restype = ctypes.c_char_p
        
        # Memoria
        self.lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_ram_total_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_used_kb.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_free_kb.restype = ctypes.c_ulong
        
        # Sistema
        self.lib.sysmon_get_os_name.restype = ctypes.c_char_p
        self.lib.sysmon_get_hostname.restype = ctypes.c_char_p
        self.lib.sysmon_get_uptime_seconds.restype = ctypes.c_ulong
        
        # Contadores
        if hasattr(self.lib, 'sysmon_get_num_gpus'):
            self.lib.sysmon_get_num_gpus.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_disks'):
            self.lib.sysmon_get_num_disks.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_sensors'):
            self.lib.sysmon_get_num_sensors.restype = ctypes.c_int
        if hasattr(self.lib, 'sysmon_get_num_network_interfaces'):
            self.lib.sysmon_get_num_network_interfaces.restype = ctypes.c_int
        
        # GPU
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
    
    def initialize(self):
        if not self.library_loaded:
            return False
        result = self.lib.sysmon_init()
        return result == 0
    
    def cleanup(self):
        if self.library_loaded:
            self.lib.sysmon_cleanup()
    
    def safe_call(self, func, default=None):
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
    
    def get_detailed_cpu_info(self):
        """Información detallada del CPU como HWiNFO"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                        INFORMACIÓN DETALLADA DEL CPU             ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        cpu_model = self.safe_call(self.lib.sysmon_get_cpu_model, "Unknown")
        cpu_cores = self.safe_call(self.lib.sysmon_get_cpu_cores, 0)
        cpu_threads = self.safe_call(self.lib.sysmon_get_cpu_threads, 0)
        
        print(f"  {Colors.OKCYAN}Procesador:{Colors.ENDC}")
        print(f"    {Colors.OKGREEN}Modelo:{Colors.ENDC} {cpu_model}")
        print(f"    {Colors.OKGREEN}Núcleos Físicos:{Colors.ENDC} {cpu_cores}")
        print(f"    {Colors.OKGREEN}Núcleos Lógicos:{Colors.ENDC} {cpu_threads}")
        
        # Información adicional desde /proc/cpuinfo
        try:
            with open('/proc/cpuinfo', 'r') as f:
                cpuinfo = f.read()
                
            # Extraer información adicional
            lines = cpuinfo.split('\n')
            cpu_info = {}
            for line in lines:
                if ':' in line:
                    key, value = line.split(':', 1)
                    key = key.strip()
                    value = value.strip()
                    if key not in cpu_info:
                        cpu_info[key] = value
            
            if 'vendor_id' in cpu_info:
                print(f"    {Colors.OKGREEN}Fabricante:{Colors.ENDC} {cpu_info['vendor_id']}")
            if 'cpu family' in cpu_info:
                print(f"    {Colors.OKGREEN}Familia:{Colors.ENDC} {cpu_info['cpu family']}")
            if 'model' in cpu_info:
                print(f"    {Colors.OKGREEN}Modelo ID:{Colors.ENDC} {cpu_info['model']}")
            if 'stepping' in cpu_info:
                print(f"    {Colors.OKGREEN}Stepping:{Colors.ENDC} {cpu_info['stepping']}")
            if 'microcode' in cpu_info:
                print(f"    {Colors.OKGREEN}Microcode:{Colors.ENDC} {cpu_info['microcode']}")
            if 'cache size' in cpu_info:
                print(f"    {Colors.OKGREEN}Cache L3:{Colors.ENDC} {cpu_info['cache size']}")
            if 'flags' in cpu_info:
                flags = cpu_info['flags'].split()
                important_flags = ['sse', 'sse2', 'sse3', 'ssse3', 'sse4_1', 'sse4_2', 'avx', 'avx2', 'aes', 'fma']
                present_flags = [flag for flag in important_flags if flag in flags]
                print(f"    {Colors.OKGREEN}Instrucciones:{Colors.ENDC} {', '.join(present_flags)}")
                
        except Exception as e:
            print(f"    {Colors.WARNING}⚠ Error leyendo /proc/cpuinfo: {e}{Colors.ENDC}")
    
    def get_detailed_disk_info(self):
        """Información detallada de discos como CrystalDiskInfo"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                    INFORMACIÓN DETALLADA DE DISCOS               ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        num_disks = self.safe_call(self.lib.sysmon_get_num_disks, 0)
        print(f"  {Colors.OKCYAN}Discos Detectados: {num_disks}{Colors.ENDC}")
        
        # Obtener información detallada de cada disco
        disk_devices = ['/dev/sda', '/dev/sdb', '/dev/nvme0n1', '/dev/nvme1n1']
        
        for i, device in enumerate(disk_devices):
            if not os.path.exists(device):
                continue
                
            print(f"\n  {Colors.OKCYAN}Disco {i+1}: {device}{Colors.ENDC}")
            
            # Información básica del disco
            try:
                result = subprocess.run(['lsblk', '-no', 'SIZE,MODEL,SERIAL', device], 
                                      capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    info = result.stdout.strip().split()
                    if len(info) >= 1:
                        print(f"    {Colors.OKGREEN}Tamaño:{Colors.ENDC} {info[0]}")
                    if len(info) >= 2:
                        print(f"    {Colors.OKGREEN}Modelo:{Colors.ENDC} {info[1]}")
                    if len(info) >= 3:
                        print(f"    {Colors.OKGREEN}Serie:{Colors.ENDC} {info[2]}")
            except:
                pass
            
            # Información SMART
            try:
                result = subprocess.run(['smartctl', '-i', device], 
                                      capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    smart_info = result.stdout
                    
                    # Extraer información clave
                    for line in smart_info.split('\n'):
                        if 'Device Model:' in line:
                            model = line.split(':', 1)[1].strip()
                            print(f"    {Colors.OKGREEN}Modelo SMART:{Colors.ENDC} {model}")
                        elif 'Serial Number:' in line:
                            serial = line.split(':', 1)[1].strip()
                            print(f"    {Colors.OKGREEN}Número de Serie:{Colors.ENDC} {serial}")
                        elif 'Firmware Version:' in line:
                            firmware = line.split(':', 1)[1].strip()
                            print(f"    {Colors.OKGREEN}Firmware:{Colors.ENDC} {firmware}")
                        elif 'User Capacity:' in line:
                            capacity = line.split(':', 1)[1].strip()
                            print(f"    {Colors.OKGREEN}Capacidad:{Colors.ENDC} {capacity}")
                        elif 'Rotation Rate:' in line:
                            rotation = line.split(':', 1)[1].strip()
                            if 'Solid State Device' in rotation:
                                print(f"    {Colors.OKGREEN}Tipo:{Colors.ENDC} SSD")
                            else:
                                print(f"    {Colors.OKGREEN}RPM:{Colors.ENDC} {rotation}")
                        elif 'Form Factor:' in line:
                            form_factor = line.split(':', 1)[1].strip()
                            print(f"    {Colors.OKGREEN}Factor de Forma:{Colors.ENDC} {form_factor}")
            except:
                pass
            
            # Temperatura SMART
            try:
                result = subprocess.run(['smartctl', '-A', device], 
                                      capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    smart_data = result.stdout
                    for line in smart_data.split('\n'):
                        if 'Temperature_Celsius' in line or 'Airflow_Temperature_Cel' in line:
                            parts = line.split()
                            if len(parts) >= 10:
                                temp = parts[9]
                                print(f"    {Colors.OKGREEN}Temperatura:{Colors.ENDC} {temp}°C")
                                break
                        elif 'Power_On_Hours' in line:
                            parts = line.split()
                            if len(parts) >= 10:
                                hours = int(parts[9])
                                days = hours // 24
                                print(f"    {Colors.OKGREEN}Horas de Uso:{Colors.ENDC} {hours} h ({days} días)")
            except:
                print(f"{Colors.WARNING}⚠ Error leyendo información SMART{Colors.ENDC}")
                pass
    
    def get_detailed_gpu_info(self):
        """Información detallada de GPU"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                     INFORMACIÓN DETALLADA DE GPU                 ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        num_gpus = self.safe_call(self.lib.sysmon_get_num_gpus, 0)
        print(f"  {Colors.OKCYAN}GPUs Detectadas: {num_gpus}{Colors.ENDC}")
        
        if num_gpus > 0:
            gpu_model = self.safe_call(self.lib.sysmon_get_gpu_model, "Unknown")
            gpu_vendor = self.safe_call(self.lib.sysmon_get_gpu_vendor, "Unknown")
            
            print(f"\n  {Colors.OKCYAN}GPU Principal:{Colors.ENDC}")
            print(f"    {Colors.OKGREEN}Modelo:{Colors.ENDC} {gpu_model}")
            print(f"    {Colors.OKGREEN}Fabricante:{Colors.ENDC} {gpu_vendor}")
        
        # Información adicional de GPU
        try:
            # NVIDIA
            result = subprocess.run(['nvidia-smi', '--query-gpu=name,driver_version,memory.total,memory.used,temperature.gpu,power.draw,clocks.gr,clocks.mem', '--format=csv,noheader,nounits'], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                gpu_info = result.stdout.strip().split(', ')
                if len(gpu_info) >= 8:
                    print(f"    {Colors.OKGREEN}Driver NVIDIA:{Colors.ENDC} {gpu_info[1]}")
                    print(f"    {Colors.OKGREEN}VRAM Total:{Colors.ENDC} {gpu_info[2]} MB")
                    print(f"    {Colors.OKGREEN}VRAM Usado:{Colors.ENDC} {gpu_info[3]} MB")
                    print(f"    {Colors.OKGREEN}Temperatura:{Colors.ENDC} {gpu_info[4]}°C")
                    print(f"    {Colors.OKGREEN}Consumo:{Colors.ENDC} {gpu_info[5]} W")
                    print(f"    {Colors.OKGREEN}Clock GPU:{Colors.ENDC} {gpu_info[6]} MHz")
                    print(f"    {Colors.OKGREEN}Clock Memoria:{Colors.ENDC} {gpu_info[7]} MHz")
        except:
            pass
        
        # AMD GPU info
        try:
            with open('/sys/class/drm/card0/device/vendor', 'r') as f:
                vendor_id = f.read().strip()
            if vendor_id == '0x1002':  # AMD
                print(f"    {Colors.OKGREEN}Vendor ID:{Colors.ENDC} AMD (0x1002)")
                
                # Intentar obtener más información de AMD
                try:
                    with open('/sys/class/drm/card0/device/device', 'r') as f:
                        device_id = f.read().strip()
                    print(f"    {Colors.OKGREEN}Device ID:{Colors.ENDC} {device_id}")
                except:
                    pass
        except:
            pass
    
    def get_detailed_sensors_info(self):
        """Información detallada de sensores"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                   INFORMACIÓN DETALLADA DE SENSORES              ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        num_sensors = self.safe_call(self.lib.sysmon_get_num_sensors, 0)
        print(f"  {Colors.OKCYAN}Sensores Detectados: {num_sensors}{Colors.ENDC}")
        
        # Leer sensores directamente
        try:
            result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                sensors_output = result.stdout
                print(f"\n  {Colors.OKCYAN}Salida de 'sensors':{Colors.ENDC}")
                for line in sensors_output.split('\n'):
                    if line.strip() and not line.startswith('Adapter:'):
                        if '°C' in line or 'RPM' in line or 'V' in line:
                            print(f"    {Colors.OKGREEN}{line.strip()}{Colors.ENDC}")
        except:
            pass
    
    def run_realtime_monitoring(self, duration=10):
        """Monitoreo en tiempo real detallado"""
        print(f"\n{Colors.HEADER}{Colors.BOLD}╔══════════════════════════════════════════════════════════════════╗{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}║                    MONITOREO EN TIEMPO REAL ({duration}s)                   ║{Colors.ENDC}")
        print(f"{Colors.HEADER}{Colors.BOLD}╚══════════════════════════════════════════════════════════════════╝{Colors.ENDC}")
        
        print(f"\n{Colors.OKCYAN}{'Tiempo':<8} {'CPU%':<8} {'Freq':<10} {'Temp':<8} {'RAM%':<8} {'GPU%':<8} {'GPU°C':<8}{Colors.ENDC}")
        print("=" * 70)
        
        start_time = time.time()
        
        while time.time() - start_time < duration:
            # Obtener métricas en tiempo real
            cpu_usage = self.safe_call(self.lib.sysmon_get_cpu_usage_percent, -1.0)
            cpu_freq = self.safe_call(self.lib.sysmon_get_cpu_frequency, -1.0)
            cpu_temp = self.safe_call(self.lib.sysmon_get_cpu_temperature, -1.0)
            ram_usage = self.safe_call(self.lib.sysmon_get_ram_usage_percent, -1.0)
            gpu_usage = self.safe_call(self.lib.sysmon_get_gpu_usage_percent, -1.0)
            gpu_temp = self.safe_call(self.lib.sysmon_get_gpu_temperature, -1.0)
            
            elapsed = time.time() - start_time
            
            cpu_temp_str = f"{cpu_temp:.1f}°C" if cpu_temp > 0 else "N/A"
            gpu_usage_str = f"{gpu_usage:.1f}%" if gpu_usage >= 0 else "N/A"
            gpu_temp_str = f"{gpu_temp:.1f}°C" if gpu_temp > 0 else "N/A"
            
            print(f"{elapsed:7.1f}s "
                  f"{cpu_usage:7.1f}% "
                  f"{cpu_freq:9.0f}MHz "
                  f"{cpu_temp_str:<8} "
                  f"{ram_usage:7.1f}% "
                  f"{gpu_usage_str:<8} "
                  f"{gpu_temp_str:<8}")
            
            time.sleep(1)
        
        print("=" * 70)
        print(f"{Colors.OKGREEN}✅ Monitoreo en tiempo real completado{Colors.ENDC}")

def main():
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'MONITOR DETALLADO DE HARDWARE - ESTILO CRYSTALDISKINFO':^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    
    monitor = DetailedHardwareMonitor()
    
    if not monitor.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca{Colors.ENDC}")
        return 1
    
    if not monitor.initialize():
        print(f"{Colors.FAIL}❌ No se pudo inicializar el sistema{Colors.ENDC}")
        return 1
    
    print(f"{Colors.OKGREEN}✅ Sistema de monitoreo detallado inicializado{Colors.ENDC}")
    
    # Obtener información detallada
    monitor.get_detailed_cpu_info()
    monitor.get_detailed_disk_info()
    monitor.get_detailed_gpu_info()
    monitor.get_detailed_sensors_info()
    
    # Preguntar por monitoreo en tiempo real
    print(f"\n{Colors.WARNING}¿Ejecutar monitoreo en tiempo real por 10 segundos? [s/N]: {Colors.ENDC}", end="")
    try:
        if input().strip().lower() == 's':
            monitor.run_realtime_monitoring(10)
    except KeyboardInterrupt:
        print(f"\n{Colors.WARNING}Monitoreo interrumpido{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🎉 Análisis detallado de hardware completado{Colors.ENDC}")
    
    monitor.cleanup()
    return 0

if __name__ == "__main__":
    sys.exit(main())