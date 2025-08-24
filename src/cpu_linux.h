#pragma once
#include "../includes/hard_info.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../common/common.h"


int get_cpu_model(char *buffer, size_t size);

double get_cpu_usage();