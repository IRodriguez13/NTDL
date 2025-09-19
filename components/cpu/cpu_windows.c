/**
 * @file cpu_windows.c
 * @brief Implementación de CPU para Windows usando WMI y Performance Counters
 */

#ifdef _WIN32

#include "cpu_interface.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static PDH_HQUERY cpu_query = NULL;
static PDH_HCOUNTER cpu_counter = NULL;
static BOOL cpu_initialized = FALSE;
static char cpu_model_cache[256] = {0};
static int physical_cores_cache = 0;
static int logical_cores_cache = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene información del CPU usando WMI
 */
static int get_cpu_info_wmi(void) {
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *class_object = NULL;
    ULONG returned = 0;
    VARIANT variant;
    
    // Inicializar COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) return -1;
    
    // Crear locator WMI
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID*)&locator
    );
    if (FAILED(hr)) {
        CoUninitialize();
        return -1;
    }
    
    // Conectar al namespace WMI
    hr = locator->lpVtbl->ConnectServer(
        locator,
        L"ROOT\\CIMV2",
        NULL, NULL, 0, NULL, 0, 0,
        &services
    );
    if (FAILED(hr)) {
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }
    
    // Configurar seguridad
    hr = CoSetProxyBlanket(
        (IUnknown*)services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );
    if (FAILED(hr)) {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }
    
    // Ejecutar consulta WQL
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Name, NumberOfCores, NumberOfLogicalProcessors FROM Win32_Processor",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator
    );
    if (FAILED(hr)) {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }
    
    // Obtener primer resultado
    hr = enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned);
    if (returned == 0) {
        enumerator->lpVtbl->Release(enumerator);
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }
    
    // Obtener nombre del CPU
    VariantInit(&variant);
    hr = class_object->lpVtbl->Get(class_object, L"Name", 0, &variant, 0, 0);
    if (SUCCEEDED(hr) && variant.vt == VT_BSTR) {
        WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1, 
                           cpu_model_cache, sizeof(cpu_model_cache), NULL, NULL);
    }
    VariantClear(&variant);
    
    // Obtener número de núcleos físicos
    hr = class_object->lpVtbl->Get(class_object, L"NumberOfCores", 0, &variant, 0, 0);
    if (SUCCEEDED(hr) && variant.vt == VT_I4) {
        physical_cores_cache = variant.lVal;
    }
    VariantClear(&variant);
    
    // Obtener número de núcleos lógicos
    hr = class_object->lpVtbl->Get(class_object, L"NumberOfLogicalProcessors", 0, &variant, 0, 0);
    if (SUCCEEDED(hr) && variant.vt == VT_I4) {
        logical_cores_cache = variant.lVal;
    }
    VariantClear(&variant);
    
    // Limpiar recursos
    class_object->lpVtbl->Release(class_object);
    enumerator->lpVtbl->Release(enumerator);
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();
    
    return 0;
}

/**
 * @brief Inicializa los contadores de rendimiento
 */
