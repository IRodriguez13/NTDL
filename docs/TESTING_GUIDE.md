# Guía de Testing Multi-Plataforma - SysMon

## 🎯 **Objetivo**

Esta guía explica cómo testear SysMon en múltiples sistemas operativos usando Docker y herramientas de CI/CD para asegurar compatibilidad y detectar problemas de segmentación.

## 🐳 **Testing con Docker (Recomendado)**

### **Ventajas del Testing con Docker:**

1. **Aislamiento**: Cada plataforma se testea en un entorno limpio
2. **Consistencia**: Mismos resultados en cualquier máquina
3. **Logging Detallado**: Captura de todos los errores y warnings
4. **Multi-Platform**: Testea Linux, Windows (via Wine), macOS (simulado)
5. **Seguridad**: Evita problemas de segmentación en tu máquina

### **Comandos Básicos:**

```bash
# Testear todas las plataformas
cd docker
docker-compose up test-linux test-windows test-macos

# Testear solo Linux
docker-compose up test-linux

# Testear con logs detallados
docker-compose up test-linux 2>&1 | tee linux-test.log

# Limpiar contenedores
docker-compose down
docker system prune -f
```

### **Estructura de Logs:**

```
docker/logs/
├── linux-test.log      # Logs completos de Linux
├── windows-test.log    # Logs completos de Windows
├── macos-test.log      # Logs completos de macOS
└── python-*-test.log   # Logs de tests de Python
```

## 🚀 **Testing Avanzado con Scripts**

### **Script de Testing con Logs Detallados:**

```bash
# Hacer ejecutable
chmod +x scripts/test-with-logs.sh

# Testear todo con Docker y generar reporte
./scripts/test-with-logs.sh -p all -t all -d -r

# Testear solo Linux localmente
./scripts/test-with-logs.sh -p linux -t all -l

# Testear build en todas las plataformas
./scripts/test-with-logs.sh -p all -t build -d

# Testear con limpieza previa
./scripts/test-with-logs.sh -p all -t all -d -c -r
```

### **Opciones del Script:**

- `-p, --platform`: Especificar plataforma (linux, windows, macos, all)
- `-t, --test`: Tipo de test (build, unit, python, performance, security, integration, all)
- `-d, --docker`: Usar Docker para testing
- `-l, --local`: Testear en máquina local
- `-c, --clean`: Limpiar antes de compilar
- `-r, --report`: Generar reporte detallado
- `-v, --verbose`: Output verboso

## 🔧 **Testing Manual por Plataforma**

### **Linux (Nativo)**

```bash
cd low

# Compilar
make clean && make

# Tests básicos
make test-examples
LD_LIBRARY_PATH=. ./tests/test_minimal
LD_LIBRARY_PATH=. ./tests/example

# Tests de Python
LD_LIBRARY_PATH=. python3 tests/example_simple.py
LD_LIBRARY_PATH=. python3 tests/example_ctypes.py
LD_LIBRARY_PATH=. python3 tests/example_cffi.py

# Tests de rendimiento
time LD_LIBRARY_PATH=. ./tests/example

# Tests de seguridad
cppcheck --enable=all *.c linux/*.c
valgrind --leak-check=full LD_LIBRARY_PATH=. ./tests/example
```

### **Windows (via MinGW)**

```bash
cd low

# Compilar para Windows
make clean
make CC=x86_64-w64-mingw32-gcc

# Verificar que se creó la DLL
ls -la *.dll

# En Windows real, ejecutar:
# tests/example.exe
# python tests/example_simple.py
```

### **macOS (Nativo)**

```bash
cd low

# Compilar
make clean && make

# Tests básicos
make test-examples
DYLD_LIBRARY_PATH=. ./tests/test_minimal
DYLD_LIBRARY_PATH=. ./tests/example

# Tests de Python
DYLD_LIBRARY_PATH=. python3 tests/example_simple.py
```

## 📊 **Interpretación de Resultados**

### **Logs de Build:**

```bash
# Buscar errores de compilación
grep -i "error\|warning" logs/linux-build.log

# Verificar que se crearon los binarios
ls -la low/*.so low/*.dll low/*.dylib
```

### **Logs de Tests:**

```bash
# Verificar que los tests pasaron
grep -i "success\|passed" logs/linux-unit.log

# Buscar errores de segmentación
grep -i "segmentation\|segfault" logs/linux-unit.log

# Verificar métricas recolectadas
grep -E "CPU:|RAM:|GPU:" logs/linux-integration-output.log
```

### **Logs de Python:**

