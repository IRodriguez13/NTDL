// src/gpu_linux.h
#pragma once

#include "../common/common.h"
#include <stddef.h>

// Estructura para información de GPU
typedef struct
{
    char name[128];            // Nombre del modelo de GPU
    char vendor[64];           // Fabricante (NVIDIA, AMD, Intel)
    char driver_version[64];   // Versión del driver
    int memory_total_mb;       // VRAM total en MB (-1 si no disponible)
    int memory_used_mb;        // VRAM usada en MB (-1 si no disponible)
    int temperature;           // Temperatura en °C (-1 si no disponible)
    float utilization_percent; // Utilización GPU en % (-1 si no disponible)
    int core_clock_mhz;        // Frecuencia del núcleo en MHz (-1 si no disponible)
    int memory_clock_mhz;      // Frecuencia de memoria en MHz (-1 si no disponible)
} GpuInfo;

// Funciones principales
GpuInfo *alloc_gpu_info();
void free_gpu_info(GpuInfo *info);
void print_gpu_info(const GpuInfo *info);

// Funciones de detección específicas
int detect_nvidia_gpu(GpuInfo *info);
int detect_amd_gpu(GpuInfo *info);
int detect_intel_gpu(GpuInfo *info);