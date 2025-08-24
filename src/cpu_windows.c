#include "cpu_windows.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// CPU modelo: obtenemos usando WMI o Registry
int get_cpu_model(char *buffer, size_t size)
{
    HKEY hKey;
    char cpuName[256] = {0};
    DWORD dataSize = sizeof(cpuName);
    
    // Intentar obtener desde el registro de Windows
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, 
                            (LPBYTE)cpuName, &dataSize) == ERROR_SUCCESS)
        {
            // Limpiar el nombre del procesador
            char *newline = strchr(cpuName, '\n');
            if (newline) *newline = '\0';
            
            char *carriage = strchr(cpuName, '\r');
            if (carriage) *carriage = '\0';
            
            // Copiar al buffer proporcionado
            if (strlen(cpuName) < size) {
                strcpy(buffer, cpuName);
                RegCloseKey(hKey);
                return 0;
            }
        }
        RegCloseKey(hKey);
    }
    
    // Fallback: usar GetSystemInfo para obtener información básica
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    const char* archNames[] = {
        "Unknown", "x86", "x64", "ARM", "ARM64", "IA64", "Unknown"
    };
    
    snprintf(buffer, size, "Windows CPU (%s)", 
             archNames[sysInfo.wProcessorArchitecture]);
    
    return 0;
}

// CPU uso: usando Performance Counters de Windows
double get_cpu_usage()
{
    static ULARGE_INTEGER prevIdleTime = {0};
    static ULARGE_INTEGER prevKernelTime = {0};
    static ULARGE_INTEGER prevUserTime = {0};
    static int firstCall = 1;
    
    FILETIME idleTime, kernelTime, userTime;
    
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return -1.0;
    }
    
    ULARGE_INTEGER currentIdleTime, currentKernelTime, currentUserTime;
    currentIdleTime.LowPart = idleTime.dwLowDateTime;
    currentIdleTime.HighPart = idleTime.dwHighDateTime;
    currentKernelTime.LowPart = kernelTime.dwLowDateTime;
    currentKernelTime.HighPart = kernelTime.dwHighDateTime;
    currentUserTime.LowPart = userTime.dwLowDateTime;
    currentUserTime.HighPart = userTime.dwHighDateTime;
    
    if (firstCall) {
        prevIdleTime = currentIdleTime;
        prevKernelTime = currentKernelTime;
        prevUserTime = currentUserTime;
        firstCall = 0;
        return 0.0; // Primera llamada, no hay datos para comparar
    }
    
    // Calcular diferencias
    ULARGE_INTEGER diffIdle, diffKernel, diffUser;
    diffIdle.QuadPart = currentIdleTime.QuadPart - prevIdleTime.QuadPart;
    diffKernel.QuadPart = currentKernelTime.QuadPart - prevKernelTime.QuadPart;
    diffUser.QuadPart = currentUserTime.QuadPart - prevUserTime.QuadPart;
    
    // Tiempo total del sistema
    ULARGE_INTEGER totalSystem;
    totalSystem.QuadPart = diffKernel.QuadPart + diffUser.QuadPart;
    
    // Actualizar valores previos
    prevIdleTime = currentIdleTime;
    prevKernelTime = currentKernelTime;
    prevUserTime = currentUserTime;
    
    if (totalSystem.QuadPart == 0) {
        return 0.0;
    }
    
    // Calcular porcentaje de uso
    double cpuUsage = (double)(totalSystem.QuadPart - diffIdle.QuadPart) / 
                      totalSystem.QuadPart * 100.0;
    
    return cpuUsage;
}

// Función adicional para obtener información del CPU usando WMI (opcional)
int get_cpu_info_windows(char *vendor, char *model, int *cores, double *frequency)
{
    HKEY hKey;
    char cpuName[256] = {0};
    char cpuVendor[256] = {0};
    DWORD dataSize = sizeof(cpuName);
    DWORD vendorSize = sizeof(cpuVendor);
    
    // Obtener nombre del procesador
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Obtener nombre
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, 
                            (LPBYTE)cpuName, &dataSize) == ERROR_SUCCESS)
        {
            if (model) {
                char *newline = strchr(cpuName, '\n');
                if (newline) *newline = '\0';
                strcpy(model, cpuName);
            }
        }
        
        // Obtener vendor (si está disponible)
        if (RegQueryValueExA(hKey, "VendorIdentifier", NULL, NULL, 
                            (LPBYTE)cpuVendor, &vendorSize) == ERROR_SUCCESS)
        {
            if (vendor) {
                strcpy(vendor, cpuVendor);
            }
        }
        
        RegCloseKey(hKey);
    }
    
    // Obtener número de núcleos
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    if (cores) {
        *cores = sysInfo.dwNumberOfProcessors;
    }
    
    // Obtener frecuencia (aproximada)
    if (frequency) {
        // Windows no proporciona frecuencia en tiempo real fácilmente
        // Usamos una aproximación basada en la arquitectura
        switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                *frequency = 2000.0; // 2 GHz aproximado
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                *frequency = 1800.0; // 1.8 GHz aproximado
                break;
            default:
                *frequency = 1500.0; // 1.5 GHz por defecto
                break;
        }
    }
    
    return 0;
}
