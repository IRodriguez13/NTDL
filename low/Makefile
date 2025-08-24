CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC
LDFLAGS = -shared

# Detectar el sistema operativo
UNAME_S := $(shell uname -s)

# Configuración específica por plataforma
ifeq ($(UNAME_S),Linux)
    PLATFORM = linux
    SOURCES = sysmon.c linux/metrics-linux.c
    TARGET = libsysmon.so
    CFLAGS += -DLINUX
endif

ifeq ($(UNAME_S),Darwin)
    PLATFORM = xnu
    SOURCES = sysmon.c xnu/metrics-xnu.c
    TARGET = libsysmon.dylib
    CFLAGS += -DXNU
    LDFLAGS += -framework IOKit -framework CoreFoundation
endif

ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    SOURCES = sysmon.c windows/metrics-win.c
    TARGET = sysmon.dll
    CFLAGS += -DWINDOWS
    LDFLAGS = -shared -Wl,--out-implib,libsysmon.a
endif

# Objetos
OBJECTS = $(SOURCES:.c=.o)

# Reglas principales
all: $(TARGET)

# Inicializar y probar la librería
init: $(TARGET)
	@echo "=== Compilando test de inicialización ==="
	$(CC) $(CFLAGS) -I. -c test_init.c -o test_init.o
	$(CC) test_init.o -L. -lsysmon -o test_init
	@echo "=== Ejecutando test de inicialización ==="
	LD_LIBRARY_PATH=. ./test_init
	@echo "=== Limpiando archivos temporales ==="
	rm -f test_init.o test_init

# Test de Python (10 segundos)
python: $(TARGET)
	@echo "=== Ejecutando test de Python (10 segundos) ==="
	python3 test_python.py

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Reglas de compilación
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -f *.o *.so *.dylib *.dll *.a
	rm -f linux/*.o windows/*.o xnu/*.o

# Instalar (solo para Linux/macOS)
install: $(TARGET)
ifeq ($(UNAME_S),Linux)
	sudo cp $(TARGET) /usr/local/lib/
	sudo cp sysmon.h /usr/local/include/
	sudo cp extern_api.h /usr/local/include/
	sudo ldconfig
endif
ifeq ($(UNAME_S),Darwin)
	sudo cp $(TARGET) /usr/local/lib/
	sudo cp sysmon.h /usr/local/include/
	sudo cp extern_api.h /usr/local/include/
endif

# Desinstalar
uninstall:
ifeq ($(UNAME_S),Linux)
	sudo rm -f /usr/local/lib/$(TARGET)
	sudo rm -f /usr/local/include/sysmon.h
	sudo rm -f /usr/local/include/extern_api.h
	sudo ldconfig
endif
ifeq ($(UNAME_S),Darwin)
	sudo rm -f /usr/local/lib/$(TARGET)
	sudo rm -f /usr/local/include/sysmon.h
	sudo rm -f /usr/local/include/extern_api.h
endif

# Información del proyecto
info:
	@echo "=== Información del Proyecto ==="
	@echo "Plataforma detectada: $(PLATFORM)"
	@echo "Archivo objetivo: $(TARGET)"
	@echo "Fuentes: $(SOURCES)"
	@echo ""
	@echo "Para usar la librería:"
	@echo "  make          - Compilar la librería"
	@echo "  make init     - Compilar, probar y limpiar"
	@echo "  make python   - Test de Python (10 segundos)"
	@echo "  make clean    - Limpiar archivos compilados"
	@echo "  make install  - Instalar en el sistema"
	@echo "  make info     - Mostrar esta información"

.PHONY: all init python clean install uninstall info
