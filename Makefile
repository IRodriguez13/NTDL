CC = gcc
CFLAGS = -Wall -O2 -fPIC -g      # -g para debugging, -fPIC para librerías compartidas
TARGET_LINUX = libsysmon.so
TARGET_WINDOWS = sysmon.dll

SRC_COMMON = common/common.c
SRC_LINUX = src/cpu_linux.c src/disk_linux.c
SRC_WINDOWS = src/cpu_windows.c

# Detección de OS
ifeq ($(OS),Windows_NT)
    SRC_PLATFORM = $(SRC_WINDOWS)
    TARGET = $(TARGET_WINDOWS)
    SHARED_FLAG = -shared
    LDFLAGS = -ladvapi32  # Para acceder al registro de Windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        SRC_PLATFORM = $(SRC_LINUX)
        TARGET = $(TARGET_LINUX)
        SHARED_FLAG = -shared
        LDFLAGS = 
    endif
endif

SRCS = $(SRC_COMMON) $(SRC_PLATFORM)
OBJS = $(SRCS:.c=.o)

# Variables para Python
PYTHON = python3
PYTHON_SCRIPTS_DIR = testing
PYTHON_TEST = $(PYTHON_SCRIPTS_DIR)/test.py
PYTHON_TEST_ENHANCED = $(PYTHON_SCRIPTS_DIR)/test_enhanced.py
PYTHON_TEST_LINUX = $(PYTHON_SCRIPTS_DIR)/test_linux.py
PYTHON_TEST_WINDOWS = $(PYTHON_SCRIPTS_DIR)/test_windows.py

all: $(TARGET)

# Compilar solo la librería
compile-lib: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $(OBJS) $(LDFLAGS)
	@echo "✅ Librería $(TARGET) compilada exitosamente"
	@echo "📋 Verificando símbolos exportados:"
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		nm -D $(TARGET) | grep -E "(alloc_cpu_info|get_cpu|free_cpu_info)" || echo "⚠ Algunos símbolos no encontrados"; \
	fi

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

python-linux: $(TARGET)
	@echo "=== Ejecutando test de Python para Linux (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_linux.py

python-win: $(TARGET)
	@echo "=== Ejecutando test de Python para Windows (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_windows.py

python-all: python python-enhanced python-linux python-win

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
test: $(TARGET) python-check python-default

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
		nm -D $(TARGET) | head -10; \
	fi

clean:
	rm -f $(OBJS) $(TARGET)
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
	@echo "  make python-linux - Test de Python para Linux (10 segundos)"
	@echo "  make python-win   - Test de Python para Windows (10 segundos)"
	@echo "  make python-all   - Ejecutar todos los tests de Python"
	@echo "  make python-default - Auto-detectar plataforma y ejecutar test"
	@echo ""
	@echo "Utilidades:"
	@echo "  make python-deps  - Instalar dependencias de Python"
	@echo "  make python-check - Verificar Python y dependencias"
	@echo "  make test         - Compilar + test Python (detecta plataforma)"
	@echo "  make install      - Instalar librería en el sistema (Linux)"
	@echo "  make uninstall    - Desinstalar librería del sistema (Linux)"
	@echo ""
	@echo "Limpieza:"
	@echo "  make clean        - Limpiar archivos compilados"
	@echo "  make clean-python - Limpiar archivos temporales de Python"
	@echo "  make clean-all    - Limpiar todo"
	@echo "  make help         - Mostrar esta ayuda"

.PHONY: all compile-lib debug verify python python-enhanced python-linux python-win python-all python-default python-deps python-check test install uninstall clean clean-python clean-all help