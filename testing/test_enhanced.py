#!/usr/bin/env python3
"""
Test Python Mejorado para SysMon
Simula la capa de interfaz que usará tu compañero para obtener información del hardware
"""

import ctypes
import time
import platform
import psutil
import sys
import os
from ctypes import POINTER, Structure, c_char, c_char_p, c_int, c_double

# Configurar LD_LIBRARY_PATH si es necesario
os.environ['LD_LIBRARY_PATH'] = '.'

# ==================== CONFIGURACIÓN Y CONSTANTES ====================
CPU_MODEL_LEN = 128
DISK_NAME_LEN = 128
DISK_DEVICE = b"/dev/sda"  # Disco principal a consultar

class Colors:
    """Colores ANSI para terminal"""
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

# ==================== ESTRUCTURAS C ====================
class CpuInfo(Structure):
    _fields_ = [
        ("vendor", c_char_p),
        ("model", c_char_p),
        ("cores", c_int),
        ("mhz", c_double)
    ]

class DiskInfo(Structure):
    _fields_ = [
        ("name", c_char * DISK_NAME_LEN),
        ("temperature", c_int),
        ("power_on_hours", c_int)
    ]

# ==================== CLASE PRINCIPAL SYSMON ====================
class SysMonInterface:
    """
    Interfaz principal para la biblioteca SysMon
    Esta es la clase que tu compañero usaría en su capa
    """
    
    def __init__(self, library_path=None):
        """Inicializar la interfaz con la biblioteca"""
        self.lib = None
        self.library_loaded = False
        
        # Detectar el path de la biblioteca según el SO
        if library_path is None:
            if platform.system() == "Windows":
                library_path = "./sysmon.dll"
            else:
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
        """Configurar tipos de datos y funciones de la biblioteca C"""
        if not self.lib:
            return
        
        # Configurar funciones de CPU
        self.lib.alloc_cpu_info.restype = POINTER(CpuInfo)
        self.lib.get_cpu_usage.restype = c_double
        self.lib.get_cpu_model.argtypes = [c_char_p, ctypes.c_size_t]
        self.lib.get_cpu_model.restype = c_int
        
        # Configurar funciones de disco (si existen)
        if hasattr(self.lib, 'alloc_disk_info'):
            self.lib.alloc_disk_info.restype = POINTER(DiskInfo)
        if hasattr(self.lib, 'get_disk_smart'):
            self.lib.get_disk_smart.argtypes = [c_char_p, POINTER(DiskInfo)]
            self.lib.get_disk_smart.restype = c_int
        if hasattr(self.lib, 'get_disk_smart_via_smartctl'):
            self.lib.get_disk_smart_via_smartctl.argtypes = [c_char_p, POINTER(DiskInfo)]
            self.lib.get_disk_smart_via_smartctl.restype = c_int
    
    def get_system_info(self):
        """Obtener información básica del sistema usando Python"""
        return {
            'platform': platform.system(),
            'platform_release': platform.release(),
            'platform_version': platform.version(),
            'machine': platform.machine(),
            'processor': platform.processor(),
            'python_version': platform.python_version(),
            'cpu_count_logical': psutil.cpu_count(logical=True),
            'cpu_count_physical': psutil.cpu_count(logical=False),
            'cpu_freq': psutil.cpu_freq(),
            'memory': psutil.virtual_memory(),
            'boot_time': psutil.boot_time(),
            'hostname': platform.node()
        }
    
    def get_cpu_info_c(self):
        """Obtener información del CPU usando la biblioteca C"""
        if not self.library_loaded:
            return None
        
        try:
            info_ptr = self.lib.alloc_cpu_info()
            if not info_ptr:
                return None
            
            info = info_ptr.contents
            result = {
                'vendor': info.vendor.decode('utf-8', errors='ignore') if info.vendor else 'Unknown',
                'model': info.model.decode('utf-8', errors='ignore') if info.model else 'Unknown',
                'cores': info.cores,
                'frequency_mhz': info.mhz,
                'source': 'C Library'
            }
            
            # Liberar memoria
            if hasattr(self.lib, 'free_cpu_info'):
                self.lib.free_cpu_info(info_ptr)
            
            return result
        except Exception as e:
            print(f"{Colors.WARNING}⚠ Error obteniendo info CPU desde C: {e}{Colors.ENDC}")
            return None
    
    def get_cpu_model_c(self):
        """Obtener modelo específico del CPU usando get_cpu_model"""
        if not self.library_loaded:
            return "Library not loaded"
        
        try:
            buffer = ctypes.create_string_buffer(CPU_MODEL_LEN)
            if self.lib.get_cpu_model(buffer, CPU_MODEL_LEN) == 0:
                return buffer.value.decode('utf-8', errors='ignore').strip()
            else:
                return "Error getting CPU model"
        except Exception as e:
            return f"Exception: {e}"
    
    def get_cpu_usage_c(self):
        """Obtener uso actual del CPU usando la biblioteca C"""
        if not self.library_loaded:
            return -1.0
        
        try:
            return self.lib.get_cpu_usage()
        except Exception as e:
            print(f"{Colors.WARNING}⚠ Error obteniendo uso CPU: {e}{Colors.ENDC}")
            return -1.0
    
    def get_disk_info_c(self, device_path="/dev/sda"):
        """Obtener información del disco usando la biblioteca C"""
        if not self.library_loaded or not hasattr(self.lib, 'alloc_disk_info'):
            return None
        
        try:
            device_bytes = device_path.encode('utf-8')
            disk_ptr = self.lib.alloc_disk_info()
            if not disk_ptr:
                return None
            
            # Intentar obtener datos SMART
            smart_result = -1
            if hasattr(self.lib, 'get_disk_smart'):
                smart_result = self.lib.get_disk_smart(device_bytes, disk_ptr)
            
            # Si falla, intentar método alternativo
            if smart_result != 0 and hasattr(self.lib, 'get_disk_smart_via_smartctl'):
                smart_result = self.lib.get_disk_smart_via_smartctl(device_bytes, disk_ptr)
            
            disk = disk_ptr.contents
            result = {
                'device': device_path,
                'name': disk.name.decode('utf-8', errors='ignore').strip(),
                'temperature': disk.temperature if disk.temperature > -1 else None,
                'power_on_hours': disk.power_on_hours if disk.power_on_hours > -1 else None,
                'smart_available': smart_result == 0,
                'source': 'C Library'
            }
            
            return result
        except Exception as e:
            print(f"{Colors.WARNING}⚠ Error obteniendo info disco: {e}{Colors.ENDC}")
            return None
    
    def get_comprehensive_hardware_info(self):
        """Obtener información completa del hardware (método principal)"""
        info = {}
        
        # Información del sistema
        info['system'] = self.get_system_info()
        
        # Información del CPU (C + Python)
        info['cpu'] = {
            'c_library': self.get_cpu_info_c(),
            'c_model': self.get_cpu_model_c(),
            'python': {
                'count_logical': psutil.cpu_count(logical=True),
                'count_physical': psutil.cpu_count(logical=False),
                'frequency': psutil.cpu_freq(),
                'percent': psutil.cpu_percent(interval=0.1),
                'load_avg': psutil.getloadavg() if hasattr(psutil, 'getloadavg') else None
            }
        }
        
        # Información de memoria
        memory = psutil.virtual_memory()
        info['memory'] = {
            'total_gb': memory.total / (1024**3),
            'available_gb': memory.available / (1024**3),
            'used_gb': memory.used / (1024**3),
            'percent': memory.percent,
            'free_gb': memory.free / (1024**3)
        }
        
        # Información de discos
        info['disks'] = []
        
        # Intentar varios dispositivos comunes
        disk_devices = ["/dev/sda", "/dev/nvme0n1", "/dev/sdb"]
        for device in disk_devices:
            if os.path.exists(device):
                disk_info = self.get_disk_info_c(device)
                if disk_info:
                    info['disks'].append(disk_info)
        
        # Información adicional de discos usando Python
        info['disk_usage'] = []
        for partition in psutil.disk_partitions():
            try:
                usage = psutil.disk_usage(partition.mountpoint)
                info['disk_usage'].append({
                    'device': partition.device,
                    'mountpoint': partition.mountpoint,
                    'fstype': partition.fstype,
                    'total_gb': usage.total / (1024**3),
                    'used_gb': usage.used / (1024**3),
                    'free_gb': usage.free / (1024**3),
                    'percent': (usage.used / usage.total) * 100
                })
            except PermissionError:
                continue
        
        # Información de red
        info['network'] = []
        net_io = psutil.net_io_counters(pernic=True)
        for interface, stats in net_io.items():
            info['network'].append({
                'interface': interface,
                'bytes_sent': stats.bytes_sent,
                'bytes_recv': stats.bytes_recv,
                'packets_sent': stats.packets_sent,
                'packets_recv': stats.packets_recv
            })
        
        return info

