# Arquitectura Modular de SysMon

## Estructura del Proyecto Modularizado

La librería SysMon ha sido completamente modularizada para facilitar el mantenimiento, extensibilidad y soporte multiplataforma.

### Estructura de Directorios

```
sysmon/
├── metrics.h                    # Definiciones de estructuras de datos
├── extern_api.h                 # API pública para FFI
├── extern_api.c                 # Implementación de la API pública
├── core/                        # Núcleo central del sistema
│   ├── sysmon_core.h           # Interfaz del núcleo
│   └── sysmon_core.c           # Implementación del núcleo
├── components/                  # Componentes modulares
│   ├── cpu/                    # Módulo de CPU
│   │   ├── cpu_interface.h     # Interfaz común de CPU
│   │   ├── cpu_linux.c         # Implementación para Linux
│   │   └── cpu_windows.c       # Implementación para Windows (futuro)
│   ├── gpu/                    # Módulo de GPU
│   │   ├── gpu_interface.h     # Interfaz común de GPU
│   │   ├── gpu_linux.c         # Implementación para Linux
│   │   └── gpu_windows.c       # Implementación para Windows (futuro)
│   ├── memory/                 # Módulo de memoria
│   │   ├── memory_interface.h  # Interfaz común de memoria
│   │   ├── memory_linux.c      # Implementación para Linux
│   │   └── memory_windows.c    # Implementación para Windows (futuro)
│   ├── disk/                   # Módulo de discos
│   │   ├── disk_interface.h    # Interfaz común de discos
│   │   ├── disk_linux.c        # Implementación para Linux
│   │   └── disk_windows.c      # Implementación para Windows (futuro)
│   ├── network/                # Módulo de red
│   │   ├── network_interface.h # Interfaz común de red
│   │   ├── network_linux.c     # Implementación para Linux
│   │   └── network_windows.c   # Implementación para Windows (futuro)
│   └── sensors/                # Módulo de sensores
│       ├── sensors_interface.h # Interfaz común de sensores
│       ├── sensors_linux.c     # Implementación para Linux
│       └── sensors_windows.c   # Implementación para Windows (futuro)
├── common/                     # Código común y utilidades
│   ├── common.h               # Definiciones comunes
│   ├── common.c               # Implementaciones comunes
│   └── main.c                 # Programa principal de prueba
└── testing/                   # Scripts de prueba
    ├── test_modular.py        # Test completo del sistema modular
    ├── test_enhanced.py       # Test mejorado original
    └── test_windows.py        # Test para Windows
```

## Componentes Implementados

### 1. CPU (components/cpu/)
- **Funcionalidades**: Modelo, vendor, núcleos, frecuencia, uso, temperatura
- **Soporte**: Linux completo, Windows (futuro)
- **Características especiales**: 
  - Información por núcleo individual
  - Detección de arquitectura
  - Monitoreo de temperatura desde sensores térmicos
  - Frecuencias dinámicas y estáticas

### 2. GPU (components/gpu/)
- **Funcionalidades**: Detección NVIDIA/AMD/Intel, memoria VRAM, temperatura, uso
- **Soporte**: Linux (NVIDIA completo, AMD/Intel básico)
- **Características especiales**:
  - Soporte para múltiples GPUs
  - Información detallada de NVIDIA via nvidia-smi
  - Detección automática de fabricante
  - Métricas de potencia y ventiladores

### 3. Memoria (components/memory/)
- **Funcionalidades**: RAM total/usada/libre/disponible, caché, buffers, swap
- **Soporte**: Linux completo
- **Características especiales**:
  - Información detallada de uso de memoria
  - Separación entre memoria libre y disponible
  - Información de swap
  - Métricas de memoria compartida

### 4. Discos (components/disk/)
- **Funcionalidades**: Información SMART, temperatura, uso, tipo (SSD/HDD)
- **Soporte**: Linux con smartctl y acceso directo
- **Características especiales**:
  - Detección automática de discos montados
  - Información SMART (temperatura, horas de uso)
  - Detección SSD vs HDD
  - Estadísticas de I/O
  - Soporte para múltiples sistemas de archivos

### 5. Red (components/network/)
- **Funcionalidades**: Interfaces, IPs, MACs, estadísticas de tráfico
- **Soporte**: Linux completo
- **Características especiales**:
  - Detección de interfaces inalámbricas
  - Estadísticas de paquetes y errores
  - Información de velocidad de enlace
  - Estado de interfaces (UP/DOWN)

### 6. Sensores (components/sensors/)
- **Funcionalidades**: Temperatura, ventiladores, voltajes, potencia
- **Soporte**: Linux via hwmon y thermal zones
- **Características especiales**:
  - Clasificación automática por tipo
  - Valores mínimos/máximos/críticos
  - Soporte para múltiples fuentes de sensores
  - Etiquetas descriptivas

## Núcleo Central (core/)

El núcleo central (`sysmon_core`) actúa como orquestador de todos los componentes:

- **Inicialización**: Coordina la inicialización de todos los módulos
- **Recolección**: Unifica la recolección de métricas de todos los componentes
- **Cache**: Mantiene un cache de métricas para optimizar el rendimiento
- **Compatibilidad**: Mantiene compatibilidad con la API anterior

## API Externa (extern_api.h/c)

La API externa proporciona:

- **Compatibilidad total** con la API anterior
- **Nuevas funciones** para acceder a métricas específicas
- **Getters individuales** para cada tipo de métrica
- **Funciones de utilidad** para información del sistema

## Compilación

```bash
# Compilar la librería modular
make compile-lib

# Ejecutar test modular completo
make python-modular

# Ejecutar todos los tests
make test
```

## Uso desde Python

```python
# Importar y usar el sistema modular
from test_modular import SysMonModular

sysmon = SysMonModular()
sysmon.initialize()

# Recolectar todas las métricas
status = sysmon.collect_all_metrics()

# Acceder a métricas específicas
print(f"CPU: {status.cpu.model.decode()}")
print(f"RAM: {status.ram.usage_percent:.1f}%")
print(f"GPUs: {status.num_gpus}")
print(f"Discos: {status.num_disks}")
print(f"Sensores: {status.num_sensors}")
```

## Ventajas de la Arquitectura Modular

1. **Mantenibilidad**: Cada componente es independiente
2. **Extensibilidad**: Fácil agregar nuevos componentes o plataformas
3. **Testabilidad**: Cada módulo se puede probar independientemente
4. **Rendimiento**: Inicialización y limpieza optimizadas
5. **Compatibilidad**: Mantiene la API anterior intacta
6. **Escalabilidad**: Soporte para múltiples dispositivos del mismo tipo

## Próximos Pasos

1. **Soporte Windows**: Implementar versiones Windows de cada componente
2. **Soporte macOS**: Agregar soporte para macOS/Darwin
3. **Más componentes**: Batería, motherboard, BIOS
4. **Optimizaciones**: Cache inteligente, recolección asíncrona
5. **Documentación**: API docs completa