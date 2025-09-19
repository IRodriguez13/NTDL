/**
 * @file gpu_windows.c
 * @brief Implementación de GPU para Windows usando DirectX y WMI
 */

#ifdef _WIN32

#include "gpu_interface.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL gpu_initialized = FALSE;
static GpuInfo gpu_list[MAX_GPUS];
static int gpu_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Enumera GPUs usando DXGI
 */
static int enumerate_gpus_dxgi(void) {
    HRESULT hr;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC adapter_desc;
    UINT adapter_index = 0;
    
    // Crear factory DXGI
    hr = CreateDXGIFactory(&IID_IDXGIFactory, (void**)&factory);
    if (FAILED(hr)) {
        return -1;
    }
    
    gpu_count = 0;
    
    // Enumerar adaptadores
    while (SUCCEEDED(factory->lpVtbl->EnumAdapters(factory, adapter_index, &adapter)) && 
           gpu_count < MAX_GPUS) {
        
        // Obtener descripción del adaptador
        hr = adapter->lpVtbl->GetDesc(adapter, &adapter_desc);
        if (SUCCEEDED(hr)) {
            GpuInfo *gpu = &gpu_list[gpu_count];
            
            // Convertir nombre de wide char a char
            WideCharToMultiByte(CP_UTF8, 0, adapter_desc.Description, -1,
                               gpu->name, sizeof(gpu->name), NULL, NULL);
            
            // Detectar vendor por ID
            switch (adapter_desc.VendorId) {
                case 0x10DE: // NVIDIA
                    strncpy(gpu->vendor, "NVIDIA", sizeof(gpu->vendor) - 1);
                    break;
                case 0x1002: // AMD
                    strncpy(gpu->vendor, "AMD", sizeof(gpu->vendor) - 1);
                    break;
                case 0x8086: // Intel
                    strncpy(gpu->vendor, "Intel", sizeof(gpu->vendor) - 1);
                    break;
                default:
                    strncpy(gpu->vendor, "Unknown", sizeof(gpu->vendor) - 1);
                    break;
            }
            
            // Información básica
            gpu->memory_total_mb = (float)(adapter_desc.DedicatedVideoMemory / (1024 * 1024));
            gpu->memory_used_mb = 0.0f; // No disponible fácilmente en DXGI
            gpu->memory_free_mb = gpu->memory_total_mb;
            gpu->usage_percent = 0.0f; // Requiere APIs específicas
            gpu->temperature = -1.0f; // No disponible en DXGI
            gpu->fan_speed_percent = -1.0f;
            gpu->power_usage_watts = -1.0f;
            gpu->clock_core_mhz = -1.0f;
            gpu->clock_memory_mhz = -1.0f;
            
            gpu_count++;
        }
        
        adapter->lpVtbl->Release(adapter);
        adapter = NULL;
        adapter_index++;
    }
    
    factory->lpVtbl->Release(factory);
    return gpu_count > 0 ? 0 : -1;
}

/**
 * @brief Obtiene información adicional de GPU usando WMI
 */
static int get_gpu_info_wmi(void) {
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *class_object = NULL;
    ULONG returned = 0;
    VARIANT variant;
    int wmi_gpu_index = 0;
    
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
    
    // Ejecutar consulta WQL para tarjetas de video
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Name, AdapterRAM, DriverVersion FROM Win32_VideoController",
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
    
    // Procesar resultados
    while (SUCCEEDED(enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned)) &&
           returned != 0 && wmi_gpu_index < gpu_count) {
        
        GpuInfo *gpu = &gpu_list[wmi_gpu_index];
        
        // Obtener versión del driver
        VariantInit(&variant);
        hr = class_object->lpVtbl->Get(class_object, L"DriverVersion", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_BSTR) {
            WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                               gpu->driver_version, sizeof(gpu->driver_version), NULL, NULL);
        }
        VariantClear(&variant);
        
        // Verificar memoria de adaptador (puede ser más precisa que DXGI)
        hr = class_object->lpVtbl->Get(class_object, L"AdapterRAM", 0, &variant, 0, 0);
        if (SUCCEEDED(hr) && variant.vt == VT_I4 && variant.lVal > 0) {
            gpu->memory_total_mb = (float)(variant.lVal / (1024 * 1024));
            gpu->memory_free_mb = gpu->memory_total_mb;
        }
        VariantClear(&variant);
        
        class_object->lpVtbl->Release(class_object);
        class_object = NULL;
        wmi_gpu_index++;
    }
    
    // Limpiar recursos
    if (class_object) class_object->lpVtbl->Release(class_object);
    enumerator->lpVtbl->Release(enumerator);
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();
    
    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int gpu_init(void) {
    if (gpu_initialized) {
        return 0;
    }
    
    // Limpiar lista de GPUs
    memset(gpu_list, 0, sizeof(gpu_list));
    gpu_count = 0;
    
    // Enumerar GPUs usando DXGI
    if (enumerate_gpus_dxgi() != 0) {
        return -1;
    }
    
    // Obtener información adicional usando WMI
    get_gpu_info_wmi();
    
    gpu_initialized = TRUE;
    return 0;
}

void gpu_cleanup(void) {
    gpu_initialized = FALSE;
    gpu_count = 0;
}

int gpu_get_count(void) {
    return gpu_initialized ? gpu_count : -1;
}

int gpu_get_info(int gpu_id, GpuInfo *info) {
    if (!info || !gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1;
    }
    
    memcpy(info, &gpu_list[gpu_id], sizeof(GpuInfo));
    return 0;
}

int gpu_get_metrics(GpuMetrics *metrics) {
    if (!metrics || !gpu_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(GpuMetrics));
    
    metrics->num_gpus = gpu_count;
    
    // Copiar información de todas las GPUs
    for (int i = 0; i < gpu_count && i < MAX_GPUS; i++) {
        memcpy(&metrics->gpus[i], &gpu_list[i], sizeof(GpuInfo));
    }
    
    return 0;
}

float gpu_get_usage_percent(int gpu_id) {
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    // En Windows, obtener el uso de GPU requiere APIs específicas como
    // Performance Counters o bibliotecas del fabricante (NVML, ADL)
    // Por simplicidad, retornamos el valor cached
    return gpu_list[gpu_id].usage_percent;
}

float gpu_get_temperature(int gpu_id) {
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    // La temperatura de GPU en Windows requiere APIs específicas del fabricante
    return gpu_list[gpu_id].temperature;
}

float gpu_get_memory_usage_percent(int gpu_id) {
    if (!gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1.0f;
    }
    
    GpuInfo *gpu = &gpu_list[gpu_id];
    if (gpu->memory_total_mb <= 0) {
        return -1.0f;
    }
    
    return (gpu->memory_used_mb / gpu->memory_total_mb) * 100.0f;
}

int gpu_get_name(int gpu_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !gpu_initialized || gpu_id < 0 || gpu_id >= gpu_count) {
        return -1;
    }
    
    strncpy(buffer, gpu_list[gpu_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

#endif /* _WIN32 */