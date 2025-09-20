CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -g -std=c99 -D_GNU_SOURCE
TARGET_LINUX = libsysmon.so
TARGET_WINDOWS = sysmon.dll
TARGET_MACOS = libsysmon.dylib

# Directorios
BUILD_DIR = build
TESTING_DIR = testing

# Código fuente común
SRC_COMMON = common/common.c core/sysmon_core.c extern_api.c

# Código específico por plataforma
SRC_LINUX = components/cpu/cpu_linux.c \
             components/gpu/gpu_linux.c \
             components/memory/memory_linux.c \
             components/disk/disk_linux.c \
             components/network/network_linux.c \
             components/sensors/sensors_linux.c \
             components/display/display_linux.c \
             components/battery/battery_linux.c \
             components/audio/audio_linux.c \
             components/advanced/advanced_linux.c

SRC_WINDOWS = components/cpu/cpu_windows.c \
              components/gpu/gpu_windows.c \
              components/memory/memory_windows.c \
              components/disk/disk_windows.c \
              components/network/network_windows.c \
              components/sensors/sensors_windows.c \
              components/display/display_windows.c \
              components/battery/battery_windows.c \
              components/audio/audio_windows.c \
              components/advanced/advanced_windows.c

SRC_MACOS = components/cpu/cpu_macos.c \
            components/gpu/gpu_macos.c \
            components/memory/memory_macos.c \
            components/disk/disk_macos.c \
            components/network/network_macos.c \
            components/sensors/sensors_macos.c \
            components/display/display_macos.c \
            components/battery/battery_macos.c \
            components/audio/audio_macos.c \
            components/advanced/advanced_macos.c

# Detección automática de plataforma
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    SRC_PLATFORM = $(SRC_WINDOWS)
    TARGET = $(TARGET_WINDOWS)
    SHARED_FLAG = -shared
    LDFLAGS = -ladvapi32 -lole32 -loleaut32 -lwbemuuid
    PLATFORM_LIBS = 
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = linux
        SRC_PLATFORM = $(SRC_LINUX)
        TARGET = $(TARGET_LINUX)
        SHARED_FLAG = -shared
        LDFLAGS = -lX11 -lXrandr
        PLATFORM_LIBS = 
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
        SRC_PLATFORM = $(SRC_MACOS)
        TARGET = $(TARGET_MACOS)
        SHARED_FLAG = -dynamiclib
        LDFLAGS = -framework CoreFoundation -framework IOKit -framework CoreGraphics
        PLATFORM_LIBS = 
    endif
endif

# Archivos fuente y objetos
SRCS = $(SRC_COMMON) $(SRC_PLATFORM)
OBJS = $(SRCS:.c=.o)

# Crear directorio de build si no existe
$(shell mkdir -p $(BUILD_DIR))

# Variables para Python
PYTHON = python3
PYTHON_SCRIPTS_DIR = testing
PYTHON_TEST = $(PYTHON_SCRIPTS_DIR)/test.py
PYTHON_TEST_DETAILED = $(PYTHON_SCRIPTS_DIR)/test_detailed_hardware.py
PYTHON_TEST_ENHANCED = $(PYTHON_SCRIPTS_DIR)/test_enhanced.py
PYTHON_TEST_LINUX = $(PYTHON_SCRIPTS_DIR)/test_linux.py
PYTHON_TEST_WINDOWS = $(PYTHON_SCRIPTS_DIR)/test_windows.py
PYTHON_TEST_GPU = $(PYTHON_SCRIPTS_DIR)/test_gpu.py

all: $(TARGET)

# Compilar solo la librería
compile-lib: $(TARGET)

