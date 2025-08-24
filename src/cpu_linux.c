#include "cpu_linux.h"

// CPU modelo: lo sacamos de /proc/cpuinfo
int get_cpu_model(char *buffer, size_t size)
{

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f == NULL)
    {
        perror("NULL pointer");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "model name", 10) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                snprintf(buffer, size, "%s", colon + 2); // skip ": "
                buffer[strcspn(buffer, "\n")] = 0;       // quitar salto
                fclose(f);
                return 0;
            }
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
        perror("NULL pointer");
        return -1.0;
    }

    char cpu[5];
    long user, nice, system, idle, iowait, irq, softirq, steal;

    if (fscanf(f, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
               cpu, &user, &nice, &system, &idle,
               &iowait, &irq, &softirq, &steal) == 9)
    {
        perror("ERR en los datos de la cpu");
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
        perror("ERR en el cálculo de porcentaje. División por cero.");
        return 0.0;
    }

    return (double)(diffTotal - diffIdle) / diffTotal * 100.0; // porcentaje de uso de la cpu
}
