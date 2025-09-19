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
        // BUG CORREGIDO: Cambiar la lógica del strncmp
        // Antes: if(ret == 0) -> esto era incorrecto
        // Ahora: if(ret != 0) -> saltar si NO es "model name"
        if (strncmp(line, "model name", 10) != 0)
        {
            continue; // Saltar líneas que no sean "model name"
        }
        
        // Si llegamos aquí, encontramos "model name"
        char *colon = strchr(line, ':');
        if (colon)
        {
            snprintf(buffer, size, "%s", colon + 2); // skip ": "
            buffer[strcspn(buffer, "\n")] = 0;       // quitar salto
            fclose(f);
            return 0; // Éxito
        }
    }

    fclose(f);
    fprintf(stderr, "No se pudo encontrar el modelo de CPU\n");
    return -1;
}

// CPU uso: calculamos diferencia de tiempos de /proc/stat
double get_cpu_usage()
{
    static long prevIdle = 0, prevTotal = 0;
    static int firstCall = 1;

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

    fclose(f);

    if (ret != 9)
    {
        fprintf(stderr, "No se pudo leer /proc/stat correctamente (ret=%d)\n", ret);
        return -1.0;
    }

    long idleAll = idle + iowait;
    long total = user + nice + system + idle + iowait + irq + softirq + steal;

    // En la primera llamada, solo guardamos los valores iniciales
    if (firstCall)
    {
        prevIdle = idleAll;
        prevTotal = total;
        firstCall = 0;
        return 0.0; // Primera llamada siempre retorna 0%
    }

    long diffIdle = idleAll - prevIdle;
    long diffTotal = total - prevTotal;

    prevIdle = idleAll;
    prevTotal = total;

    if (diffTotal == 0)
    {
        fprintf(stderr, "diffTotal es cero, no se puede calcular CPU usage\n");
        return 0.0;
    }

    double cpu_usage = (double)(diffTotal - diffIdle) / diffTotal * 100.0;
    
    // Validar que el resultado esté en rango válido
    if (cpu_usage < 0.0) cpu_usage = 0.0;
    if (cpu_usage > 100.0) cpu_usage = 100.0;
    
    return cpu_usage;
}