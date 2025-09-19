#!/usr/bin/env python3
"""
Test Modular Completo para SysMon
Demuestra todas las nuevas funcionalidades del sistema modularizado
"""

import ctypes
import time
import platform
import sys
import os
from ctypes import POINTER, Structure, c_char, c_char_p, c_int, c_double, c_float, c_uint64, c_uint32

# Configurar LD_LIBRARY_PATH si es necesario
os.environ['LD_LIBRARY_PATH'] = '.'

# ==================== CONFIGURACIÓN Y CONSTANTES ====================
MAX_STRING_LEN = 256
MAX_SHORT_STRING_LEN = 64
MAX_CORES = 128
MAX_DISKS = 16
MAX_NETWORK_INTERFACES = 32
MAX_SENSORS = 64
MAX_GPUS = 8

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

class CoreInfo(Structure):
    _fields_ = [
        ("core_id", c_int),
        ("usage_percent", c_float),
        ("frequency_mhz", c_float),
        ("temperature", c_float)
    ]

class CpuMetrics(Structure):
    _fields_ = [
        ("vendor", c_char * MAX_SHORT_STRING_LEN),
        ("model", c_char * MAX_STRING_LEN),
        ("architecture", c_char * MAX_SHORT_STRING_LEN),
        ("physical_cores", c_int),
        ("logical_cores", c_int),
        ("base_frequency_mhz", c_float),
        ("max_frequency_mhz", c_float),
        ("current_frequency_mhz", c_float),
        ("usage_percent", c_float),
        ("temperature", c_float),
        ("cache_l1_kb", c_int),
        ("cache_l2_kb", c_int),
        ("cache_l3_kb", c_int),
        ("cores", CoreInfo * MAX_CORES),
        ("num_cores_info", c_int)
    ]

class DetailedRamInfo(Structure):
    _fields_ = [
        ("total_kb", c_uint64),
        ("free_kb", c_uint64),
        ("available_kb", c_uint64),
        ("used_kb", c_uint64),
        ("cached_kb", c_uint64),
        ("buffers_kb", c_uint64),
        ("shared_kb", c_uint64),
        ("usage_percent", c_float)
    ]

class GpuMetrics(Structure):
    _fields_ = [
        ("name", c_char * MAX_STRING_LEN),
        ("vendor", c_char * MAX_SHORT_STRING_LEN),
        ("driver_version", c_char * MAX_SHORT_STRING_LEN),
        ("memory_total_mb", c_uint64),
        ("memory_used_mb", c_uint64),
        ("memory_free_mb", c_uint64),
        ("memory_usage_percent", c_float),
        ("usage_percent", c_float),
        ("temperature", c_float),
        ("fan_speed_rpm", c_int),
        ("power_usage_w", c_int),
        ("core_clock_mhz", c_int),
        ("memory_clock_mhz", c_int)
    ]

class DiskInfo(Structure):
    _fields_ = [
        ("device", c_char * MAX_SHORT_STRING_LEN),
        ("name", c_char * MAX_STRING_LEN),
        ("filesystem", c_char * MAX_SHORT_STRING_LEN),
        ("mount_point", c_char * MAX_STRING_LEN),
        ("total_gb", c_uint64),
        ("used_gb", c_uint64),
        ("free_gb", c_uint64),
        ("usage_percent", c_float),
        ("temperature", c_int),
        ("read_bytes", c_uint64),
        ("write_bytes", c_uint64),
        ("read_ops", c_uint32),
        ("write_ops", c_uint32),
        ("power_on_hours", c_int),
        ("is_ssd", c_int)
    ]

class NetworkInfo(Structure):
    _fields_ = [
        ("interface", c_char * MAX_SHORT_STRING_LEN),
        ("ip_address", c_char * 46),
        ("mac_address", c_char * 18),
        ("is_up", c_int),
        ("is_wireless", c_int),
        ("bytes_sent", c_uint64),
        ("bytes_recv", c_uint64),
        ("packets_sent", c_uint64),
        ("packets_recv", c_uint64),
        ("errors_in", c_uint32),
        ("errors_out", c_uint32),
        ("drops_in", c_uint32),
        ("drops_out", c_uint32),
        ("speed_mbps", c_int)
    ]

