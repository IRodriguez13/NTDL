#pragma once
#include <stddef.h>
#define CPU_MODEL_LEN 128
#define DISK_MODEL_LEN 128

// common.h o disk_linux.h

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
void free_cpu_info(CpuInfo *info);

// funciones que deben implementar Linux/Windows
int get_cpu_model(char *buffer, size_t size);
double get_cpu_usage();