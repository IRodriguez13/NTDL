// Definiciones para uso con cffi desde Python
// Este archivo contiene todas las estructuras y funciones expuestas por sysmon

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

// Funciones de inicialización
int sysmon_init(void);
void sysmon_cleanup(void);

// Funciones de métricas generales
int sysmon_get_metrics(SystemStatus *status);

// Funciones de métricas específicas
float sysmon_get_cpu_usage(void);
int sysmon_get_cpu_cores(void);
const char* sysmon_get_cpu_model(void);
unsigned long sysmon_get_ram_total(void);
unsigned long sysmon_get_ram_free(void);
unsigned long sysmon_get_ram_used(void);
float sysmon_get_ram_usage_percent(void);
const char* sysmon_get_os_name(void);
const char* sysmon_get_disk_model(void);
int sysmon_get_disk_health(void);


