# Scripts de Python para SysMon

Este directorio contiene scripts de Python para probar la librería SysMon desde Python usando ctypes.

## Archivos

- `test.py` - Test básico de la librería
- `test_python_enhanced.py` - Test mejorado con monitoreo en tiempo real (10 segundos)
- `libsysmon.so` - Librería compartida compilada
- `Makefile` - Comandos para ejecutar los tests

## Requisitos

- Python 3.x
- psutil (`sudo apt install python3-psutil`)
- Librería libsysmon.so compilada

## Uso

### Usando Makefile (recomendado)

```bash
# Ejecutar test mejorado (por defecto)
make

# Ejecutar test básico
make test

# Ejecutar test mejorado (10 segundos)
make test-enhanced

# Ejecutar ambos tests
make all

# Mostrar ayuda
make help

# Limpiar archivos temporales
make clean
```

### Usando Python directamente

```bash
# Test básico
python3 test.py

# Test mejorado (10 segundos)
python3 test_python_enhanced.py
```

## Funcionalidades del Test Mejorado

El script `test_python_enhanced.py` incluye:

1. **Información del sistema** - Usando Python (psutil, platform)
2. **Información del CPU** - Usando la librería C
3. **Modelo de CPU** - Usando get_cpu_model()
4. **Monitoreo en tiempo real** - Por 10 segundos mostrando:
   - Uso de CPU (desde librería C y Python)
   - Frecuencia de CPU
   - Uso de RAM
   - Load average (1, 5, 15 minutos)
5. **Comparación de métricas** - Entre librería C y Python

## Salida de ejemplo

```
=== Test Python Mejorado de SysMon (10 segundos) ===
✅ Librería cargada correctamente

0. Información del sistema (Python):
----------------------------------------
   Plataforma: Linux #78-Ubuntu SMP PREEMPT_DYNAMIC Tue Aug 12 11:34:18 UTC 2025
   Arquitectura: x86_64
   Procesador: x86_64
   Núcleos físicos: 12
   Frecuencia actual: 4199 MHz
   Frecuencia mínima: 1400 MHz
   Frecuencia máxima: 4200 MHz
   Memoria total: 13.4 GB
   Memoria disponible: 8.2 GB

1. Información del CPU (Librería C):
----------------------------------------
   Vendor: N/A
   Model:  N/A
   Cores:  0
   MHz:    0

2. Modelo de CPU (get_cpu_model):
----------------------------------------
   Modelo: AMD Ryzen 5 5600G with Radeon Graphics

3. Monitoreo en tiempo real (10 segundos):
================================================================================
Tiempo CPU%     FreqMHz    RAM%     Load1    Load5    Load15  
================================================================================
  0.1s   -1.00%          0MHz   39.2%     1.53     1.80     2.00
  1.2s   -1.00%          0MHz   39.2%     1.48     1.78     2.00
  ...
================================================================================
✅ Test completado exitosamente
La librería funciona correctamente desde Python

4. Información adicional:
----------------------------------------
   Iteraciones completadas: 10
   Tiempo total de ejecución: 11.01 segundos
   Frecuencia de muestreo: 1 Hz

5. Comparación de métricas (última lectura):
----------------------------------------
   CPU Usage (C): -1.00%
   CPU Usage (Python): 16.00%
   RAM Usage: 38.9%
   Load Average: 1.44, 1.77, 1.99
```

## Notas importantes

- La librería C actualmente tiene algunos problemas:
  - `alloc_cpu_info()` no llena correctamente los datos (vendor, model, cores, mhz)
  - `get_cpu_usage()` retorna -1.00% (error)
  - `get_cpu_model()` funciona correctamente

- El script usa psutil como respaldo para obtener métricas confiables
- El monitoreo se ejecuta por exactamente 10 segundos con frecuencia de 1 Hz
- Se libera correctamente la memoria después de cada llamada a la librería C

## Troubleshooting

### Error: "undefined symbol"
- Verificar que libsysmon.so esté compilada correctamente
- Usar `nm -D libsysmon.so` para ver símbolos disponibles

### Error: "psutil not found"
- Instalar: `sudo apt install python3-psutil`

### Error: "Permission denied"
- Verificar permisos de ejecución: `chmod +x *.py`

## Desarrollo

Para agregar nuevas funcionalidades:

1. Modificar `test_python_enhanced.py`
2. Agregar nuevas funciones a la librería C
3. Actualizar este README
4. Probar con `make test-enhanced`
