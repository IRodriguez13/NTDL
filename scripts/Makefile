# Makefile para tests de Python de SysMon
# Autor: SysMon Team
# Versión: 1.0.0

.PHONY: test test-enhanced clean help

# Variables
PYTHON = python3
TEST_SCRIPT = test.py
TEST_ENHANCED_SCRIPT = test_python_enhanced.py

# Test básico
test:
	@echo "=== Ejecutando test básico de Python ==="
	$(PYTHON) $(TEST_SCRIPT)

# Test mejorado (10 segundos)
test-enhanced:
	@echo "=== Ejecutando test mejorado de Python (10 segundos) ==="
	$(PYTHON) $(TEST_ENHANCED_SCRIPT)

# Ejecutar ambos tests
all: test test-enhanced

# Limpiar archivos temporales
clean:
	@echo "Limpiando archivos temporales..."
	rm -f *.pyc __pycache__/* .pytest_cache/*

# Mostrar ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make test          - Ejecutar test básico"
	@echo "  make test-enhanced - Ejecutar test mejorado (10 segundos)"
	@echo "  make all           - Ejecutar ambos tests"
	@echo "  make clean         - Limpiar archivos temporales"
	@echo "  make help          - Mostrar esta ayuda"

# Comando por defecto
.DEFAULT_GOAL := test-enhanced