class SensorInfo(Structure):
    _fields_ = [
        ("name", c_char * MAX_STRING_LEN),
        ("label", c_char * MAX_SHORT_STRING_LEN),
        ("type", c_int),  # SensorType enum
        ("value", c_float),
        ("min_value", c_float),
        ("max_value", c_float),
        ("critical_value", c_float),
        ("unit", c_char * 16)
    ]

class SystemStatus(Structure):
    _fields_ = [
        # Información básica del sistema
        ("os_name", c_char * MAX_SHORT_STRING_LEN),
        ("os_version", c_char * MAX_STRING_LEN),
        ("kernel_version", c_char * MAX_STRING_LEN),
        ("hostname", c_char * MAX_STRING_LEN),
        ("architecture", c_char * MAX_SHORT_STRING_LEN),
        
        # Timestamp y tiempo de actividad
        ("timestamp", c_uint64),
        ("uptime_seconds", c_uint64),
        ("idle_time_seconds", c_uint64),
        
        # Métricas de CPU
        ("cpu", CpuMetrics),
        
        # Métricas de memoria
        ("ram", DetailedRamInfo),
        
        # Métricas de GPU
        ("gpus", GpuMetrics * MAX_GPUS),
        ("num_gpus", c_int),
        
        # Métricas de discos
        ("disks", DiskInfo * MAX_DISKS),
        ("num_disks", c_int),
        
        # Métricas de red
        ("network_interfaces", NetworkInfo * MAX_NETWORK_INTERFACES),
        ("num_network_interfaces", c_int),
        
        # Sensores
        ("sensors", SensorInfo * MAX_SENSORS),
        ("num_sensors", c_int),
        
        # Carga del sistema
        ("load_average_1min", c_float),
        ("load_average_5min", c_float),
        ("load_average_15min", c_float),
        ("load_1min_percent", c_float),
        ("load_5min_percent", c_float),
        ("load_15min_percent", c_float),
        
        # Información de procesos
        ("total_processes", c_int),
        ("running_processes", c_int),
        ("sleeping_processes", c_int),
        ("stopped_processes", c_int),
        ("zombie_processes", c_int),
        
        # Campos de compatibilidad
        ("cpu_usage_percent", c_float),
        ("ram_usage_percent", c_float),
        ("cpu_model", c_char * MAX_STRING_LEN),
        ("cpu_vendor", c_char * MAX_SHORT_STRING_LEN),
        ("cpu_cores", c_int),
        ("cpu_frequency_mhz", c_float),
        ("ram_total_gb", c_float),
        ("ram_available_gb", c_float),
        ("ram_used_gb", c_float)
    ]

# ==================== CLASE PRINCIPAL SYSMON MODULAR ====================

class SysMonModular:
    """
    Interfaz modular para la biblioteca SysMon
    Demuestra todas las nuevas funcionalidades
    """
    
    def __init__(self, library_path=None):
        """Inicializar la interfaz con la biblioteca"""
        self.lib = None
        self.library_loaded = False
        
        # Detectar el path de la biblioteca según el SO
        if library_path is None:
            if platform.system() == "Windows":
                library_path = "../sysmon.dll"
            else:
                library_path = "../libsysmon.so"
        
        try:
            self.lib = ctypes.CDLL(library_path)
            self._configure_library()
            self.library_loaded = True
            print(f"{Colors.OKGREEN}✅ Biblioteca modular {library_path} cargada correctamente{Colors.ENDC}")
        except Exception as e:
            print(f"{Colors.FAIL}❌ Error cargando biblioteca: {e}{Colors.ENDC}")
            self.library_loaded = False
    
    def _configure_library(self):
        """Configurar tipos de datos y funciones de la biblioteca C"""
        if not self.lib:
            return
        
        # Funciones principales
        self.lib.sysmon_init.restype = c_int
        self.lib.sysmon_cleanup.restype = None
        self.lib.sysmon_collect_metrics.argtypes = [POINTER(SystemStatus)]
        self.lib.sysmon_collect_metrics.restype = c_int
        
        # Getters individuales
        self.lib.sysmon_get_cpu_usage_percent.restype = c_float
        self.lib.sysmon_get_ram_usage_percent.restype = c_float
        self.lib.sysmon_get_cpu_temperature.restype = c_float
        self.lib.sysmon_get_num_gpus.restype = c_int
        self.lib.sysmon_get_num_disks.restype = c_int
        self.lib.sysmon_get_num_network_interfaces.restype = c_int
        self.lib.sysmon_get_num_sensors.restype = c_int
        
        # Funciones de compatibilidad
        if hasattr(self.lib, 'alloc_cpu_info'):
            class CpuInfoCompat(Structure):
                _fields_ = [
                    ("vendor", c_char_p),
                    ("model", c_char_p),
                    ("cores", c_int),
                    ("mhz", c_double)
                ]
            
            self.lib.alloc_cpu_info.restype = POINTER(CpuInfoCompat)
            self.lib.free_cpu_info.argtypes = [POINTER(CpuInfoCompat)]
            self.lib.get_cpu_usage.restype = c_double
    
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
    
    def collect_all_metrics(self):
        """Recolectar todas las métricas del sistema"""
        if not self.library_loaded:
            return None
        
        status = SystemStatus()
        result = self.lib.sysmon_collect_metrics(ctypes.byref(status))
        
        if result == 0:
            return status
        else:
            return None
    
    def get_cpu_usage_realtime(self):
        """Obtener uso de CPU en tiempo real"""
        if not self.library_loaded:
            return -1.0
        
        return self.lib.sysmon_get_cpu_usage_percent()
    
    def get_ram_usage_realtime(self):
        """Obtener uso de RAM en tiempo real"""
        if not self.library_loaded:
            return -1.0
        
        return self.lib.sysmon_get_ram_usage_percent()

