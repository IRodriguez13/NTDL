CC = gcc
CFLAGS = -Wall -O2 -fPIC      # -fPIC necesario para librerías compartidas en Linux
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
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        SRC_PLATFORM = $(SRC_LINUX)
        TARGET = $(TARGET_LINUX)
        SHARED_FLAG = -shared
    endif
endif

SRCS = $(SRC_COMMON) $(SRC_PLATFORM)
OBJS = $(SRCS:.c=.o)

# Variables para Python
PYTHON = python3
PYTHON_SCRIPTS_DIR = testing
PYTHON_TEST = $(PYTHON_SCRIPTS_DIR)/test.py
PYTHON_TEST_LINUX = $(PYTHON_SCRIPTS_DIR)/test_linux.py
PYTHON_TEST_WINDOWS = $(PYTHON_SCRIPTS_DIR)/test_windows.py

all: $(TARGET)

# Compilar solo la librería
compile-lib: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $(OBJS)
	@echo "✅ Librería $(TARGET) compilada exitosamente"

# regla implícita para .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Comandos de Python
python: $(TARGET)
	@echo "=== Ejecutando test básico de Python ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test.py

python-linux: $(TARGET)
	@echo "=== Ejecutando test de Python para Linux (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_linux.py

python-win: $(TARGET)
	@echo "=== Ejecutando test de Python para Windows (10 segundos) ==="
	cd $(PYTHON_SCRIPTS_DIR) && $(PYTHON) test_windows.py

python-all: python python-linux python-win

# Comando por defecto para Python (detecta automáticamente la plataforma)
python-default:
	@echo "=== Detectando plataforma para test de Python ==="
	@if [ "$(OS)" = "Windows_NT" ]; then \
		echo "Plataforma detectada: Windows"; \
		$(MAKE) python-win; \
	else \
		echo "Plataforma detectada: Linux"; \
		$(MAKE) python-linux; \
	fi

# Instalar dependencias de Python
python-deps:
	@echo "=== Instalando dependencias de Python ==="
	sudo apt update
	sudo apt install -y python3-psutil
	@echo "✅ Dependencias instaladas"

# Verificar que Python y dependencias estén disponibles
python-check:
	@echo "=== Verificando Python y dependencias ==="
	@which $(PYTHON) > /dev/null || (echo "❌ Python3 no encontrado" && exit 1)
	@$(PYTHON) -c "import psutil" 2>/dev/null || (echo "❌ psutil no encontrado. Ejecuta: make python-deps" && exit 1)
	@echo "✅ Python y dependencias OK"

# Test completo: compilar + test Python (detecta plataforma)
test: $(TARGET) python-check python-default

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "✅ Archivos compilados eliminados"

clean-python:
	cd $(PYTHON_SCRIPTS_DIR) && rm -f *.pyc __pycache__/* .pytest_cache/* 2>/dev/null || true
	@echo "✅ Archivos temporales de Python eliminados"

clean-all: clean clean-python

help:
	@echo "Comandos disponibles:"
	@echo "  make              - Compilar la librería"
	@echo "  make compile-lib  - Compilar solo la librería C"
	@echo "  make python       - Test básico de Python"
	@echo "  make python-linux - Test de Python para Linux (10 segundos)"
	@echo "  make python-win   - Test de Python para Windows (10 segundos)"
	@echo "  make python-all   - Ejecutar todos los tests de Python"
	@echo "  make python-deps  - Instalar dependencias de Python"
	@echo "  make python-check - Verificar Python y dependencias"
	@echo "  make test         - Compilar + test Python (detecta plataforma)"
	@echo "  make clean        - Limpiar archivos compilados"
	@echo "  make clean-python - Limpiar archivos temporales de Python"
	@echo "  make clean-all    - Limpiar todo"
	@echo "  make help         - Mostrar esta ayuda"

.PHONY: all compile-lib python python-linux python-win python-all python-default python-deps python-check test clean clean-python clean-all help
