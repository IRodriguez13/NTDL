#pragma once 
#include "../includes/hard_info.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "../common/common.h"

// Funciones básicas (iguales que Linux)
int get_cpu_model(char *buffer, size_t size);
double get_cpu_usage();

// Función adicional específica de Windows
int get_cpu_info_windows(char *vendor, char *model, int *cores, double *frequency);