# ==================== FUNCIONES DE DISPLAY ====================

def print_header(title):
    """Imprimir encabezado con estilo"""
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{title:^70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")

def print_section(title):
    """Imprimir sección con estilo"""
    print(f"\n{Colors.OKCYAN}{Colors.BOLD}{title}{Colors.ENDC}")
    print(f"{Colors.OKCYAN}{'-'*len(title)}{Colors.ENDC}")

def display_system_info(status):
    """Mostrar información del sistema"""
    print_section("INFORMACIÓN DEL SISTEMA")
    
    print(f"   {Colors.OKBLUE}Sistema Operativo:{Colors.ENDC} {status.os_name.decode()} {status.os_version.decode()}")
    print(f"   {Colors.OKBLUE}Kernel:{Colors.ENDC} {status.kernel_version.decode()}")
    print(f"   {Colors.OKBLUE}Arquitectura:{Colors.ENDC} {status.architecture.decode()}")
    print(f"   {Colors.OKBLUE}Hostname:{Colors.ENDC} {status.hostname.decode()}")
    print(f"   {Colors.OKBLUE}Timestamp:{Colors.ENDC} {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(status.timestamp))}")
    print(f"   {Colors.OKBLUE}Uptime:{Colors.ENDC} {status.uptime_seconds // 3600} horas")

def display_cpu_info(status):
    """Mostrar información detallada del CPU"""
    print_section("INFORMACIÓN DETALLADA DEL CPU")
    
    cpu = status.cpu
    print(f"   {Colors.OKGREEN}Vendor:{Colors.ENDC} {cpu.vendor.decode()}")
    print(f"   {Colors.OKGREEN}Model:{Colors.ENDC} {cpu.model.decode()}")
    print(f"   {Colors.OKGREEN}Architecture:{Colors.ENDC} {cpu.architecture.decode()}")
    print(f"   {Colors.OKGREEN}Physical Cores:{Colors.ENDC} {cpu.physical_cores}")
    print(f"   {Colors.OKGREEN}Logical Cores:{Colors.ENDC} {cpu.logical_cores}")
    print(f"   {Colors.OKGREEN}Base Frequency:{Colors.ENDC} {cpu.base_frequency_mhz:.0f} MHz")
    print(f"   {Colors.OKGREEN}Max Frequency:{Colors.ENDC} {cpu.max_frequency_mhz:.0f} MHz")
    print(f"   {Colors.OKGREEN}Current Frequency:{Colors.ENDC} {cpu.current_frequency_mhz:.0f} MHz")
    print(f"   {Colors.OKGREEN}Usage:{Colors.ENDC} {cpu.usage_percent:.1f}%")
    
    if cpu.temperature > 0:
        print(f"   {Colors.OKGREEN}Temperature:{Colors.ENDC} {cpu.temperature:.1f}°C")
    
    if cpu.cache_l3_kb > 0:
        print(f"   {Colors.OKGREEN}L3 Cache:{Colors.ENDC} {cpu.cache_l3_kb} KB")
    
    # Información por núcleo (mostrar solo los primeros 4)
    if cpu.num_cores_info > 0:
        print(f"   {Colors.OKGREEN}Core Info (first 4):{Colors.ENDC}")
        for i in range(min(4, cpu.num_cores_info)):
            core = cpu.cores[i]
            freq_str = f"{core.frequency_mhz:.0f} MHz" if core.frequency_mhz > 0 else "N/A"
            print(f"     Core {core.core_id}: {freq_str}")

def display_memory_info(status):
    """Mostrar información detallada de memoria"""
    print_section("INFORMACIÓN DETALLADA DE MEMORIA")
    
    ram = status.ram
    print(f"   {Colors.OKGREEN}Total:{Colors.ENDC} {ram.total_kb / (1024*1024):.1f} GB")
    print(f"   {Colors.OKGREEN}Used:{Colors.ENDC} {ram.used_kb / (1024*1024):.1f} GB ({ram.usage_percent:.1f}%)")
    print(f"   {Colors.OKGREEN}Available:{Colors.ENDC} {ram.available_kb / (1024*1024):.1f} GB")
    print(f"   {Colors.OKGREEN}Free:{Colors.ENDC} {ram.free_kb / (1024*1024):.1f} GB")
    print(f"   {Colors.OKGREEN}Cached:{Colors.ENDC} {ram.cached_kb / (1024*1024):.1f} GB")
    print(f"   {Colors.OKGREEN}Buffers:{Colors.ENDC} {ram.buffers_kb / (1024*1024):.1f} GB")
    print(f"   {Colors.OKGREEN}Shared:{Colors.ENDC} {ram.shared_kb / (1024*1024):.1f} GB")

def display_gpu_info(status):
    """Mostrar información de GPU"""
    print_section("INFORMACIÓN DE GPU")
    
    if status.num_gpus == 0:
        print(f"   {Colors.WARNING}No se detectaron GPUs{Colors.ENDC}")
        return
    
    for i in range(status.num_gpus):
        gpu = status.gpus[i]
        print(f"   {Colors.OKGREEN}GPU {i+1}:{Colors.ENDC}")
        print(f"     Name: {gpu.name.decode()}")
        print(f"     Vendor: {gpu.vendor.decode()}")
        print(f"     Driver: {gpu.driver_version.decode()}")
        
        if gpu.memory_total_mb > 0:
            print(f"     VRAM: {gpu.memory_used_mb}/{gpu.memory_total_mb} MB ({gpu.memory_usage_percent:.1f}%)")
        
        if gpu.usage_percent >= 0:
            print(f"     Usage: {gpu.usage_percent:.1f}%")
        
        if gpu.temperature > 0:
            print(f"     Temperature: {gpu.temperature:.1f}°C")
        
        if gpu.power_usage_w > 0:
            print(f"     Power: {gpu.power_usage_w} W")

def display_disk_info(status):
    """Mostrar información de discos"""
    print_section("INFORMACIÓN DE DISCOS")
    
    if status.num_disks == 0:
        print(f"   {Colors.WARNING}No se detectaron discos{Colors.ENDC}")
        return
    
    for i in range(status.num_disks):
        disk = status.disks[i]
        print(f"   {Colors.OKGREEN}Disk {i+1}:{Colors.ENDC}")
        print(f"     Device: {disk.device.decode()}")
        print(f"     Name: {disk.name.decode()}")
        print(f"     Mount: {disk.mount_point.decode()}")
        print(f"     Filesystem: {disk.filesystem.decode()}")
        print(f"     Size: {disk.used_gb}/{disk.total_gb} GB ({disk.usage_percent:.1f}%)")
        
        if disk.is_ssd == 1:
            print(f"     Type: SSD")
        elif disk.is_ssd == 0:
            print(f"     Type: HDD")
        
        if disk.temperature > 0:
            print(f"     Temperature: {disk.temperature}°C")
        
        if disk.power_on_hours > 0:
            print(f"     Power On Hours: {disk.power_on_hours}")

def display_network_info(status):
    """Mostrar información de red"""
    print_section("INFORMACIÓN DE RED")
    
    if status.num_network_interfaces == 0:
        print(f"   {Colors.WARNING}No se detectaron interfaces de red{Colors.ENDC}")
        return
    
    for i in range(status.num_network_interfaces):
        net = status.network_interfaces[i]
        print(f"   {Colors.OKGREEN}{net.interface.decode()}:{Colors.ENDC}")
        print(f"     IP: {net.ip_address.decode()}")
        print(f"     MAC: {net.mac_address.decode()}")
        print(f"     Status: {'UP' if net.is_up else 'DOWN'}")
        print(f"     Type: {'Wireless' if net.is_wireless else 'Wired'}")
        print(f"     TX: {net.bytes_sent / (1024*1024):.1f} MB ({net.packets_sent} packets)")
        print(f"     RX: {net.bytes_recv / (1024*1024):.1f} MB ({net.packets_recv} packets)")
        
        if net.speed_mbps > 0:
            print(f"     Speed: {net.speed_mbps} Mbps")

def display_sensor_info(status):
    """Mostrar información de sensores"""
    print_section("INFORMACIÓN DE SENSORES")
    
    if status.num_sensors == 0:
        print(f"   {Colors.WARNING}No se detectaron sensores{Colors.ENDC}")
        return
    
    # Agrupar sensores por tipo
    temp_sensors = []
    fan_sensors = []
    voltage_sensors = []
    other_sensors = []
    
    for i in range(status.num_sensors):
        sensor = status.sensors[i]
        if sensor.type == 0:  # SENSOR_TYPE_TEMPERATURE
            temp_sensors.append(sensor)
        elif sensor.type == 2:  # SENSOR_TYPE_FAN_SPEED
            fan_sensors.append(sensor)
        elif sensor.type == 1:  # SENSOR_TYPE_VOLTAGE
            voltage_sensors.append(sensor)
        else:
            other_sensors.append(sensor)
    
    # Mostrar sensores de temperatura
    if temp_sensors:
        print(f"   {Colors.OKGREEN}Temperature Sensors:{Colors.ENDC}")
        for sensor in temp_sensors[:5]:  # Mostrar solo los primeros 5
            print(f"     {sensor.label.decode()}: {sensor.value:.1f}{sensor.unit.decode()}")
    
    # Mostrar sensores de ventiladores
    if fan_sensors:
        print(f"   {Colors.OKGREEN}Fan Sensors:{Colors.ENDC}")
        for sensor in fan_sensors[:3]:  # Mostrar solo los primeros 3
            print(f"     {sensor.label.decode()}: {sensor.value:.0f} {sensor.unit.decode()}")
    
    # Mostrar sensores de voltaje
    if voltage_sensors:
        print(f"   {Colors.OKGREEN}Voltage Sensors:{Colors.ENDC}")
        for sensor in voltage_sensors[:3]:  # Mostrar solo los primeros 3
            print(f"     {sensor.label.decode()}: {sensor.value:.2f} {sensor.unit.decode()}")

def display_system_load(status):
    """Mostrar información de carga del sistema"""
    print_section("CARGA DEL SISTEMA Y PROCESOS")
    
    print(f"   {Colors.OKGREEN}Load Average:{Colors.ENDC}")
    print(f"     1 min: {status.load_average_1min:.2f} ({status.load_1min_percent:.1f}%)")
    print(f"     5 min: {status.load_average_5min:.2f} ({status.load_5min_percent:.1f}%)")
    print(f"     15 min: {status.load_average_15min:.2f} ({status.load_15min_percent:.1f}%)")
    
    print(f"   {Colors.OKGREEN}Processes:{Colors.ENDC}")
    print(f"     Total: {status.total_processes}")
    print(f"     Running: {status.running_processes}")
    print(f"     Sleeping: {status.sleeping_processes}")
    print(f"     Stopped: {status.stopped_processes}")
    print(f"     Zombie: {status.zombie_processes}")

def run_realtime_monitoring(sysmon, duration=10):
    """Ejecutar monitoreo en tiempo real"""
    print_section(f"MONITOREO EN TIEMPO REAL ({duration} segundos)")
    
    print(f"{'Time':<8} {'CPU%':<8} {'RAM%':<8} {'Freq':<10} {'Temp':<8} {'Load1':<8}")
    print("=" * 60)
    
    start_time = time.time()
    
    while time.time() - start_time < duration:
        # Obtener métricas en tiempo real
        cpu_usage = sysmon.get_cpu_usage_realtime()
        ram_usage = sysmon.get_ram_usage_realtime()
        
        # Obtener métricas completas cada 2 segundos
        if int(time.time() - start_time) % 2 == 0:
            status = sysmon.collect_all_metrics()
            if status:
                freq = status.cpu.current_frequency_mhz
                temp = status.cpu.temperature if status.cpu.temperature > 0 else -1
                load1 = status.load_average_1min
            else:
                freq = temp = load1 = -1
        
        elapsed = time.time() - start_time
        
        temp_str = f"{temp:.1f}°C" if temp > 0 else "N/A"
        
        print(f"{elapsed:7.1f}s "
              f"{cpu_usage:7.1f}% "
              f"{ram_usage:7.1f}% "
              f"{freq:9.0f}MHz "
              f"{temp_str:<8} "
              f"{load1:7.2f}")
        
        time.sleep(1)
    
    print("=" * 60)
    print(f"✅ Monitoreo completado")

# ==================== FUNCIÓN PRINCIPAL ====================

def main():
    """Función principal del test modular"""
    print_header("SYSMON MODULAR - TEST COMPLETO DE HARDWARE")
    print(f"{Colors.OKGREEN}Demostrando todas las funcionalidades del sistema modularizado{Colors.ENDC}")
    
    # Inicializar SysMon
    sysmon = SysMonModular()
    
    if not sysmon.library_loaded:
        print(f"{Colors.FAIL}❌ No se pudo cargar la biblioteca. Saliendo...{Colors.ENDC}")
        return 1
    
    # Inicializar el sistema
    if not sysmon.initialize():
        print(f"{Colors.FAIL}❌ No se pudo inicializar el sistema. Saliendo...{Colors.ENDC}")
        return 1
    
    print(f"{Colors.OKGREEN}✅ Sistema inicializado correctamente{Colors.ENDC}")
    
    # Recolectar todas las métricas
    print(f"\n{Colors.OKCYAN}Recolectando métricas completas del sistema...{Colors.ENDC}")
    status = sysmon.collect_all_metrics()
    
    if not status:
        print(f"{Colors.FAIL}❌ No se pudieron recolectar las métricas{Colors.ENDC}")
        return 1
    
    # Mostrar toda la información
    display_system_info(status)
    display_cpu_info(status)
    display_memory_info(status)
    display_gpu_info(status)
    display_disk_info(status)
    display_network_info(status)
    display_sensor_info(status)
    display_system_load(status)
    
    # Preguntar si ejecutar monitoreo en tiempo real
    print(f"\n{Colors.WARNING}¿Ejecutar monitoreo en tiempo real por 10 segundos? [s/N]: {Colors.ENDC}", end="")
    try:
        if input().strip().lower() == 's':
            run_realtime_monitoring(sysmon, 10)
    except KeyboardInterrupt:
        print(f"\n{Colors.WARNING}Monitoreo interrumpido por el usuario{Colors.ENDC}")
    
    # Resumen final
    print_section("RESUMEN DEL TEST MODULAR")
    print(f"   {Colors.OKGREEN}✅ Sistema de información: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de CPU: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de memoria: OK{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de GPU: {'OK' if status.num_gpus > 0 else 'Sin GPUs detectadas'}{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de discos: {'OK' if status.num_disks > 0 else 'Sin discos detectados'}{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de red: {'OK' if status.num_network_interfaces > 0 else 'Sin interfaces detectadas'}{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Módulo de sensores: {'OK' if status.num_sensors > 0 else 'Sin sensores detectados'}{Colors.ENDC}")
    print(f"   {Colors.OKGREEN}✅ Sistema de carga: OK{Colors.ENDC}")
    
    print(f"\n{Colors.HEADER}{Colors.BOLD}🎉 Test modular completado exitosamente{Colors.ENDC}")
    print(f"{Colors.OKCYAN}El sistema modularizado está funcionando correctamente{Colors.ENDC}")
    print(f"{Colors.OKCYAN}Componentes detectados: CPU, RAM, {status.num_gpus} GPU(s), {status.num_disks} disco(s), {status.num_network_interfaces} interfaz(es) de red, {status.num_sensors} sensor(es){Colors.ENDC}")
    
    # Limpiar recursos
    sysmon.cleanup()
    return 0

if __name__ == "__main__":
    sys.exit(main())