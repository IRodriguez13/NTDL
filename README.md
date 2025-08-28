# TNATDL - This is Not a To Do List

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/your-repo/sysmon)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-blue)](https://github.com/your-repo/sysmon)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

TNATDL es una aplicación de escritorio basada en una librería multiplataforma para obtener métricas detalladas del sistema operativo, incluyendo información de CPU, RAM, discos, red, GPU, sensores y más en lenguaje C. Diseñada para ser consumida desde Python mediante FFI (ctypes).

## 🚀 Características

- **Multiplataforma**: Soporte completo para Linux, Windows y macOS (XNU)
- **API Simple**: Interfaz C clara y bien documentada
- **Python FFI**: Integración nativa con Python usando ctypes
- **Métricas Detalladas**: CPU, RAM, discos, red, GPU, sensores, motherboard
- **Tiempo Real**: Monitoreo continuo con actualizaciones en tiempo real
- **Robusto**: Validaciones de punteros y manejo de errores robusto

## 📋 Requisitos

### Sistema
- **Linux**: Kernel 2.6+ con soporte para `/proc` y `/sys`
- **Windows**: Windows 7+ con Visual Studio Build Tools
- **macOS**: macOS 10.12+ con Xcode Command Line Tools

### Dependencias
- **C**: GCC/Clang (Linux/macOS) o MSVC (Windows)
- **Python**: Python 3.7+
- **Python Dependencies**: `psutil` (`pip install psutil`)

## 🏗️ Instalación

### 1. Clonar el repositorio
```bash
git clone https://github.com/your-repo/sysmon.git
cd sysmon
```

### 2. Compilar la biblioteca
```bash
# Compilar solo la biblioteca C
make compile-lib

# O compilar todo (biblioteca + tests)
make all
```

### 3. Instalar dependencias de Python (Linux)
```bash
make python-deps
```

## 🎯 Uso Rápido

### Desde C
```c
#include "extern_api.h"

int main() {
    // Inicializar la biblioteca
    if (sysmon_init() != 0) {
        printf("Error inicializando SysMon\n");
        return -1;
    }
    
    // Obtener métricas del sistema
    SystemStatus status;
    if (sysmon_collect_metrics(&status) == 0) {
        printf("CPU Usage: %.2f%%\n", status.cpu_usage_percent);
        printf("RAM Usage: %.2f%%\n", status.ram_usage_percent);
        printf("CPU Model: %s\n", status.cpu_model);
    }
    
    // Limpiar recursos
    sysmon_cleanup();
    return 0;
}
```

### Desde Python
```python
import ctypes
from ctypes import cdll

# Cargar la biblioteca
lib = cdll.LoadLibrary('./libsysmon.so')  # Linux
# lib = cdll.LoadLibrary('./sysmon.dll')  # Windows

# Inicializar
lib.sysmon_init()

# Obtener métricas
status = lib.sysmon_get_system_status()
print(f"CPU Usage: {status.cpu_usage_percent}%")
print(f"RAM Usage: {status.ram_usage_percent}%")

# Limpiar
lib.sysmon_cleanup()
```

## 🧪 Testing

### Tests Automáticos
```bash
# Test completo (detecta plataforma automáticamente)
make test

# Test específico por plataforma
make python-linux    # Linux
make python-win      # Windows

# Test básico
make python
```

### Tests Manuales
```bash
# Ejecutar test de Python por 10 segundos
cd testing
python3 test_linux.py    # Linux
python3 test_windows.py  # Windows
```

## 📁 Estructura del Proyecto

```
sysmon/
├── src/                    # Código fuente C
│   ├── cpu_linux.c        # Implementación CPU para Linux
│   ├── cpu_windows.c      # Implementación CPU para Windows
│   └── cpu_*.h           # Headers específicos de plataforma
├── common/                # Código común multiplataforma
│   └── common.c          # Funciones utilitarias
├── includes/             # Headers del sistema
├── testing/              # Scripts de Python para testing
│   ├── test_linux.py     # Test específico para Linux
│   ├── test_windows.py   # Test específico para Windows
│   └── test.py          # Test básico
├── extern_api.h          # API pública para FFI
├── Makefile              # Sistema de build
└── README.md            # Este archivo
```

## 🔧 Comandos Makefile

| Comando | Descripción |
|---------|-------------|
| `make compile-lib` | Compila solo la biblioteca C |
| `make all` | Compila biblioteca y ejecuta tests |
| `make test` | Ejecuta tests completos |
| `make python-linux` | Test específico para Linux |
| `make python-win` | Test específico para Windows |
| `make python-deps` | Instala dependencias de Python |
| `make clean` | Limpia archivos compilados |
| `make clean-all` | Limpia todo (incluyendo Python) |
| `make help` | Muestra todos los comandos |

## 📊 Métricas Disponibles

### CPU
- **Uso de CPU** (`cpu_usage_percent`): Porcentaje de uso actual
- **Modelo de CPU** (`cpu_model`): Nombre del procesador
- **Núcleos** (`cpu_cores`): Número de núcleos físicos
- **Frecuencia** (`cpu_frequency_mhz`): Frecuencia actual en MHz
- **Vendor** (`cpu_vendor`): Fabricante del procesador

### Memoria RAM
- **Uso de RAM** (`ram_usage_percent`): Porcentaje de uso
- **Total** (`ram_total_gb`): Memoria total en GB
- **Disponible** (`ram_available_gb`): Memoria disponible en GB
- **Usada** (`ram_used_gb`): Memoria usada en GB

### Sistema
- **OS Name** (`os_name`): Nombre del sistema operativo
- **OS Version** (`os_version`): Versión del sistema operativo
- **Kernel** (`kernel_version`): Versión del kernel
- **Hostname** (`hostname`): Nombre del host
- **Uptime** (`uptime_seconds`): Tiempo activo del sistema

### Discos
- **Espacio total** (`disk_total_gb`): Espacio total en GB
- **Espacio usado** (`disk_used_gb`): Espacio usado en GB
- **Espacio libre** (`disk_free_gb`): Espacio libre en GB
- **Uso de disco** (`disk_usage_percent`): Porcentaje de uso

### Red
- **Interfaces** (`network_interfaces`): Lista de interfaces de red
- **Bytes enviados** (`network_bytes_sent`): Bytes enviados
- **Bytes recibidos** (`network_bytes_recv`): Bytes recibidos

## 🔍 Debugging

### Errores Comunes

1. **Segmentation Fault**: Verificar que los punteros no sean NULL
2. **"ERR en los datos de la cpu"**: Problema de parsing en `/proc/stat`
3. **"undefined symbol"**: Verificar que la biblioteca esté compilada correctamente

### Herramientas de Debug
```bash
# Ver símbolos de la biblioteca
nm -D libsysmon.so

# Debug con gdb
gdb --args python3 test_linux.py

# Verificar dependencias
ldd libsysmon.so
```

## 🤝 Contribuir

1. Fork el proyecto
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

## 📝 Licencia

Este proyecto está bajo la Licencia MIT. Ver el archivo `LICENSE` para más detalles.

## 🐛 Reportar Bugs

Si encuentras un bug, por favor:

1. Verifica que no esté ya reportado en los Issues
2. Incluye información del sistema operativo y versión
3. Proporciona pasos para reproducir el error
4. Adjunta logs de error si están disponibles

---

**SysMonitor** - Para que el puto de Dino no se queje.
