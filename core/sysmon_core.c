/**
 * @file sysmon_core.c
 * @brief Implementación del núcleo central del sistema de monitoreo
 */

#include "sysmon_core.h"
#include "../common/common.h"
#include "../components/advanced/advanced_interface.h"
#include "../components/audio/audio_interface.h"
#include "../components/battery/battery_interface.h"
#include "../components/cpu/cpu_interface.h"
#include "../components/disk/disk_interface.h"
#include "../components/display/display_interface.h"
#include "../components/gpu/gpu_interface.h"
#include "../components/memory/memory_interface.h"
#include "../components/network/network_interface.h"
#include "../components/sensors/sensors_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Includes específicos por plataforma
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/utsname.h>
#include <unistd.h>
#endif

/* ============================================================================
 * CONSTANTES Y VARIABLES GLOBALES
 * ============================================================================
 */

#define SYSMON_VERSION "1.0.0"

static int core_initialized = 0;
static uint64_t total_collections = 0;
static double last_collection_time_ms = 0.0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================
 */

static double get_time_ms(void)
{
#ifdef _WIN32
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return (double)(counter.QuadPart * 1000.0) / frequency.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
#endif
}

static int get_system_load_average(float *load1, float *load5, float *load15)
{
#ifdef _WIN32
  // Windows no tiene load average, usar aproximación con CPU
  *load1 = 0.0f;
  *load5 = 0.0f;
  *load15 = 0.0f;
  return 0;
#elif defined(__APPLE__)
  double loadavg[3];
  if (getloadavg(loadavg, 3) == -1)
  {
    return -1;
  }
  *load1 = (float)loadavg[0];
  *load5 = (float)loadavg[1];
  *load15 = (float)loadavg[2];
  return 0;
#else

  FILE *f = fopen("/proc/loadavg", "r");

  if (!f)
    return -1;

  if (fscanf(f, "%f %f %f", load1, load5, load15) != 3)
  {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
#endif
}

static int get_process_counts(SystemStatus *status)
{
#ifdef _WIN32
  // Windows: usar Performance API
  PERFORMANCE_INFORMATION perf_info;
  perf_info.cb = sizeof(PERFORMANCE_INFORMATION);

  if (GetPerformanceInfo(&perf_info, sizeof(PERFORMANCE_INFORMATION)))
  {
    status->total_processes = perf_info.ProcessCount;
    status->running_processes = perf_info.ProcessCount / 4; // Estimación
    status->sleeping_processes = perf_info.ProcessCount - status->running_processes;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
  }
  else
  {
    // Valores por defecto
    status->total_processes = 100;
    status->running_processes = 10;
    status->sleeping_processes = 90;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
  }
  return 0;

#elif defined(__APPLE__)
  // macOS: usar sysctl
  int mib[4];
  size_t size;

  // Obtener número de procesos
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_ALL;
  mib[3] = 0;

  if (sysctl(mib, 4, NULL, &size, NULL, 0) == 0)
  {
    status->total_processes = size / sizeof(struct kinfo_proc);
    status->running_processes = status->total_processes / 10; // Estimación
    status->sleeping_processes = status->total_processes - status->running_processes;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
  }
  else
  {
    // Valores por defecto
    status->total_processes = 150;
    status->running_processes = 15;
    status->sleeping_processes = 135;
    status->stopped_processes = 0;
    status->zombie_processes = 0;
  }
  return 0;

#else
  // Linux: usar /proc/stat
  FILE *f = fopen("/proc/stat", "r");
  if (!f)
    return -1;

  char line[256];
  while (fgets(line, sizeof(line), f))
  {
    if (strncmp(line, "processes", 9) == 0)
    {
      // Total de procesos creados (no es lo que queremos)
      continue;
    }
    if (strncmp(line, "procs_running", 13) == 0)
    {
      sscanf(line, "procs_running %d", &status->running_processes);
    }
    if (strncmp(line, "procs_blocked", 13) == 0)
    {
      sscanf(line, "procs_blocked %d", &status->stopped_processes);
    }
  }
  fclose(f);

  // Contar procesos desde /proc
  status->total_processes = 0;
  status->sleeping_processes = 0;
  status->zombie_processes = 0;

  FILE *ps = popen("ps -eo stat --no-headers 2>/dev/null", "r");
  if (ps)
  {
    char stat[16];
    while (fscanf(ps, "%15s", stat) == 1)
    {
      status->total_processes++;

      switch (stat[0])
      {
      case 'S': // Sleeping
      case 'I': // Idle
        status->sleeping_processes++;
        break;
      case 'Z': // Zombie
        status->zombie_processes++;
        break;
        // R (Running) ya se cuenta en running_processes
        // T (Stopped) ya se cuenta en stopped_processes
      }
    }
    pclose(ps);
  }

  return 0;
#endif
}

static int get_uptime_info(SystemStatus *status)
{
  FILE *f = fopen("/proc/uptime", "r");
  if (!f)
    return -1;

  double uptime, idle_time;
  if (fscanf(f, "%lf %lf", &uptime, &idle_time) == 2)
  {
    status->uptime_seconds = (uint64_t)uptime;
    status->idle_time_seconds = (uint64_t)idle_time;
  }

  fclose(f);
  return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================
 */

int sysmon_core_init(void)
{
  if (core_initialized)
    return 0;

  // Inicializar todos los módulos
  if (cpu_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize CPU module\n");
  }

  if (memory_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize memory module\n");
  }

  if (gpu_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize GPU module\n");
  }

  if (disk_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize disk module\n");
  }

  if (network_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize network module\n");
  }

  if (sensors_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize sensors module\n");
  }

  if (display_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize display module\n");
  }

  if (battery_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize battery module (normal if "
                    "no battery)\n");
  }

  if (audio_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize audio module\n");
  }

  if (advanced_sensors_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize advanced sensors module\n");
  }

  if (advanced_memory_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize advanced memory module\n");
  }

  if (advanced_network_init() != 0)
  {
    fprintf(stderr, "Warning: Failed to initialize advanced network module\n");
  }

  core_initialized = 1;
  total_collections = 0;
  last_collection_time_ms = 0.0;

  return 0;
}

void sysmon_core_cleanup(void)
{
  if (!core_initialized)
    return;

  // Limpiar todos los módulos
  cpu_cleanup();
  memory_cleanup();
  gpu_cleanup();
  disk_cleanup();
  network_cleanup();
  sensors_cleanup();
  display_cleanup();
  battery_cleanup();
  audio_cleanup();
  advanced_sensors_cleanup();
  advanced_memory_cleanup();
  advanced_network_cleanup();

  core_initialized = 0;
}

int sysmon_core_collect_all_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  if (!core_initialized)
  {
    if (sysmon_core_init() != 0)
    {
      return -1;
    }
  }

  double start_time = get_time_ms();

  // Inicializar estructura
  memset(status, 0, sizeof(SystemStatus));

  // Timestamp
  status->timestamp = time(NULL);

  // Información básica del sistema
  sysmon_core_get_system_info(status);

  // Métricas de CPU
  sysmon_core_get_cpu_metrics(status);

  // Métricas de memoria
  sysmon_core_get_memory_metrics(status);

  // Métricas de GPU
  sysmon_core_get_gpu_metrics(status);

  // Métricas de discos
  sysmon_core_get_disk_metrics(status);

  // Métricas de red
  sysmon_core_get_network_metrics(status);

  // Métricas de sensores
  sysmon_core_get_sensor_metrics(status);

  // Métricas de pantallas
  sysmon_core_get_display_metrics(status);

  // Métricas de batería
  sysmon_core_get_battery_metrics(status);

  // Métricas de audio
  sysmon_core_get_audio_metrics(status);

  // Métricas avanzadas
  sysmon_core_get_advanced_metrics(status);

  // Información de carga del sistema
  get_system_load_average(&status->load_average_1min,
                          &status->load_average_5min,
                          &status->load_average_15min);

  // Convertir carga a porcentaje (basado en número de núcleos)
  int logical_cores = cpu_get_logical_cores();
  if (logical_cores > 0)
  {
    status->load_1min_percent =
        (status->load_average_1min / logical_cores) * 100.0f;
    status->load_5min_percent =
        (status->load_average_5min / logical_cores) * 100.0f;
    status->load_15min_percent =
        (status->load_average_15min / logical_cores) * 100.0f;
  }

  // Información de procesos
  get_process_counts(status);

  // Información de tiempo de actividad
  get_uptime_info(status);

  // Campos de compatibilidad con la API anterior
  status->cpu_usage_percent = status->cpu.usage_percent;
  status->ram_usage_percent = status->ram.usage_percent;

  strncpy(status->cpu_model, status->cpu.model, sizeof(status->cpu_model) - 1);
  status->cpu_model[sizeof(status->cpu_model) - 1] = '\0';

  strncpy(status->cpu_vendor, status->cpu.vendor,
          sizeof(status->cpu_vendor) - 1);
  status->cpu_vendor[sizeof(status->cpu_vendor) - 1] = '\0';

  status->cpu_cores = status->cpu.physical_cores;
  status->cpu_frequency_mhz = status->cpu.current_frequency_mhz;
  status->ram_total_gb = status->ram.total_kb / (1024.0f * 1024.0f);
  status->ram_available_gb = status->ram.available_kb / (1024.0f * 1024.0f);
  status->ram_used_gb = status->ram.used_kb / (1024.0f * 1024.0f);

  // Estadísticas de rendimiento
  double end_time = get_time_ms();
  last_collection_time_ms = end_time - start_time;
  total_collections++;

  return 0;
}

int sysmon_core_update_dynamic_metrics(SystemStatus *status)
{
  if (!status || !core_initialized)
    return -1;

  double start_time = get_time_ms();

  // Actualizar timestamp
  status->timestamp = time(NULL);

  // Actualizar métricas que cambian frecuentemente
  sysmon_core_get_cpu_metrics(status);
  sysmon_core_get_memory_metrics(status);
  sysmon_core_get_network_metrics(status);

  // Actualizar carga del sistema
  get_system_load_average(&status->load_average_1min,
                          &status->load_average_5min,
                          &status->load_average_15min);

  // Actualizar campos de compatibilidad
  status->cpu_usage_percent = status->cpu.usage_percent;
  status->ram_usage_percent = status->ram.usage_percent;
  status->cpu_frequency_mhz = status->cpu.current_frequency_mhz;

  // Estadísticas de rendimiento
  double end_time = get_time_ms();
  last_collection_time_ms = end_time - start_time;
  total_collections++;

  return 0;
}

int sysmon_core_get_system_info(SystemStatus *status)
{
  if (!status)
    return -1;

  struct utsname uname_info;
  if (uname(&uname_info) == 0)
  {
    strncpy(status->os_name, uname_info.sysname, sizeof(status->os_name) - 1);
    status->os_name[sizeof(status->os_name) - 1] = '\0';

    strncpy(status->os_version, uname_info.release,
            sizeof(status->os_version) - 1);
    status->os_version[sizeof(status->os_version) - 1] = '\0';

    strncpy(status->kernel_version, uname_info.version,
            sizeof(status->kernel_version) - 1);
    status->kernel_version[sizeof(status->kernel_version) - 1] = '\0';

    strncpy(status->hostname, uname_info.nodename,
            sizeof(status->hostname) - 1);
    status->hostname[sizeof(status->hostname) - 1] = '\0';

    strncpy(status->architecture, uname_info.machine,
            sizeof(status->architecture) - 1);
    status->architecture[sizeof(status->architecture) - 1] = '\0';
  }

  return 0;
}

int sysmon_core_get_cpu_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  return cpu_get_metrics(&status->cpu);
}

