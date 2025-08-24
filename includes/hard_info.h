#pragma once
#include "../common/common.h"

#ifdef __cplusplus

extern "C" 
{
#endif

#include <stddef.h>

// Obtiene el modelo de CPU en un buffer
int get_cpu_model(char* buffer, size_t size);

// Obtiene uso actual de CPU (dinámico)
double get_cpu_usage();

#ifdef __cplusplus
}

#endif


