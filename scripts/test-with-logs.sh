#!/bin/bash

# ============================================================================
# SysMon Advanced Testing with Detailed Logging
# ============================================================================

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuración
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_DIR="logs/$TIMESTAMP"
DOCKER_LOG_DIR="docker/logs"

# Crear directorio de logs primero
mkdir -p "$LOG_DIR"
mkdir -p "$DOCKER_LOG_DIR"

# Función para logging
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_DIR/test.log"
}

success() {
    echo -e "${GREEN}✅ $1${NC}" | tee -a "$LOG_DIR/test.log"
}

error() {
    echo -e "${RED}❌ $1${NC}" | tee -a "$LOG_DIR/test.log"
}

warning() {
    echo -e "${YELLOW}⚠️  $1${NC}" | tee -a "$LOG_DIR/test.log"
}

info() {
    echo -e "${CYAN}ℹ️  $1${NC}" | tee -a "$LOG_DIR/test.log"
}

# Función para mostrar ayuda
show_help() {
    echo "SysMon Advanced Testing with Detailed Logging"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -p, --platform PLATFORM Test specific platform (linux, windows, macos, all)"
    echo "  -t, --test TYPE         Test type (build, unit, integration, performance, security, all)"
    echo "  -d, --docker            Use Docker for testing"
    echo "  -l, --local             Test on local machine"
    echo "  -v, --verbose           Verbose output"
    echo "  -c, --clean             Clean before building"
    echo "  -r, --report            Generate detailed report"
    echo ""
    echo "Examples:"
    echo "  $0 -p all -t all -d     # Test all platforms with Docker"
    echo "  $0 -p linux -t all -l   # Test Linux locally"
    echo "  $0 -d -r                # Docker testing with report"
}

# Variables por defecto
PLATFORM="all"
TEST_TYPE="all"
USE_DOCKER=false
USE_LOCAL=false
VERBOSE=false
CLEAN=false
GENERATE_REPORT=false

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -p|--platform)
            PLATFORM="$2"
            shift 2
            ;;
        -t|--test)
            TEST_TYPE="$2"
            shift 2
            ;;
        -d|--docker)
            USE_DOCKER=true
            shift
            ;;
        -l|--local)
            USE_LOCAL=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -r|--report)
            GENERATE_REPORT=true
            shift
            ;;
        *)
            echo -e "${RED}❌ Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Función para obtener información del sistema
get_system_info() {
    log "=== System Information ==="
    log "OS: $(uname -s)"
    log "Architecture: $(uname -m)"
    log "Kernel: $(uname -r)"
    log "Hostname: $(hostname)"
    log "User: $(whoami)"
    log "Working Directory: $(pwd)"
    log "Timestamp: $TIMESTAMP"
    log ""
}