int sysmon_core_get_memory_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  int ret1 = memory_get_ram_info(&status->ram);
  int ret2 = memory_get_swap_info(&status->swap);

  return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}

int sysmon_core_get_gpu_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_gpus = gpu_get_count();

  for (int i = 0; i < status->num_gpus && i < MAX_GPUS; i++)
  {
    if (gpu_get_metrics(i, &status->gpus[i]) != 0)
    {
      status->num_gpus = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_disk_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_disks = disk_get_count();

  for (int i = 0; i < status->num_disks && i < MAX_DISKS; i++)
  {
    if (disk_get_info(i, &status->disks[i]) != 0)
    {
      status->num_disks = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_network_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_network_interfaces = network_get_interface_count();

  for (int i = 0;
       i < status->num_network_interfaces && i < MAX_NETWORK_INTERFACES; i++)
  {
    if (network_get_interface_info(i, &status->network_interfaces[i]) != 0)
    {
      status->num_network_interfaces = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_sensor_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_sensors = sensors_get_count();

  for (int i = 0; i < status->num_sensors && i < MAX_SENSORS; i++)
  {
    if (sensors_get_info(i, &status->sensors[i]) != 0)
    {
      status->num_sensors = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_display_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_displays = display_get_count();

  for (int i = 0; i < status->num_displays && i < MAX_DISPLAYS; i++)
  {
    if (display_get_info(i, &status->displays[i]) != 0)
    {
      status->num_displays = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_battery_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  // Intentar obtener información de batería
  if (battery_get_info(&status->battery) != 0)
  {
    // No hay batería o error - inicializar con valores por defecto
    memset(&status->battery, 0, sizeof(BatteryInfo));
    status->battery.is_present = 0;
    status->battery.percent = -1.0f;
    status->battery.voltage = -1.0f;
    status->battery.current_ma = -1.0f;
    status->battery.power_w = -1.0f;
    status->battery.time_remaining_minutes = -1;
    status->battery.cycle_count = -1;
    status->battery.design_capacity_wh = -1.0f;
    status->battery.full_charge_capacity_wh = -1.0f;
    status->battery.wear_level_percent = -1.0f;
  }

  return 0;
}

int sysmon_core_get_audio_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  status->num_audio_devices = audio_get_device_count();

  for (int i = 0; i < status->num_audio_devices && i < MAX_AUDIO_DEVICES; i++)
  {
    if (audio_get_device_info(i, &status->audio_devices[i]) != 0)
    {
      status->num_audio_devices = i; // Ajustar el conteo si hay error
      break;
    }
  }

  return 0;
}

int sysmon_core_get_advanced_metrics(SystemStatus *status)
{
  if (!status)
    return -1;

  // Obtener sensores avanzados
  advanced_sensors_get_info(&status->advanced_sensors);

  // Obtener información avanzada de memoria
  advanced_memory_get_info(&status->memory_advanced);

  // Obtener información avanzada de red
  advanced_network_get_info(&status->network_advanced);

  return 0;
}

int sysmon_core_is_initialized(void) { return core_initialized; }

const char *sysmon_core_get_version(void) { return SYSMON_VERSION; }

int sysmon_core_get_performance_stats(double *collection_time_ms,
                                      uint64_t *total_collections_out)
{
  if (collection_time_ms)
  {
    *collection_time_ms = last_collection_time_ms;
  }

  if (total_collections_out)
  {
    *total_collections_out = total_collections;
  }

  return 0;
}