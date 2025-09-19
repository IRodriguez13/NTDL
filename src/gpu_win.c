// src/gpu_windows.c
#include "gpu_windows.h"
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "wbemuuid.lib")

// Función para obtener información de GPU usando WMI
int get_gpu_info_wmi(GpuInfo *info)
{
    HRESULT hres;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject *pEnumerator = NULL;
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;

    // Inicializar COM
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        return -1;
    }

    // Configurar seguridad COM
    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL);

    if (FAILED(hres) && hres != RPC_E_TOO_LATE)
    {
        CoUninitialize();
        return -1;
    }

    // Obtener localizador WMI
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return -1;
    }

    // Conectar al namespace WMI
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc);

    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return -1;
    }

    // Configurar proxy de seguridad
    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -1;
    }

    // Consultar información de video controller
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -1;
    }

    // Obtener datos del primer adaptador de video
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

    if (uReturn == 0)
    {
        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -1;
    }

    VARIANT vtProp;

    // Obtener nombre de la GPU
    hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
    {
        wcstombs(info->name, vtProp.bstrVal, sizeof(info->name) - 1);
        info->name[sizeof(info->name) - 1] = '\0';
    }
    VariantClear(&vtProp);

    // Obtener fabricante
    hr = pclsObj->Get(L"AdapterCompatibility", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
    {
        wcstombs(info->vendor, vtProp.bstrVal, sizeof(info->vendor) - 1);
        info->vendor[sizeof(info->vendor) - 1] = '\0';
    }
    else
    {
        // Determinar vendor por el nombre
        if (strstr(info->name, "NVIDIA") || strstr(info->name, "GeForce") || strstr(info->name, "RTX") || strstr(info->name, "GTX"))
        {
            strcpy(info->vendor, "NVIDIA");
        }
        else if (strstr(info->name, "AMD") || strstr(info->name, "Radeon") || strstr(info->name, "RX"))
        {
            strcpy(info->vendor, "AMD");
        }
        else if (strstr(info->name, "Intel") || strstr(info->name, "UHD") || strstr(info->name, "Iris"))
        {
            strcpy(info->vendor, "Intel");
        }
        else
        {
            strcpy(info->vendor, "Unknown");
        }
    }
    VariantClear(&vtProp);

    // Obtener versión del driver
    hr = pclsObj->Get(L"DriverVersion", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
    {
        wcstombs(info->driver_version, vtProp.bstrVal, sizeof(info->driver_version) - 1);
        info->driver_version[sizeof(info->driver_version) - 1] = '\0';
    }
    VariantClear(&vtProp);

    // Obtener memoria de video (en bytes, convertir a MB)
    hr = pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
    {
        info->memory_total_mb = vtProp.lVal / (1024 * 1024);
    }
    VariantClear(&vtProp);

    // Limpiar objetos COM
    pclsObj->Release();
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return 0;
}

// Función para detectar GPU NVIDIA usando NVIDIA-ML API (si está disponible)
int detect_nvidia_gpu_nvml(GpuInfo *info)
{
    // Esta función requeriría la librería NVIDIA-ML
    // Por simplicidad, usaremos la implementación WMI
    return get_gpu_info_wmi(info);
}

// Función principal para obtener información de GPU en Windows
GpuInfo *alloc_gpu_info()
{
    GpuInfo *info = (GpuInfo *)malloc(sizeof(GpuInfo));
    if (!info)
    {
        fprintf(stderr, "Error: malloc failed for GpuInfo\n");
        return NULL;
    }

    // Inicializar estructura
    memset(info, 0, sizeof(GpuInfo));
    info->memory_total_mb = -1;
    info->memory_used_mb = -1;
    info->temperature = -1;
    info->utilization_percent = -1.0f;
    info->core_clock_mhz = -1;
    info->memory_clock_mhz = -1;

    // Intentar obtener información usando WMI
    if (get_gpu_info_wmi(info) == 0)
    {
        printf("DEBUG: GPU detectada usando WMI\n");
        return info;
    }

    // Si WMI falla, usar valores por defecto
    strcpy(info->name, "Unknown GPU");
    strcpy(info->vendor, "Unknown");
    strcpy(info->driver_version, "Unknown");

    return info;
}

// Función para liberar memoria de GPU info
void free_gpu_info(GpuInfo *info)
{
    if (info)
    {
        free(info);
    }
}

// Función para imprimir información de GPU
void print_gpu_info(const GpuInfo *info)
{
    if (!info)
        return;

    printf("\n==== GPU Info ====\n");
    printf("Name:         %s\n", info->name);
    printf("Vendor:       %s\n", info->vendor);
    printf("Driver:       %s\n", info->driver_version);

    if (info->memory_total_mb > 0)
    {
        printf("VRAM Total:   %d MB\n", info->memory_total_mb);
    }
    if (info->memory_used_mb >= 0)
    {
        printf("VRAM Used:    %d MB\n", info->memory_used_mb);
    }
    if (info->temperature > 0)
    {
        printf("Temperature:  %d°C\n", info->temperature);
    }
    if (info->utilization_percent >= 0)
    {
        printf("Utilization:  %.1f%%\n", info->utilization_percent);
    }
    if (info->core_clock_mhz > 0)
    {
        printf("Core Clock:   %d MHz\n", info->core_clock_mhz);
    }
    if (info->memory_clock_mhz > 0)
    {
        printf("Memory Clock: %d MHz\n", info->memory_clock_mhz);
    }

    printf("==================\n");
}