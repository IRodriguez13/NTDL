#!/bin/bash

# Script de compilación multiplataforma para SysMon
# Detecta automáticamente la plataforma y compila la librería apropiada

set -e  # Salir en caso de error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para imprimir mensajes coloreados
print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Detectar plataforma
detect_platform() {
    case "$(uname -s)" in
        Linux*)     PLATFORM="linux";;
        Darwin*)    PLATFORM="macos";;
        CYGWIN*|MINGW*|MSYS*) PLATFORM="windows";;
        *)          PLATFORM="unknown";;
    esac
    
    print_info "Plataforma detectada: $PLATFORM"
}

# Configurar variables según plataforma
setup_platform_vars() {
    case $PLATFORM in
        "linux")
            CC="gcc"
            CFLAGS="-Wall -Wextra -O2 -fPIC -g -std=c99 -D_GNU_SOURCE"
            SHARED_FLAG="-shared"
            TARGET="libsysmon.so"
            LDFLAGS="-lX11 -lXrandr"
            SRC_PLATFORM="components/cpu/cpu_linux.c \
                         components/gpu/gpu_linux.c \
                         components/memory/memory_linux.c \
                         components/disk/disk_linux.c \
                         components/network/network_linux.c \
                         components/sensors/sensors_linux.c \
                         components/display/display_linux.c \
                         components/battery/battery_linux.c \
                         components/audio/audio_linux.c \
                         components/advanced/advanced_linux.c"
            ;;
        "macos")
            CC="clang"
            CFLAGS="-Wall -Wextra -O2 -fPIC -g -std=c99"
            SHARED_FLAG="-dynamiclib"
            TARGET="libsysmon.dylib"
            LDFLAGS="-framework CoreFoundation -framework IOKit -framework CoreGraphics -framework CoreAudio -framework DiskArbitration -framework SystemConfiguration"
            SRC_PLATFORM="components/cpu/cpu_macos.c \
                         components/gpu/gpu_macos.c \
                         components/memory/memory_macos.c \
                         components/disk/disk_macos.c \
                         components/network/network_macos.c \
                         components/sensors/sensors_macos.c \
                         components/display/display_macos.c \
                         components/battery/battery_macos.c \
                         components/audio/audio_macos.c \
                         components/advanced/advanced_macos.c"
            ;;
        "windows")
            CC="gcc"  # Asumiendo MinGW
            CFLAGS="-Wall -Wextra -O2 -g -std=c99"
            SHARED_FLAG="-shared"
            TARGET="sysmon.dll"
            LDFLAGS="-ladvapi32 -lole32 -loleaut32 -lwbemuuid -liphlpapi -lpdh -lpowrprof"
            SRC_PLATFORM="components/cpu/cpu_windows.c \
                         components/gpu/gpu_windows.c \
                         components/memory/memory_windows.c \
                         components/disk/disk_windows.c \
                         components/network/network_windows.c \
                         components/sensors/sensors_windows.c \
                         components/display/display_windows.c \
                         components/battery/battery_windows.c \
                         components/audio/audio_windows.c \
                         components/advanced/advanced_windows.c"
            ;;
        *)
            print_error "Plataforma no soportada: $PLATFORM"
            exit 1
            ;;
    esac
}

# Verificar dependencias
check_dependencies() {
    print_info "Verificando dependencias..."
    
    # Verificar compilador
    if ! command -v $CC &> /dev/null; then
        print_error "Compilador $CC no encontrado"
        exit 1
    fi
    print_success "Compilador $CC encontrado"
    
    # Verificar Python3
    if ! command -v python3 &> /dev/null; then
        print_warning "Python3 no encontrado - tests de Python no estarán disponibles"
    else
        print_success "Python3 encontrado"
    fi
    
    # Verificaciones específicas por plataforma
    case $PLATFORM in
        "linux")
            # Verificar librerías de desarrollo X11
            if ! pkg-config --exists x11 2>/dev/null; then
                print_warning "libx11-dev no encontrada - algunas funciones de display pueden fallar"
            fi
            ;;
        "macos")
            # Verificar Xcode command line tools
            if ! xcode-select -p &> /dev/null; then
                print_warning "Xcode command line tools no encontradas"
            fi
            ;;
        "windows")
            # Verificar MinGW
            if ! command -v mingw32-gcc &> /dev/null && ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
                print_warning "MinGW no encontrado - usando gcc del sistema"
            fi
            ;;
    esac
}

