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
	rm -f *.o *.so *.dylib *.dll *.a
	rm -f linux/*.o windows/*.o xnu/*.o
	rm -f tests/*.o tests/example tests/test_simple tests/test_minimal

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

# Ejecutar ejemplo
run: tests/example
	./tests/example

# Ejecutar ejemplos de Python
run-python: tests/example_simple.py
	python3 tests/example_simple.py

# Testing interno con Python
test: $(TARGET)
	@echo "=== Ejecutando tests internos con Python ==="
	python3 tests/example_simple.py

# Compilar ejemplos de test
test-examples: $(TARGET)
	@echo "=== Compilando ejemplos de test ==="
	$(CC) $(CFLAGS) -I. -c tests/example.c -o tests/example.o
	$(CC) tests/example.o -L. -lsysmon -o tests/example
	$(CC) $(CFLAGS) -I. -c tests/test_simple.c -o tests/test_simple.o
	$(CC) tests/test_simple.o -L. -lsysmon -o tests/test_simple
	$(CC) $(CFLAGS) -I. -c tests/test_minimal.c -o tests/test_minimal.o
	$(CC) tests/test_minimal.o -L. -lsysmon -o tests/test_minimal

# Ejecutar todos los tests
run-tests: test-examples
	@echo "=== Ejecutando tests C ==="
	LD_LIBRARY_PATH=. ./tests/test_minimal
	LD_LIBRARY_PATH=. ./tests/test_simple
	LD_LIBRARY_PATH=. ./tests/example
	@echo "=== Ejecutando tests Python ==="
	LD_LIBRARY_PATH=. python3 tests/example_simple.py
	LD_LIBRARY_PATH=. python3 tests/example_ctypes.py
	LD_LIBRARY_PATH=. python3 tests/example_cffi.py

# Información
info:
	@echo "Plataforma detectada: $(PLATFORM)"
	@echo "Archivo objetivo: $(TARGET)"
	@echo "Fuentes: $(SOURCES)"

.PHONY: all clean install uninstall run run-python test info