static int init_performance_counters(void) {
    PDH_STATUS status;
    
    // Crear consulta PDH
    status = PdhOpenQuery(NULL, 0, &cpu_query);
    if (status != ERROR_SUCCESS) {
        return -1;
    }
    
    // Agregar contador de CPU total
    status = PdhAddCounter(cpu_query, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &cpu_counter);
    if (status != ERROR_SUCCESS) {
        PdhCloseQuery(cpu_query);
        cpu_query = NULL;
        return -1;
    }
    
    // Primera recolección (necesaria para PDH)
    PdhCollectQueryData(cpu_query);
    Sleep(100); // Esperar un poco para obtener datos válidos
    
    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int cpu_init(void) {
    if (cpu_initialized) {
        return 0;
    }
    
    // Obtener información básica del CPU
    if (get_cpu_info_wmi() != 0) {
        return -1;
    }
    
    // Inicializar contadores de rendimiento
    if (init_performance_counters() != 0) {
        return -1;
    }
    
    cpu_initialized = TRUE;
    return 0;
}

void cpu_cleanup(void) {
    if (cpu_query) {
        PdhCloseQuery(cpu_query);
        cpu_query = NULL;
        cpu_counter = NULL;
    }
    cpu_initialized = FALSE;
}

int cpu_get_metrics(CpuMetrics *metrics) {
    if (!metrics || !cpu_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(CpuMetrics));
    
    // Copiar información básica
    strncpy(metrics->model, cpu_model_cache, sizeof(metrics->model) - 1);
    strncpy(metrics->vendor, "Unknown", sizeof(metrics->vendor) - 1);
    
    // Detectar vendor desde el modelo
    if (strstr(cpu_model_cache, "Intel")) {
        strncpy(metrics->vendor, "Intel", sizeof(metrics->vendor) - 1);
    } else if (strstr(cpu_model_cache, "AMD")) {
        strncpy(metrics->vendor, "AMD", sizeof(metrics->vendor) - 1);
    }
    
    metrics->physical_cores = physical_cores_cache;
    metrics->logical_cores = logical_cores_cache;
    metrics->usage_percent = cpu_get_usage_percent();
    metrics->temperature = cpu_get_temperature();
    metrics->frequency_mhz = cpu_get_frequency_mhz();
    
    // Información por núcleo (simplificada para Windows)
    metrics->num_cores = logical_cores_cache;
    for (int i = 0; i < metrics->num_cores && i < MAX_CPU_CORES; i++) {
        metrics->cores[i].core_id = i;
        metrics->cores[i].usage_percent = metrics->usage_percent; // Aproximación
        metrics->cores[i].frequency_mhz = metrics->frequency_mhz;
        metrics->cores[i].temperature = metrics->temperature;
    }
    
    return 0;
}

int cpu_get_model(char *buffer, size_t size) {
    if (!buffer || size == 0 || !cpu_initialized) {
        return -1;
    }
    
    strncpy(buffer, cpu_model_cache, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float cpu_get_usage_percent(void) {
    if (!cpu_initialized || !cpu_query) {
        return -1.0f;
    }
    
    PDH_STATUS status;
    PDH_FMT_COUNTERVALUE counter_value;
    
    // Recolectar datos
    status = PdhCollectQueryData(cpu_query);
    if (status != ERROR_SUCCESS) {
        return -1.0f;
    }
    
    // Obtener valor formateado
    status = PdhGetFormattedCounterValue(cpu_counter, PDH_FMT_DOUBLE, NULL, &counter_value);
    if (status != ERROR_SUCCESS) {
        return -1.0f;
    }
    
    return (float)counter_value.doubleValue;
}

float cpu_get_temperature(void) {
    // Windows no proporciona temperatura de CPU fácilmente
    // Se necesitaría acceso a WMI específico del fabricante o drivers especiales
    return -1.0f;
}

float cpu_get_frequency_mhz(void) {
    HKEY hkey;
    DWORD frequency = 0;
    DWORD size = sizeof(DWORD);
    
    // Intentar obtener frecuencia del registro
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                     0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        
        if (RegQueryValueEx(hkey, TEXT("~MHz"), NULL, NULL, 
                           (LPBYTE)&frequency, &size) == ERROR_SUCCESS) {
            RegCloseKey(hkey);
            return (float)frequency;
        }
        RegCloseKey(hkey);
    }
    
    return -1.0f;
}

int cpu_get_core_info(int core_id, CoreInfo *core_info) {
    if (!core_info || core_id < 0 || core_id >= logical_cores_cache || !cpu_initialized) {
        return -1;
    }
    
    // Información simplificada por núcleo
    core_info->core_id = core_id;
    core_info->usage_percent = cpu_get_usage_percent(); // Aproximación
    core_info->frequency_mhz = cpu_get_frequency_mhz();
    core_info->temperature = cpu_get_temperature();
    
    return 0;
}

int cpu_get_physical_cores(void) {
    return cpu_initialized ? physical_cores_cache : -1;
}

int cpu_get_logical_cores(void) {
    return cpu_initialized ? logical_cores_cache : -1;
}

#endif /* _WIN32 */