$(TARGET): $(OBJS)
	@echo "🔗 Enlazando librería para $(PLATFORM)..."
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $(OBJS) $(LDFLAGS) $(PLATFORM_LIBS)
	@echo "✅ Librería $(TARGET) compilada exitosamente para $(PLATFORM)"
	@echo "📋 Verificando símbolos exportados:"
	@if [ "$(PLATFORM)" = "linux" ]; then \
		nm -D $(TARGET) | grep -E "(sysmon_init|sysmon_get_cpu|sysmon_collect)" | head -5 || echo "⚠ Verificación de símbolos omitida"; \
	fi
	@echo "📁 Copiando librería al directorio de testing..."
	@cp $(TARGET) $(TESTING_DIR)/

# regla implícita para .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Comandos de Python mejorados
python: $(TARGET)
	@echo "=== Ejecutando test básico de Python ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test.py

python-enhanced: $(TARGET)
	@echo "=== Ejecutando test mejorado de Python ==="
	@if [ -f "$(PYTHON_TEST_ENHANCED)" ]; then \
		cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_enhanced.py; \
	else \
		echo "⚠ test_enhanced.py no encontrado, usando test básico"; \
		cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test.py; \
	fi

python-detailed: $(TARGET)
	@echo "=== Ejecutando test detallado de Python ==="
	@if [ -f "$(PYTHON_TEST_DETAILED)" ]; then \
		cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_detailed_hardware.py; \
	else \
		echo "⚠ test_detailed_hardware.py no encontrado, usando test básico"; \
		cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test.py; \
	fi

python-gpu: $(TARGET)
	@echo "=== Ejecutando test de GPU ==="
	@if [ -f "$(PYTHON_TEST_GPU)" ]; then \
		cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_gpu.py; \
	else \
		echo "⚠ test_gpu.py no encontrado"; \
	fi

python-linux: $(TARGET)
	@echo "=== Ejecutando test de Python para Linux (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_linux.py

python-win: $(TARGET)
	@echo "=== Ejecutando test de Python para Windows (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_windows.py

python-modular: $(TARGET)
	@echo "=== Ejecutando test modular completo ==="
	cd $(TESTING_DIR) && $(PYTHON) test_modular.py

python-detailed: $(TARGET)
	@echo "=== Ejecutando test detallado de hardware (10 segundos) ==="
	cd $(TESTING_DIR) && $(PYTHON) test_detailed_hardware.py

python-simple: $(TARGET)
	@echo "=== Ejecutando test simple modular ==="
	cd $(TESTING_DIR) && $(PYTHON) test_simple_modular.py

python-detection: $(TARGET)
	@echo "=== Ejecutando detección de hardware ==="
	cd $(TESTING_DIR) && $(PYTHON) test_hardware_detection.py

python-professional: $(TARGET)
	@echo "=== Ejecutando test profesional (para aplicaciones de escritorio) ==="
	cd $(TESTING_DIR) && $(PYTHON) test_professional.py

python-complete: $(TARGET)
	@echo "=== Ejecutando test completo del sistema (TODOS los componentes) ==="
	cd $(TESTING_DIR) && $(PYTHON) test_complete_system.py

python-all: python python-enhanced python-gpu python-linux python-win python-modular python-professional

# Comando por defecto para Python (detecta automáticamente la plataforma)
python-default: $(TARGET)
	@echo "=== Detectando plataforma para test de Python ==="
	@if [ "$(OS)" = "Windows_NT" ]; then \
		echo "Plataforma detectada: Windows"; \
		$(MAKE) python-win; \
	else \
		echo "Plataforma detectada: Linux"; \
		$(MAKE) python-enhanced; \
	fi

# Instalar dependencias de Python
python-deps:
	@echo "=== Instalando dependencias de Python ==="
	@if command -v apt >/dev/null 2>&1; then \
		sudo apt update && sudo apt install -y python3-psutil; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y python3-psutil; \
	elif command -v pip3 >/dev/null 2>&1; then \
		pip3 install psutil; \
	else \
		echo "⚠ No se pudo detectar el gestor de paquetes. Instale psutil manualmente."; \
	fi
	@echo "✅ Dependencias instaladas"

