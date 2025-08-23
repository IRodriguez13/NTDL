#!/usr/bin/env python3
"""
Wrapper de Python para la biblioteca sysmon
Proporciona una interfaz más simple y pythónica para acceder a las métricas del sistema.
"""

import ctypes
import os
import sys
import json
from typing import Dict, Any, Optional

class SystemStatus:
    """Clase que representa el estado del sistema"""
    
    def __init__(self, os_name: str, cpu_model: str, cpu_cores: int, 
                 cpu_usage: float, ram_total: int, ram_free: int, 
                 disk_model: str, disk_health: int):
        self.os_name = os_name
        self.cpu_model = cpu_model
        self.cpu_cores = cpu_cores
        self.cpu_usage = cpu_usage
        self.ram_total = ram_total
        self.ram_free = ram_free
        self.ram_used = ram_total - ram_free
        self.ram_usage_percent = (self.ram_used / ram_total * 100) if ram_total > 0 else 0
        self.disk_model = disk_model
        self.disk_health = disk_health
    
    def to_dict(self) -> Dict[str, Any]:
        """Convertir a diccionario"""
        return {
            'os_name': self.os_name,
            'cpu_model': self.cpu_model,
            'cpu_cores': self.cpu_cores,
            'cpu_usage_percent': self.cpu_usage,
            'ram_total_kb': self.ram_total,
            'ram_free_kb': self.ram_free,
            'ram_used_kb': self.ram_used,
            'ram_usage_percent': self.ram_usage_percent,
            'disk_model': self.disk_model,
            'disk_health': self.disk_health
        }
    
    def to_json(self) -> str:
        """Convertir a JSON"""
        return json.dumps(self.to_dict(), indent=2)
    
    def __str__(self) -> str:
        return f"SystemStatus(os='{self.os_name}', cpu='{self.cpu_model}', cores={self.cpu_cores}, cpu_usage={self.cpu_usage:.1f}%, ram_usage={self.ram_usage_percent:.1f}%)"

