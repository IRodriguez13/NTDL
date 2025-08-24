# SysMon - Biblioteca de Monitoreo de Sistema Multi-Plataforma

## 📋 Descripción

SysMon es una biblioteca C de alto rendimiento que proporciona acceso completo a métricas del sistema en tiempo real. Diseñada para ser consumida desde aplicaciones externas mediante FFI (Foreign Function Interface), es compatible con Python (ctypes/cffi), Rust, Go y otros lenguajes.

## 🎯 Características Principales

- **Multi-Plataforma**: Soporte completo para Linux, Windows y macOS (XNU)
- **Alto Rendimiento**: Acceso directo a APIs del kernel sin dependencias externas
- **API Unificada**: Misma interfaz en todas las plataformas
- **Métricas Detalladas**: CPU, RAM, discos, red, GPU, sensores, motherboard
- **Tiempo Real**: Monitoreo continuo con actualizaciones en tiempo real
- **FFI Ready**: Optimizada para integración con otros lenguajes

## 🏗️ Arquitectura

```
sysmon/
├── low/                    # Implementación principal
│   ├── metrics.h          # Estructuras de datos
│   ├── extern_api.h       # API pública final
│   ├── sysmon.h           # API interna
│   ├── sysmon.c           # Implementación de la API
│   ├── linux/             # Implementación específica de Linux
│   ├── windows/           # Implementación específica de Windows
│   ├── xnu/               # Implementación específica de macOS
│   ├── tests/             # Tests y ejemplos
│   ├── scripts/           # Scripts de Python
│   └── docs/              # Documentación
```

## 🚀 Instalación

### Compilación

```bash
cd low
make clean && make
```

### Instalación del Sistema

```bash
make install
```

## 📖 Uso Rápido

### C

```c
#include "extern_api.h"

int main() {
    // Inicializar la biblioteca
    if (sysmon_init() != 0) {
        printf("Error al inicializar sysmon\n");
        return -1;
    }
    
    // Recolectar métricas
    SystemStatus status;
    if (sysmon_collect_metrics(&status) == 0) {
        printf("CPU: %.2f%% | RAM: %.2f%%\n", 
               status.cpu_usage_percent, 
               status.ram_usage_percent);
    }
    
    // Limpiar recursos
    sysmon_cleanup();
    return 0;
}
```

### Python (ctypes)

```python
import ctypes
from ctypes import *

# Cargar la biblioteca
lib = ctypes.CDLL("./libsysmon.so")

# Definir estructuras
class SystemStatus(Structure):
    _fields_ = [
        ("os_name", c_char * 32),
        ("cpu_usage_percent", c_float),
        ("ram_usage_percent", c_float),
        # ... más campos
    ]

# Usar la biblioteca
lib.sysmon_init()
status = SystemStatus()
lib.sysmon_collect_metrics(byref(status))
print(f"CPU: {status.cpu_usage_percent}%")
lib.sysmon_cleanup()
```

### Python (cffi)

```python
from cffi import FFI

ffi = FFI()
ffi.cdef("""
    int sysmon_init(void);
    int sysmon_collect_metrics(SystemStatus *status);
    void sysmon_cleanup(void);
""")

lib = ffi.dlopen("./libsysmon.so")
lib.sysmon_init()
# ... usar la biblioteca
lib.sysmon_cleanup()
```

## 📊 Métricas Disponibles

### CPU
- Modelo, núcleos, threads
- Uso por núcleo y total
- Frecuencias (actual, mín, máx)
- Temperaturas por núcleo
- Voltajes por núcleo

### Memoria RAM
- Total, libre, usado
- Porcentaje de uso
- Tipo (DDR3/DDR4/DDR5)
- Frecuencia
- Número de módulos
- Información de caché (L1/L2/L3)

### Discos
- Modelo, protocolo (SATA/SSD/NVMe)
- Capacidad total y disponible
- Porcentaje de uso
- Temperatura
- Información SMART
- Punto de montaje

### Red
- Interfaces activas
- MAC, IP, máscara, gateway
- Estadísticas (bytes, paquetes, errores)
- Velocidad, duplex, MTU

### GPU
- Modelo, driver, fabricante
- Memoria total/usada/libre
- Uso de GPU y memoria
- Temperatura, ventilador
- Frecuencias de clock

### Sensores
- Temperaturas del sistema
- Voltajes
- Velocidades de ventiladores

### Sistema
- Información de OS y kernel
- Carga promedio (1/5/15 min)
- Procesos (total, ejecutándose, etc.)
- Tiempo de actividad
- Información de energía

## 🔧 APIs Utilizadas

### Linux
- `/proc` filesystem
- `/sys` filesystem
- `sysinfo()`, `getloadavg()`
- `glob()`, `popen()`

### Windows
- Registry APIs
- Performance Counters (PDH)
- `GlobalMemoryStatusEx()`
- `GetSystemInfo()`

### macOS (XNU)
- Mach APIs
- `sysctl` y `sysctlbyname`
- IOKit
- CoreFoundation

## 🧪 Testing

```bash
# Ejecutar todos los tests
make run-tests

# Tests específicos
make test-examples
./tests/test_minimal
./tests/example
python3 tests/example_simple.py
```

## 📚 Documentación

- [API Reference](API_REFERENCE.md)
- [Platform Support](PLATFORM_SUPPORT.md)
- [Examples](EXAMPLES.md)
- [Performance Guide](PERFORMANCE.md)
- [Troubleshooting](TROUBLESHOOTING.md)

## 🤝 Contribución

1. Fork el proyecto
2. Crea una rama para tu feature
3. Commit tus cambios
4. Push a la rama
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Ver [LICENSE](LICENSE) para más detalles.

## 🆘 Soporte

- **Issues**: [GitHub Issues](https://github.com/your-repo/sysmon/issues)
- **Documentación**: [docs/](docs/)
- **Ejemplos**: [tests/](tests/)

## 🔄 Roadmap

- [ ] Soporte para FreeBSD
- [ ] Métricas de red más detalladas
- [ ] Soporte para GPUs NVIDIA/AMD específico
- [ ] API asíncrona
- [ ] Métricas de contenedores (Docker)
- [ ] Integración con Prometheus