# ==================== FUNCIONES DE DISPLAY ====================
def print_header(title):
    """Imprimir encabezado con estilo"""
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'='*60}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{title:^60}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*60}{Colors.ENDC}")

def print_section(title):
    """Imprimir sección con estilo"""
    print(f"\n{Colors.OKCYAN}{Colors.BOLD}{title}{Colors.ENDC}")
    print(f"{Colors.OKCYAN}{'-'*len(title)}{Colors.ENDC}")

def display_system_info(info):
    """Mostrar información del sistema"""
    sys_info = info['system']
    print_section("INFORMACIÓN DEL SISTEMA")
    
    print(f"   {Colors.OKBLUE}Sistema Operativo:{Colors.ENDC} {sys_info['platform']} {sys_info['platform_release']}")
    print(f"   {Colors.OKBLUE}Arquitectura:{Colors.ENDC} {sys_info['machine']}")
    print(f"   {Colors.OKBLUE}Procesador:{Colors.ENDC} {sys_info['processor']}")
    print(f"   {Colors.OKBLUE}Hostname:{Colors.ENDC} {sys_info['hostname']}")
    print(f"   {Colors.OKBLUE}Python:{Colors.ENDC} {sys_info['python_version']}")
    
    # Tiempo de arranque
    boot_time = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(sys_info['boot_time']))
    uptime = time.time() - sys_info['boot_time']
    uptime_hours = int(uptime // 3600)
    print(f"   {Colors.OKBLUE}Boot Time:{Colors.ENDC} {boot_time}")
    print(f"   {Colors.OKBLUE}Uptime:{Colors.ENDC} {uptime_hours} horas")

def display_cpu_info(info):
    """Mostrar información del CPU"""
    cpu_info = info['cpu']
    print_section("INFORMACIÓN DEL CPU")
    
    # Información desde C
    c_info = cpu_info['c_library']
    if c_info:
        print(f"   {Colors.OKGREEN}[Biblioteca C]{Colors.ENDC}")
        print(f"     Vendor: {c_info['vendor']}")
        print(f"     Model: {c_info['model']}")
        print(f"     Cores: {c_info['cores']}")
        print(f"     Frequency: {c_info['frequency_mhz']:.0f} MHz")
    
    print(f"   {Colors.OKGREEN}[get_cpu_model()]{Colors.ENDC}")
    print(f"     Model: {cpu_info['c_model']}")
    
    # Información desde Python
    py_info = cpu_info['python']
    print(f"   {Colors.OKGREEN}[Python - psutil]{Colors.ENDC}")
    print(f"     Cores Logical: {py_info['count_logical']}")
    print(f"     Cores Physical: {py_info['count_physical']}")
    
    if py_info['frequency']:
        print(f"     Current Freq: {py_info['frequency'].current:.0f} MHz")
        print(f"     Min Freq: {py_info['frequency'].min:.0f} MHz")
        print(f"     Max Freq: {py_info['frequency'].max:.0f} MHz")
    
    print(f"     Usage: {py_info['percent']:.1f}%")
    
    if py_info['load_avg']:
        print(f"     Load Avg: {py_info['load_avg'][0]:.2f}, {py_info['load_avg'][1]:.2f}, {py_info['load_avg'][2]:.2f}")

def display_memory_info(info):
    """Mostrar información de memoria"""
    mem_info = info['memory']
    print_section("INFORMACIÓN DE MEMORIA")
    
    print(f"   {Colors.OKGREEN}Total:{Colors.ENDC} {mem_info['total_gb']:.1f} GB")
    print(f"   {Colors.OKGREEN}Usado:{Colors.ENDC} {mem_info['used_gb']:.1f} GB ({mem_info['percent']:.1f}%)")
    print(f"   {Colors.OKGREEN}Disponible:{Colors.ENDC} {mem_info['available_gb']:.1f} GB")
    print(f"   {Colors.OKGREEN}Libre:{Colors.ENDC} {mem_info['free_gb']:.1f} GB")

def display_disk_info(info):
    """Mostrar información de discos"""
    print_section("INFORMACIÓN DE DISCOS")
    
    # Discos desde biblioteca C (información SMART)
    if info['disks']:
        print(f"   {Colors.OKGREEN}[Biblioteca C - Información SMART]{Colors.ENDC}")
        for i, disk in enumerate(info['disks']):
            print(f"     Disco {i+1}: {disk['device']}")
            print(f"       Name: {disk['name']}")
            print(f"       SMART: {'✅ Disponible' if disk['smart_available'] else '❌ No disponible'}")
            
            if disk['temperature'] is not None:
                print(f"       Temperature: {disk['temperature']}°C")
            if disk['power_on_hours'] is not None:
                print(f"       Power On Hours: {disk['power_on_hours']} h")
    
    # Uso de discos desde Python
    if info['disk_usage']:
        print(f"   {Colors.OKGREEN}[Python - Uso de Particiones]{Colors.ENDC}")
        for partition in info['disk_usage'][:3]:  # Mostrar solo las primeras 3
            print(f"     {partition['device']} ({partition['fstype']}) en {partition['mountpoint']}")
            print(f"       Total: {partition['total_gb']:.1f} GB")
            print(f"       Usado: {partition['used_gb']:.1f} GB ({partition['percent']:.1f}%)")
            print(f"       Libre: {partition['free_gb']:.1f} GB")

def display_network_info(info):
    """Mostrar información de red"""
    print_section("INFORMACIÓN DE RED")
    
    # Mostrar solo interfaces principales
    main_interfaces = [net for net in info['network'] 
                      if not net['interface'].startswith('lo') and 
                      (net['bytes_sent'] > 0 or net['bytes_recv'] > 0)][:3]
    
    for net in main_interfaces:
        print(f"   {Colors.OKGREEN}{net['interface']}:{Colors.ENDC}")
        print(f"     Bytes enviados: {net['bytes_sent'] / (1024*1024):.1f} MB")
        print(f"     Bytes recibidos: {net['bytes_recv'] / (1024*1024):.1f} MB")
        print(f"     Paquetes enviados: {net['packets_sent']}")
        print(f"     Paquetes recibidos: {net['packets_recv']}")

def run_realtime_monitoring(sysmon, duration=10):
    """Ejecutar monitoreo en tiempo real"""
    print_section(f"MONITOREO EN TIEMPO REAL ({duration} segundos)")
    
    print(f"{'Tiempo':<8} {'CPU%(C)':<9} {'CPU%(Py)':<9} {'RAM%':<8} {'Freq(MHz)':<10}")
    print("=" * 55)
    
    start_time = time.time()
    iteration = 0
    
    while time.time() - start_time < duration:
        iteration += 1
        
        # Obtener métricas
        cpu_usage_c = sysmon.get_cpu_usage_c()
        cpu_usage_py = psutil.cpu_percent(interval=0.1)
        memory = psutil.virtual_memory()
        
        # Frecuencia actual
        cpu_freq = psutil.cpu_freq()
        freq_mhz = cpu_freq.current if cpu_freq else 0
        
        elapsed = time.time() - start_time
        
        print(f"{elapsed:7.1f}s "
              f"{cpu_usage_c:8.2f}% "
              f"{cpu_usage_py:8.2f}% "
              f"{memory.percent:7.1f}% "
              f"{freq_mhz:9.0f}")
        
        time.sleep(0.5)
    
    print("=" * 55)
    print(f"✅ Monitoreo completado - {iteration} iteraciones")

# ==================== FUNCIÓN PRINCIPAL ====================
def main():
    """Función principal del test"""
    print_header("SYSMON - TEST DE INFORMACIÓN DE HARDWARE")
    print(f"{Colors.OKGREEN}Simulando la interfaz que usará tu compañero{Colors.ENDC}")
    
    # Inicializar SysMon
    sysmon = SysMonInterface()
    
    if not sysmon.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca. Continuando solo con Python...{Colors.ENDC}")
    
    # Obtener información completa del hardware
    print(f"\n{Colors.OKCYAN}Recopilando información del hardware...{Colors.ENDC}")
    hardware_info = sysmon.get_comprehensive_hardware_info()
    
    # Mostrar información
    display_system_info(hardware_info)
    display_cpu_info(hardware_info)
    display_memory_info(hardware_info)
    display_disk_info(hardware_info)
    display_network_info(hardware_info)
    
    # Preguntar si ejecutar monitoreo en tiempo real
    print(f"\n{Colors.WARNING}¿Ejecutar monitoreo en tiempo real por 10 segundos? [s/N]: {Colors.ENDC}", end="")
    if input().strip().lower() == 's':
        run_realtime_monitoring(sysmon, 10)
    
    # Resumen final
    print_section("RESUMEN DEL TEST")
    print(f"   {Colors.OKGREEN}✅ Información del sistema: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Información del CPU: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Información de memoria: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Información de discos: {'OK' if hardware_info['disks'] else 'Parcial (sin SMART)'}{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Información de red: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Biblioteca C: {'Cargada' if sysmon.library_loaded else 'No disponible'}{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🎉 Test completado exitosamente{Colors.ENDC}")
if __name__ == "__main__":
    main()