```bash
# Verificar que Python puede cargar la biblioteca
grep -i "loaded\|import" logs/linux-python-simple.log

# Buscar errores de FFI
grep -i "ctypes\|cffi\|ffi" logs/linux-python-ctypes.log
```

## 🚨 **Detección de Problemas Comunes**

### **Segmentation Faults:**

```bash
# Usar gdb para debugging
gdb --args ./tests/example
(gdb) run
(gdb) bt  # Si hay segfault

# Usar valgrind para detectar memory leaks
valgrind --tool=memcheck --leak-check=full ./tests/example
```

### **Problemas de Permisos:**

```bash
# Verificar permisos de archivos del sistema
ls -la /proc/cpuinfo /proc/meminfo /sys/class/thermal/

# Verificar que el usuario puede acceder
sudo -u $USER cat /proc/cpuinfo
```

### **Problemas de Dependencias:**

```bash
# Verificar que las herramientas están instaladas
which lspci nvidia-smi smartctl

# Instalar dependencias faltantes
sudo apt-get install lm-sensors smartmontools pciutils
```

## 🔄 **CI/CD Pipeline (GitHub Actions)**

### **Configuración Automática:**

El pipeline se ejecuta automáticamente en:
- Push a `main` o `develop`
- Pull requests a `main`

### **Jobs del Pipeline:**

1. **test-linux**: Tests en Ubuntu latest
2. **test-windows**: Tests en Windows latest
3. **test-macos**: Tests en macOS latest
4. **integration-tests**: Tests de integración
5. **performance-tests**: Tests de rendimiento
6. **security-tests**: Tests de seguridad
7. **compatibility-tests**: Tests de compatibilidad Python
8. **deploy**: Deployment automático

### **Artifacts Generados:**

- `libsysmon-linux.so`
- `sysmon-windows.dll`
- `libsysmon-macos.dylib`
- Reportes de tests
- Logs detallados

## 📈 **Métricas de Testing**

### **Cobertura de Plataformas:**

| Plataforma | Build | Unit Tests | Python | Performance | Security |
|------------|-------|------------|--------|-------------|----------|
| Linux      | ✅    | ✅         | ✅     | ✅          | ✅       |
| Windows    | ✅    | ⚠️         | ⚠️     | ❌          | ❌       |
| macOS      | ✅    | ⚠️         | ⚠️     | ❌          | ❌       |

### **Tiempos de Testing:**

- **Build**: ~30 segundos por plataforma
- **Unit Tests**: ~10 segundos
- **Python Tests**: ~5 segundos
- **Performance Tests**: ~60 segundos
- **Security Tests**: ~120 segundos

## 🛠️ **Troubleshooting**

### **Problemas Comunes:**

#### **Docker no funciona:**
```bash
# Verificar que Docker está instalado
docker --version
docker-compose --version

# Reiniciar Docker
sudo systemctl restart docker
```

#### **Tests fallan en Windows:**
```bash
# Verificar que Wine está funcionando
wine --version

# Reinstalar Wine
sudo apt-get remove --purge wine*
sudo apt-get install wine
```

#### **Logs no se generan:**
```bash
# Verificar permisos de directorio
ls -la logs/
chmod 755 logs/

# Verificar espacio en disco
df -h
```

### **Debugging Avanzado:**

```bash
# Ejecutar con más verbosidad
./scripts/test-with-logs.sh -p linux -t all -l -v

# Ver logs en tiempo real
tail -f logs/20241201_143022/linux-build.log

# Comparar logs entre ejecuciones
diff logs/run1/linux-build.log logs/run2/linux-build.log
```

## 📋 **Checklist de Testing**

### **Antes de cada Release:**

- [ ] Tests pasan en todas las plataformas
- [ ] No hay segmentation faults
- [ ] Memory leaks verificados con valgrind
- [ ] Python FFI funciona correctamente
- [ ] Performance dentro de límites aceptables
- [ ] Logs de errores revisados
- [ ] Reporte de testing generado
- [ ] Artifacts de build creados

### **Mantenimiento Semanal:**

- [ ] Ejecutar tests completos
- [ ] Revisar logs de errores
- [ ] Actualizar dependencias si es necesario
- [ ] Verificar que Docker images están actualizadas
- [ ] Revisar métricas de rendimiento

## 🎯 **Próximos Pasos**

1. **Automatización**: Configurar testing automático en commits
2. **Coverage**: Aumentar cobertura de tests
3. **Performance**: Optimizar tiempos de testing
4. **Monitoring**: Dashboard para métricas de testing
5. **Integration**: Integrar con sistemas de CI/CD externos
