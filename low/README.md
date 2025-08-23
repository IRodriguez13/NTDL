# SysMon - Biblioteca de Monitoreo del Sistema

Una biblioteca multiplataforma para obtener métricas del sistema (CPU, RAM, Disco) en tiempo real.

## Características

- **Multiplataforma**: Soporte para Linux, Windows y macOS (XNU)
- **Métricas en tiempo real**: Uso de CPU, memoria RAM y estado del disco
- **API simple**: Funciones fáciles de usar para obtener métricas específicas
- **Formato JSON**: Exportación de métricas en formato JSON
- **Biblioteca compartida**: Puede ser enlazada con otros programas
- **Integración Python**: Fácil uso desde Python con ctypes, cffi o wrapper personalizado

## Métricas Disponibles

- **CPU**: Modelo, número de núcleos, porcentaje de uso
- **RAM**: Total, libre, usada y porcentaje de uso
- **Disco**: Modelo y salud del disco
- **Sistema**: Nombre del sistema operativo

## Compilación

### Requisitos

- GCC o Clang
- Make
- En macOS: Xcode Command Line Tools

### Compilar

```bash
# Compilar la biblioteca y el ejemplo
make

# Ver información de la plataforma detectada
make info

# Solo compilar la biblioteca
make libsysmon.so  # Linux
make libsysmon.dylib  # macOS
make sysmon.dll  # Windows
```

### Instalar (Linux/macOS)

```bash
# Instalar en el sistema
sudo make install

# Desinstalar
sudo make uninstall
```

## Uso

### Ejemplo Básico

```c
#include "sysmon.h"
#include <stdio.h>

int main() {
    // Inicializar
    sysmon_init();
    
    // Obtener métricas específicas
    printf("CPU: %.2f%%\n", sysmon_get_cpu_usage());
    printf("RAM: %.2f%%\n", sysmon_get_ram_usage_percent());
    printf("Cores: %d\n", sysmon_get_cpu_cores());
    
    // Obtener todas las métricas
    SystemStatus status;
    sysmon_get_metrics(&status);
    
    // Obtener como JSON
    char *json = sysmon_get_metrics_json();
    printf("JSON: %s\n", json);
    free_json(json);
    
    // Limpiar
    sysmon_cleanup();
    return 0;
}
```

### Compilar tu programa

```bash
# Linux/macOS
gcc -o mi_programa mi_programa.c -L. -lsysmon

# Windows
gcc -o mi_programa.exe mi_programa.c -L. -lsysmon
```

## API de Funciones

### Inicialización
- `int sysmon_init(void)` - Inicializar el sistema de monitoreo
- `void sysmon_cleanup(void)` - Limpiar recursos

### Métricas Generales
- `int sysmon_get_metrics(SystemStatus *status)` - Obtener todas las métricas
- `char* sysmon_get_metrics_json(void)` - Obtener métricas como JSON

### Métricas Específicas
- `float sysmon_get_cpu_usage(void)` - Porcentaje de uso de CPU
- `int sysmon_get_cpu_cores(void)` - Número de núcleos CPU
- `const char* sysmon_get_cpu_model(void)` - Modelo de CPU
- `unsigned long sysmon_get_ram_total(void)` - RAM total en KB
- `unsigned long sysmon_get_ram_free(void)` - RAM libre en KB
- `unsigned long sysmon_get_ram_used(void)` - RAM usada en KB
- `float sysmon_get_ram_usage_percent(void)` - Porcentaje de uso de RAM
- `const char* sysmon_get_os_name(void)` - Nombre del sistema operativo
- `const char* sysmon_get_disk_model(void)` - Modelo del disco
- `int sysmon_get_disk_health(void)` - Salud del disco (%)

## Estructura de Datos

```c
typedef struct {
    char os_name[32];           // Nombre del SO
    char cpu_model[128];        // Modelo de CPU
    int cpu_cores;              // Número de núcleos
    float cpu_usage_percent;    // Uso de CPU (%)
    unsigned long ram_total_kb; // RAM total (KB)
    unsigned long ram_free_kb;  // RAM libre (KB)
    char disk_model[128];       // Modelo del disco
    int disk_health;            // Salud del disco (%)
} SystemStatus;
```

## Ejecutar los Ejemplos

```bash
# Compilar la biblioteca
make

# Ejecutar ejemplo en C
./example

# Ejecutar ejemplos de Python
python3 example_simple.py
python3 example_ctypes.py
python3 example_cffi.py
```

## Plataformas Soportadas

- **Linux**: Usa `/proc` filesystem y `sysinfo()`
- **macOS (XNU)**: Usa Mach APIs y IOKit
- **Windows**: Usa Windows APIs (Registry, Performance Counters)

## Notas de Implementación

- El cálculo de uso de CPU requiere múltiples lecturas para obtener valores precisos
- La información del disco puede variar según la plataforma
- En Windows, algunas métricas requieren permisos de administrador

## Uso desde Python

### Requisitos

```bash
# Instalar dependencias (opcional, solo para cffi)
pip install cffi
```

### Ejemplos de Uso

#### Usando ctypes (incluido en Python)

```python
import ctypes

# Cargar la biblioteca
sysmon = ctypes.CDLL("./libsysmon.so")  # Linux
# sysmon = ctypes.CDLL("./libsysmon.dylib")  # macOS
# sysmon = ctypes.CDLL("./sysmon.dll")  # Windows

# Inicializar
sysmon.sysmon_init()

# Obtener métricas
cpu_usage = sysmon.sysmon_get_cpu_usage()
ram_usage = sysmon.sysmon_get_ram_usage_percent()

print(f"CPU: {cpu_usage:.2f}%")
print(f"RAM: {ram_usage:.2f}%")

# Limpiar
sysmon.sysmon_cleanup()
```

#### Usando cffi

```python
from cffi import FFI

ffi = FFI()
ffi.cdef("""
    int sysmon_init(void);
    float sysmon_get_cpu_usage(void);
    float sysmon_get_ram_usage_percent(void);
    void sysmon_cleanup(void);
""")

C = ffi.dlopen("./libsysmon.so")
C.sysmon_init()
cpu_usage = C.sysmon_get_cpu_usage()
C.sysmon_cleanup()
```

#### Usando el wrapper de Python (Recomendado)

```python
from sysmon_python import SysMon, get_system_metrics

# Método simple
metrics = get_system_metrics()
print(f"CPU: {metrics.cpu_usage:.1f}%")
print(f"RAM: {metrics.ram_usage_percent:.1f}%")

# Método con más control
with SysMon() as sysmon:
    cpu = sysmon.get_cpu_usage()
    ram = sysmon.get_ram_usage_percent()
    print(f"CPU: {cpu:.1f}% | RAM: {ram:.1f}%")
```

### Ejecutar ejemplos de Python

```bash
# Ejemplo con ctypes
python3 example_ctypes.py

# Ejemplo con cffi
python3 example_cffi.py

# Ejemplo simple con wrapper
python3 example_simple.py
```

## Licencia

Este proyecto está bajo licencia MIT.
