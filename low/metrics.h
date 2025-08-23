#ifndef METRICS_H
#define METRICS_H

typedef struct {
    char os_name[32];
    char cpu_model[128];
    int cpu_cores;
    float cpu_usage_percent;
    unsigned long ram_total_kb;
    unsigned long ram_free_kb;
    char disk_model[128];
    int disk_health;
} SystemStatus;

void collect_metrics(SystemStatus *status);

#endif