# Limpiar archivos anteriores
clean_build() {
    print_info "Limpiando archivos anteriores..."
    
    # Limpiar objetos
    find . -name "*.o" -delete 2>/dev/null || true
    
    # Limpiar librerías anteriores
    rm -f libsysmon.so libsysmon.dylib sysmon.dll 2>/dev/null || true
    
    # Limpiar archivos de testing
    rm -f testing/libsysmon.* testing/sysmon.dll 2>/dev/null || true
    
    print_success "Limpieza completada"
}

# Compilar librería
compile_library() {
    print_info "Compilando librería para $PLATFORM..."
    
    # Código fuente común
    SRC_COMMON="common/common.c core/sysmon_core.c extern_api.c"
    
    # Todos los archivos fuente
    SRCS="$SRC_COMMON $SRC_PLATFORM"
    
    # Comando de compilación
    COMPILE_CMD="$CC $CFLAGS $SHARED_FLAG -o $TARGET $SRCS $LDFLAGS"
    
    print_info "Ejecutando: $COMPILE_CMD"
    
    # Compilar
    if $COMPILE_CMD; then
        print_success "Compilación exitosa: $TARGET"
    else
        print_error "Error en la compilación"
        exit 1
    fi
    
    # Verificar que el archivo se creó
    if [ -f "$TARGET" ]; then
        print_success "Librería creada: $TARGET"
        
        # Mostrar información del archivo
        ls -lh "$TARGET"
        
        # Verificar símbolos (solo en Linux/macOS)
        if [ "$PLATFORM" = "linux" ]; then
            print_info "Verificando símbolos exportados..."
            nm -D "$TARGET" | grep -E "(sysmon_init|sysmon_get_cpu|sysmon_collect)" | head -5 || print_warning "No se pudieron verificar símbolos"
        elif [ "$PLATFORM" = "macos" ]; then
            print_info "Verificando símbolos exportados..."
            nm -D "$TARGET" | grep -E "(sysmon_init|sysmon_get_cpu|sysmon_collect)" | head -5 || print_warning "No se pudieron verificar símbolos"
        fi
        
    else
        print_error "La librería no se creó correctamente"
        exit 1
    fi
}

# Copiar librería al directorio de testing
copy_to_testing() {
    print_info "Copiando librería al directorio de testing..."
    
    if [ -d "testing" ]; then
        cp "$TARGET" testing/
        print_success "Librería copiada a testing/$TARGET"
    else
        print_warning "Directorio testing no encontrado"
    fi
}

# Ejecutar test básico
run_basic_test() {
    print_info "Ejecutando test básico..."
    
    if [ -f "testing/test_multiplatform.py" ] && command -v python3 &> /dev/null; then
        cd testing
        if python3 test_multiplatform.py; then
            print_success "Test básico pasó"
        else
            print_warning "Test básico falló - pero la librería se compiló"
        fi
        cd ..
    else
        print_warning "Test de Python no disponible"
    fi
}

# Mostrar información final
show_final_info() {
    echo
    echo "🎉 COMPILACIÓN COMPLETADA"
    echo "=========================="
    echo "Plataforma: $PLATFORM"
    echo "Librería: $TARGET"
    echo "Compilador: $CC"
    echo
    echo "📋 Próximos pasos:"
    echo "1. Ejecutar tests: cd testing && python3 test_multiplatform.py"
    echo "2. Usar en tu aplicación: ctypes.CDLL('./$TARGET')"
    echo "3. Ver documentación: cat README.md"
    echo
    
    # Mostrar comandos específicos por plataforma
    case $PLATFORM in
        "linux")
            echo "🐧 Linux específico:"
            echo "   make python-enhanced  # Test completo"
            echo "   make install          # Instalar en sistema"
            ;;
        "macos")
            echo "🍎 macOS específico:"
            echo "   ./testing/test_multiplatform.py"
            echo "   otool -L $TARGET      # Verificar dependencias"
            ;;
        "windows")
            echo "🪟 Windows específico:"
            echo "   python testing/test_multiplatform.py"
            echo "   ldd $TARGET           # Verificar dependencias (si disponible)"
            ;;
    esac
}

# Función principal
main() {
    echo "🔨 SysMon - Compilador Multiplataforma"
    echo "======================================"
    
    # Detectar plataforma
    detect_platform
    
    # Configurar variables
    setup_platform_vars
    
    # Verificar dependencias
    check_dependencies
    
    # Limpiar build anterior
    clean_build
    
    # Compilar
    compile_library
    
    # Copiar a testing
    copy_to_testing
    
    # Test básico
    run_basic_test
    
    # Información final
    show_final_info
}

# Ejecutar si es llamado directamente
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    main "$@"
fi