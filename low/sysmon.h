#ifndef SYSMON_H
#define SYSMON_H

#include "metrics.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inicializar el sistema de monitoreo
int sysmon_init(void);

// Obtener métricas del sistema
int sysmon_get_metrics(SystemStatus *status);

// Obtener métricas específicas
float sysmon_get_cpu_usage(void);
unsigned long sysmon_get_ram_total(void);
unsigned long sysmon_get_ram_free(void);
unsigned long sysmon_get_ram_used(void);
float sysmon_get_ram_usage_percent(void);
int sysmon_get_cpu_cores(void);
const char* sysmon_get_cpu_model(void);
const char* sysmon_get_os_name(void);
const char* sysmon_get_disk_model(void);
int sysmon_get_disk_health(void);



// Limpiar recursos
void sysmon_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // SYSMON_H