# Verificar que Python y dependencias estén disponibles
python-check:
	@echo "=== Verificando Python y dependencias ==="
	@which $(PYTHON) > /dev/null || (echo "❌ Python3 no encontrado" && exit 1)
	@$(PYTHON) -c "import psutil" 2>/dev/null || (echo "❌ psutil no encontrado. Ejecuta: make python-deps" && exit 1)
	@echo "✅ Python y dependencias OK"

# Test completo: compilar + test Python (detecta plataforma)
test: $(TARGET) python-check python-complete

# Test profesional para aplicaciones de escritorio
test-professional: $(TARGET) python-check python-professional

# Test rápido para desarrollo
test-quick: $(TARGET) python-simple

# Test específico para GPU
test-gpu: $(TARGET) python-check python-gpu

# Debug: compilar con información de debugging
debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)
	@echo "✅ Librería compilada en modo DEBUG"

# Verificar la librería compilada
verify: $(TARGET)
	@echo "=== Verificando librería compilada ==="
	@file $(TARGET)
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		ldd $(TARGET) 2>/dev/null || echo "⚠ ldd no disponible"; \
	fi

clean:
	rm -f $(OBJS) $(TARGET)
	find . -name "*.o" -delete 2>/dev/null || true
	@echo "✅ Archivos compilados eliminados"

clean-python:
	cd $(PYTHON_SCRIPTS_DIR) && rm -f *.pyc __pycache__/* .pytest_cache/* 2>/dev/null || true
	rm -f $(PYTHON_SCRIPTS_DIR)/report.txt 2>/dev/null || true
	@echo "✅ Archivos temporales de Python eliminados"

clean-all: clean clean-python

# Instalar la librería en el sistema (solo Linux)
install: $(TARGET)
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		sudo cp $(TARGET) /usr/local/lib/; \
		sudo ldconfig; \
		echo "✅ Librería instalada en /usr/local/lib/"; \
	else \
		echo "❌ Install no disponible en esta plataforma"; \
	fi

# Desinstalar la librería del sistema
uninstall:
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		sudo rm -f /usr/local/lib/$(TARGET); \
		sudo ldconfig; \
		echo "✅ Librería desinstalada"; \
	else \
		echo "❌ Uninstall no disponible en esta plataforma"; \
	fi

help:
	@echo "Comandos disponibles:"
	@echo "  make              - Compilar la librería"
	@echo "  make compile-lib  - Compilar solo la librería C"
	@echo "  make debug        - Compilar en modo debug"
	@echo "  make verify       - Verificar librería compilada"
	@echo ""
	@echo "Tests de Python:"
	@echo "  make python       - Test básico de Python"
	@echo "  make python-enhanced - Test mejorado de Python"
	@echo "  make python-gpu   - Test específico de GPU"
	@echo "  make python-linux - Test de Python para Linux (10 segundos)"
	@echo "  make python-win   - Test de Python para Windows (10 segundos)"
	@echo "  make python-all   - Ejecutar todos los tests de Python"
	@echo "  make python-default - Auto-detectar plataforma y ejecutar test"
	@echo ""
	@echo "Tests específicos:"
	@echo "  make test         - Compilar + test Python (detecta plataforma)"
	@echo "  make test-gpu     - Test específico de GPU"
	@echo ""
	@echo "Utilidades:"
	@echo "  make python-deps  - Instalar dependencias de Python"
	@echo "  make python-check - Verificar Python y dependencias"
	@echo "  make install      - Instalar librería en el sistema (Linux)"
	@echo "  make uninstall    - Desinstalar librería del sistema (Linux)"
	@echo ""
	@echo "Limpieza:"
	@echo "  make clean        - Limpiar archivos compilados"
	@echo "  make clean-python - Limpiar archivos temporales de Python"
	@echo "  make clean-all    - Limpiar todo"
	@echo "  make help         - Mostrar esta ayuda"

.PHONY: all compile-lib debug verify python python-enhanced python-gpu python-linux python-win python-all python-default python-deps python-check test test-gpu install uninstall clean clean-python clean-all help