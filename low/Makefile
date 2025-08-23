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
all: $(TARGET) example

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

example: example.o $(TARGET)
	$(CC) example.o -L. -lsysmon -o example

# Reglas de compilación
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -f *.o *.so *.dylib *.dll *.a example
	rm -f linux/*.o windows/*.o xnu/*.o

# Instalar (solo para Linux/macOS)
install: $(TARGET)
ifeq ($(UNAME_S),Linux)
	sudo cp $(TARGET) /usr/local/lib/
	sudo cp sysmon.h /usr/local/include/
	sudo ldconfig
endif
ifeq ($(UNAME_S),Darwin)
	sudo cp $(TARGET) /usr/local/lib/
	sudo cp sysmon.h /usr/local/include/
endif

# Desinstalar
uninstall:
ifeq ($(UNAME_S),Linux)
	sudo rm -f /usr/local/lib/$(TARGET)
	sudo rm -f /usr/local/include/sysmon.h
	sudo ldconfig
endif
ifeq ($(UNAME_S),Darwin)
	sudo rm -f /usr/local/lib/$(TARGET)
	sudo rm -f /usr/local/include/sysmon.h
endif

# Ejecutar ejemplo
run: example
	./example

# Ejecutar ejemplos de Python
run-python: example_simple.py
	python3 example_simple.py

# Testing interno con Python
test: $(TARGET)
	@echo "=== Ejecutando tests internos con Python ==="
	python3 example_simple.py

# Información
info:
	@echo "Plataforma detectada: $(PLATFORM)"
	@echo "Archivo objetivo: $(TARGET)"
	@echo "Fuentes: $(SOURCES)"

.PHONY: all clean install uninstall run run-python test info
