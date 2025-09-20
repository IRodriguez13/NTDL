#pragma once
#include <stddef.h>

typedef struct
{
    char name[128]; // nombre del disco (opcional, estático)
    int temperature;
    int power_on_hours;
} DiskInfo;

int get_disk_smart(const char *device, DiskInfo *info);

int get_disk_temperature(const unsigned char *smart_data);

int get_disk_smart_via_smartctl(const char *device, DiskInfo *info);

int get_disk_power_on_hours(const unsigned char *smart_data);