# Función para testear localmente
test_local() {
    local platform=$1
    local test_type=$2
    
    log "=== Local Testing: $platform - $test_type ==="
    
    cd low
    
    case $test_type in
        build)
            log "Building for $platform..."
            if [ "$CLEAN" = true ]; then
                make clean
            fi
            make 2>&1 | tee -a "$LOG_DIR/${platform}-build.log"
            success "Build completed for $platform"
            ;;
        unit)
            log "Running unit tests for $platform..."
            make test-examples 2>&1 | tee -a "$LOG_DIR/${platform}-unit.log"
            LD_LIBRARY_PATH=. ./tests/test_minimal 2>&1 | tee -a "$LOG_DIR/${platform}-minimal.log"
            LD_LIBRARY_PATH=. ./tests/example 2>&1 | tee -a "$LOG_DIR/${platform}-example.log"
            success "Unit tests completed for $platform"
            ;;
        python)
            log "Running Python tests for $platform..."
            LD_LIBRARY_PATH=. python3 tests/example_simple.py 2>&1 | tee -a "$LOG_DIR/${platform}-python-simple.log"
            LD_LIBRARY_PATH=. python3 tests/example_ctypes.py 2>&1 | tee -a "$LOG_DIR/${platform}-python-ctypes.log"
            LD_LIBRARY_PATH=. python3 tests/example_cffi.py 2>&1 | tee -a "$LOG_DIR/${platform}-python-cffi.log"
            success "Python tests completed for $platform"
            ;;
        performance)
            log "Running performance tests for $platform..."
            { time LD_LIBRARY_PATH=. ./tests/example; } 2>&1 | tee -a "$LOG_DIR/${platform}-performance.log"
            success "Performance tests completed for $platform"
            ;;
        security)
            log "Running security tests for $platform..."
            if command -v cppcheck &> /dev/null; then
                cppcheck --enable=all *.c linux/*.c windows/*.c xnu/*.c 2>&1 | tee -a "$LOG_DIR/${platform}-security-cppcheck.log"
            fi
            if command -v valgrind &> /dev/null; then
                valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file="$LOG_DIR/${platform}-security-valgrind.log" LD_LIBRARY_PATH=. ./tests/example
            fi
            success "Security tests completed for $platform"
            ;;
        integration)
            log "Running integration tests for $platform..."
            LD_LIBRARY_PATH=. ./tests/example > "$LOG_DIR/${platform}-integration-output.log" 2>&1
            success "Integration tests completed for $platform"
            ;;
        all)
            test_local $platform "build"
            test_local $platform "unit"
            test_local $platform "python"
            test_local $platform "performance"
            test_local $platform "security"
            test_local $platform "integration"
            ;;
    esac
    
    cd ..
}

# Función para testear con Docker
test_docker() {
    local platform=$1
    local test_type=$2
    
    log "=== Docker Testing: $platform - $test_type ==="
    
    if ! command -v docker &> /dev/null; then
        error "Docker not found"
        return 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        error "Docker Compose not found"
        return 1
    fi
    
    cd docker
    
    case $platform in
        linux)
            log "Testing Linux with Docker..."
            docker-compose up test-linux 2>&1 | tee -a "$LOG_DIR/docker-linux.log"
            ;;
        windows)
            log "Testing Windows with Docker..."
            docker-compose up test-windows 2>&1 | tee -a "$LOG_DIR/docker-windows.log"
            ;;
        macos)
            log "Testing macOS with Docker..."
            docker-compose up test-macos 2>&1 | tee -a "$LOG_DIR/docker-macos.log"
            ;;
        all)
            log "Testing all platforms with Docker..."
            docker-compose up test-linux test-windows test-macos 2>&1 | tee -a "$LOG_DIR/docker-all.log"
            ;;
    esac
    
    # Copiar logs de Docker
    if [ -d "logs" ]; then
        cp -r logs/* "$LOG_DIR/"
    fi
    
    cd ..
    success "Docker testing completed for $platform"
}

# Función para generar reporte
generate_report() {
    log "=== Generating Test Report ==="
    
    local report_file="$LOG_DIR/test-report.md"
    
    cat > "$report_file" << EOF
# SysMon Test Report

**Generated:** $(date)
**Timestamp:** $TIMESTAMP
**Platform:** $PLATFORM
**Test Type:** $TEST_TYPE
**Docker:** $USE_DOCKER
**Local:** $USE_LOCAL

## System Information

- **OS:** $(uname -s)
- **Architecture:** $(uname -m)
- **Kernel:** $(uname -r)
- **Hostname:** $(hostname)

## Test Results

EOF
    
    # Agregar resultados de cada plataforma
    for platform in linux windows macos; do
        if [ "$PLATFORM" = "all" ] || [ "$PLATFORM" = "$platform" ]; then
            echo "### $platform" >> "$report_file"
            
            # Verificar logs de build
            if [ -f "$LOG_DIR/${platform}-build.log" ]; then
                if grep -q "error\|Error\|ERROR" "$LOG_DIR/${platform}-build.log"; then
                    echo "- ❌ **Build:** Failed" >> "$report_file"
                else
                    echo "- ✅ **Build:** Success" >> "$report_file"
                fi
            else
                echo "- ⚠️ **Build:** No log found" >> "$report_file"
            fi
            
            # Verificar logs de unit tests
            if [ -f "$LOG_DIR/${platform}-unit.log" ]; then
                if grep -q "error\|Error\|ERROR\|FAIL" "$LOG_DIR/${platform}-unit.log"; then
                    echo "- ❌ **Unit Tests:** Failed" >> "$report_file"
                else
                    echo "- ✅ **Unit Tests:** Success" >> "$report_file"
                fi
            else
                echo "- ⚠️ **Unit Tests:** No log found" >> "$report_file"
            fi
            
            # Verificar logs de Python
            if [ -f "$LOG_DIR/${platform}-python-simple.log" ]; then
                if grep -q "error\|Error\|ERROR\|Exception" "$LOG_DIR/${platform}-python-simple.log"; then
                    echo "- ❌ **Python Tests:** Failed" >> "$report_file"
                else
                    echo "- ✅ **Python Tests:** Success" >> "$report_file"
                fi
            else
                echo "- ⚠️ **Python Tests:** No log found" >> "$report_file"
            fi
            
            echo "" >> "$report_file"
        fi
    done
    
    # Agregar resumen de Docker
    if [ "$USE_DOCKER" = true ]; then
        echo "## Docker Results" >> "$report_file"
        if [ -f "$LOG_DIR/docker-all.log" ]; then
            echo "- Docker testing completed" >> "$report_file"
        fi
        echo "" >> "$report_file"
    fi
    
    # Agregar archivos de log
    echo "## Log Files" >> "$report_file"
    echo "" >> "$report_file"
    for log_file in "$LOG_DIR"/*.log; do
        if [ -f "$log_file" ]; then
            echo "- \`$(basename "$log_file")\`" >> "$report_file"
        fi
    done
    
    success "Report generated: $report_file"
}

# Función principal
main() {
    log "=== SysMon Advanced Testing Started ==="
    get_system_info
    
    log "Configuration:"
    log "  Platform: $PLATFORM"
    log "  Test Type: $TEST_TYPE"
    log "  Docker: $USE_DOCKER"
    log "  Local: $USE_LOCAL"
    log "  Verbose: $VERBOSE"
    log "  Clean: $CLEAN"
    log "  Generate Report: $GENERATE_REPORT"
    log ""
    
    # Ejecutar tests
    if [ "$USE_DOCKER" = true ]; then
        test_docker "$PLATFORM" "$TEST_TYPE"
    fi
    
    if [ "$USE_LOCAL" = true ]; then
        case $PLATFORM in
            linux|all)
                test_local "linux" "$TEST_TYPE"
                ;;
            windows|all)
                test_local "windows" "$TEST_TYPE"
                ;;
            macos|all)
                test_local "macos" "$TEST_TYPE"
                ;;
        esac
    fi
    
    # Generar reporte si se solicita
    if [ "$GENERATE_REPORT" = true ]; then
        generate_report
    fi
    
    log "=== Testing Completed ==="
    success "All tests completed! Logs available in: $LOG_DIR"
    
    if [ "$GENERATE_REPORT" = true ]; then
        info "Report available: $LOG_DIR/test-report.md"
    fi
}

# Ejecutar función principal
main "$@"