class SysMon:
    """Clase principal para acceder a las métricas del sistema"""
    
    def __init__(self, lib_path: Optional[str] = None):
        """
        Inicializar SysMon
        
        Args:
            lib_path: Ruta a la biblioteca compartida. Si es None, se detecta automáticamente.
        """
        if lib_path is None:
            lib_path = self._detect_library_path()
        
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"No se encontró la biblioteca: {lib_path}")
        
        self._load_library(lib_path)
        self._initialized = False
    
    def _detect_library_path(self) -> str:
        """Detectar la ruta de la biblioteca según la plataforma"""
        if sys.platform.startswith('linux'):
            return "./libsysmon.so"
        elif sys.platform.startswith('darwin'):
            return "./libsysmon.dylib"
        elif sys.platform.startswith('win'):
            return "./sysmon.dll"
        else:
            raise OSError(f"Plataforma no soportada: {sys.platform}")
    
    def _load_library(self, lib_path: str):
        """Cargar la biblioteca compartida"""
        try:
            self.lib = ctypes.CDLL(lib_path)
            self._setup_function_signatures()
        except Exception as e:
            raise RuntimeError(f"Error al cargar la biblioteca: {e}")
    
    def _setup_function_signatures(self):
        """Configurar las firmas de las funciones"""
        # Estructura C
        class CSystemStatus(ctypes.Structure):
            _fields_ = [
                ("os_name", ctypes.c_char * 32),
                ("cpu_model", ctypes.c_char * 128),
                ("cpu_cores", ctypes.c_int),
                ("cpu_usage_percent", ctypes.c_float),
                ("ram_total_kb", ctypes.c_ulong),
                ("ram_free_kb", ctypes.c_ulong),
                ("disk_model", ctypes.c_char * 128),
                ("disk_health", ctypes.c_int)
            ]
        
        self.CSystemStatus = CSystemStatus
        
        # Configurar funciones
        self.lib.sysmon_init.restype = ctypes.c_int
        self.lib.sysmon_get_metrics.restype = ctypes.c_int
        self.lib.sysmon_get_metrics.argtypes = [ctypes.POINTER(CSystemStatus)]
        self.lib.sysmon_get_cpu_usage.restype = ctypes.c_float
        self.lib.sysmon_get_cpu_cores.restype = ctypes.c_int
        self.lib.sysmon_get_cpu_model.restype = ctypes.c_char_p
        self.lib.sysmon_get_ram_total.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_free.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_used.restype = ctypes.c_ulong
        self.lib.sysmon_get_ram_usage_percent.restype = ctypes.c_float
        self.lib.sysmon_get_os_name.restype = ctypes.c_char_p
        self.lib.sysmon_get_disk_model.restype = ctypes.c_char_p
        self.lib.sysmon_get_disk_health.restype = ctypes.c_int

    
    def init(self):
        """Inicializar el sistema de monitoreo"""
        if not self._initialized:
            if self.lib.sysmon_init() != 0:
                raise RuntimeError("Error al inicializar sysmon")
            self._initialized = True
    
    def cleanup(self):
        """Limpiar recursos"""
        if self._initialized:
            self.lib.sysmon_cleanup()
            self._initialized = False
    
    def get_metrics(self) -> SystemStatus:
        """Obtener todas las métricas del sistema"""
        self.init()
        
        status = self.CSystemStatus()
        if self.lib.sysmon_get_metrics(ctypes.byref(status)) != 0:
            raise RuntimeError("Error al obtener métricas")
        
        return SystemStatus(
            os_name=status.os_name.decode('utf-8'),
            cpu_model=status.cpu_model.decode('utf-8'),
            cpu_cores=status.cpu_cores,
            cpu_usage=status.cpu_usage_percent,
            ram_total=status.ram_total_kb,
            ram_free=status.ram_free_kb,
            disk_model=status.disk_model.decode('utf-8'),
            disk_health=status.disk_health
        )
    
    def get_cpu_usage(self) -> float:
        """Obtener porcentaje de uso de CPU"""
        self.init()
        return self.lib.sysmon_get_cpu_usage()
    
    def get_cpu_cores(self) -> int:
        """Obtener número de núcleos CPU"""
        self.init()
        return self.lib.sysmon_get_cpu_cores()
    
    def get_cpu_model(self) -> str:
        """Obtener modelo de CPU"""
        self.init()
        return self.lib.sysmon_get_cpu_model().decode('utf-8')
    
    def get_ram_total(self) -> int:
        """Obtener RAM total en KB"""
        self.init()
        return self.lib.sysmon_get_ram_total()
    
    def get_ram_free(self) -> int:
        """Obtener RAM libre en KB"""
        self.init()
        return self.lib.sysmon_get_ram_free()
    
    def get_ram_used(self) -> int:
        """Obtener RAM usada en KB"""
        self.init()
        return self.lib.sysmon_get_ram_used()
    
    def get_ram_usage_percent(self) -> float:
        """Obtener porcentaje de uso de RAM"""
        self.init()
        return self.lib.sysmon_get_ram_usage_percent()
    
    def get_os_name(self) -> str:
        """Obtener nombre del sistema operativo"""
        self.init()
        return self.lib.sysmon_get_os_name().decode('utf-8')
    
    def get_disk_model(self) -> str:
        """Obtener modelo del disco"""
        self.init()
        return self.lib.sysmon_get_disk_model().decode('utf-8')
    
    def get_disk_health(self) -> int:
        """Obtener salud del disco (%)"""
        self.init()
        return self.lib.sysmon_get_disk_health()
    

    
    def __enter__(self):
        """Context manager entry"""
        self.init()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.cleanup()

# Función de conveniencia para obtener métricas rápidamente
def get_system_metrics() -> SystemStatus:
    """Obtener métricas del sistema de forma rápida"""
    with SysMon() as sysmon:
        return sysmon.get_metrics()

# Función de conveniencia para obtener métricas como diccionario
def get_system_metrics_dict() -> Dict[str, Any]:
    """Obtener métricas del sistema como diccionario"""
    return get_system_metrics().to_dict()


