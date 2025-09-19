#include "cpu_linux.h"

// CPU modelo: lo sacamos de /proc/cpuinfo
int get_cpu_model(char *buffer, size_t size)
{

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f == NULL)
    {
        fprintf(stderr, "No se pudo abrir /proc/cpuinfo\n");   
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        int ret = strncmp(line, "model name", 10);
        if(ret == 0) 
        {
            fprintf(stderr, "No se pudo encontrar el modelo de CPU\n");
            return -1.0;
        }
        char *colon = strchr(line, ':');
        if (colon)
        {
            snprintf(buffer, size, "%s", colon + 2); // skip ": "
            buffer[strcspn(buffer, "\n")] = 0;       // quitar salto
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return -1;
}

// CPU uso: calculamos diferencia de tiempos de /proc/stat
double get_cpu_usage()
{
    static long prevIdle = 0, prevTotal = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (f == NULL)
    {
        fprintf(stderr, "No se pudo abrir /proc/stat\n");
        return -1.0;
    }

    char cpu[128];
    long user, nice, system, idle, iowait, irq, softirq, steal;

    int ret = fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
                     cpu, &user, &nice, &system, &idle,
                     &iowait, &irq, &softirq, &steal);

    if (ret != 9)
    {
        fprintf(stderr, "No se pudo leer /proc/stat\n");
        fclose(f);
        return -1.0;
    }
    fclose(f);

    long idleAll = idle + iowait;
    long total = user + nice + system + idle + iowait + irq + softirq + steal;

    long diffIdle = idleAll - prevIdle;
    long diffTotal = total - prevTotal;

    prevIdle = idleAll;
    prevTotal = total;

    if (diffTotal == 0)
    {
        fprintf(stderr, "No se pudo calcular el uso de la CPU\n");
        return 0.0;
    }

    return (double)(diffTotal - diffIdle) / diffTotal * 100.0; // porcentaje de uso de la cpu
}
