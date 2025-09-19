#pragma once
#include <stddef.h>
#define CPU_MODEL_LEN 128
#define DISK_MODEL_LEN 128
#define Kerror(msg) perror(msg)

typedef struct
{
    char *vendor;
    char *model;
    int cores;
    double mhz;
} CpuInfo;

// funciones de manejo de struct
void print_cpu_info(const CpuInfo *info);
CpuInfo *alloc_cpu_info();
void free_cpu_info(CpuInfo *info);  // Función faltante agregada

// funciones que deben implementar Linux/Windows
int get_cpu_model(char *buffer, size_t size);
double get_cpu_usage();

// Funciones adicionales específicas de Windows (si están disponibles)
#ifdef _WIN32
int get_cpu_info_windows(char *vendor, char *model, int *cores, double *frequency);
#endif

