#include "../metrics.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <mach/vm_statistics.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>

// Variables globales para calcular uso de CPU
static natural_t last_cpu_count = 0;
static processor_cpu_load_info_t last_cpu_info = NULL;
static mach_msg_type_number_t last_num_cpu_info = 0;

// Declaraciones de funciones auxiliares
void get_xnu_version(SystemStatus *status);
void get_cpu_detailed_info_xnu(SystemStatus *status);
void get_memory_detailed_info_xnu(SystemStatus *status);
void get_disk_info_xnu(SystemStatus *status);
void get_network_info_xnu(SystemStatus *status);
void get_gpu_info_xnu(SystemStatus *status);
void get_sensors_info_xnu(SystemStatus *status);
void get_motherboard_info_xnu(SystemStatus *status);
void get_system_load_xnu(SystemStatus *status);
void get_process_info_xnu(SystemStatus *status);
void get_uptime_info_xnu(SystemStatus *status);
void get_power_info_xnu(SystemStatus *status);

float get_cpu_usage_xnu()
{
    kern_return_t kr;
    natural_t num_cpu_info = 0;
    processor_info_array_t cpu_info = NULL;
    mach_msg_type_number_t num_cpu_info_cnt = 0;

    kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO,
                             &num_cpu_info, &cpu_info, &num_cpu_info_cnt);
    if (kr != KERN_SUCCESS)
    {
        return 0.0f;
    }

    float total_cpu_usage = 0.0f;
    processor_cpu_load_info_t cpu_load_info = (processor_cpu_load_info_t)cpu_info;

    for (natural_t i = 0; i < num_cpu_info; i++)
    {
        if (last_cpu_info != NULL)
        {
            unsigned long total = cpu_load_info[i].cpu_ticks[CPU_STATE_USER] +
                                  cpu_load_info[i].cpu_ticks[CPU_STATE_SYSTEM] +
                                  cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] +
                                  cpu_load_info[i].cpu_ticks[CPU_STATE_NICE];

            unsigned long last_total = last_cpu_info[i].cpu_ticks[CPU_STATE_USER] +
                                       last_cpu_info[i].cpu_ticks[CPU_STATE_SYSTEM] +
                                       last_cpu_info[i].cpu_ticks[CPU_STATE_IDLE] +
                                       last_cpu_info[i].cpu_ticks[CPU_STATE_NICE];

            unsigned long total_diff = total - last_total;
            unsigned long idle_diff = cpu_load_info[i].cpu_ticks[CPU_STATE_IDLE] -
                                      last_cpu_info[i].cpu_ticks[CPU_STATE_IDLE];

            if (total_diff > 0)
            {
                float cpu_usage = 100.0f * (1.0f - ((float)idle_diff / total_diff));
                total_cpu_usage += cpu_usage;
            }
        }
    }

    // Liberar información anterior
    if (last_cpu_info != NULL)
    {
        vm_deallocate(mach_task_self(), (vm_address_t)last_cpu_info,
                      last_num_cpu_info * sizeof(*last_cpu_info));
    }

    // Guardar información actual para la próxima llamada
    last_cpu_info = cpu_load_info;
    last_num_cpu_info = num_cpu_info_cnt;
    last_cpu_count = num_cpu_info;

    return (num_cpu_info > 0) ? total_cpu_usage / num_cpu_info : 0.0f;
}

void collect_metrics(SystemStatus *status)
{
    // Información básica del sistema
    strcpy(status->os_name, "macOS");

    // Obtener versión de macOS
    get_xnu_version(status);

    // Obtener hostname
    gethostname(status->hostname, sizeof(status->hostname));

    // Información detallada de CPU
    get_cpu_detailed_info_xnu(status);

    // Información detallada de memoria
    get_memory_detailed_info_xnu(status);

    // Información de discos
    get_disk_info_xnu(status);

    // Información de red
    get_network_info_xnu(status);

    // Información de GPU
    get_gpu_info_xnu(status);

    // Información de sensores
    get_sensors_info_xnu(status);

    // Información de motherboard
    get_motherboard_info_xnu(status);

    // Información de carga del sistema
    get_system_load_xnu(status);

    // Información de procesos
    get_process_info_xnu(status);

    // Información de energía
    get_power_info_xnu(status);

    // Timestamp y tiempo de actividad
    status->timestamp = time(NULL);
    get_uptime_info_xnu(status);
}

void get_xnu_version(SystemStatus *status)
{
    struct utsname uts;
    if (uname(&uts) == 0)
    {
        strncpy(status->kernel_version, uts.release, sizeof(status->kernel_version) - 1);
    }
    else
    {
        strcpy(status->kernel_version, "Unknown");
    }

    // Obtener versión de macOS
    char version[64];
    size_t size = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &size, NULL, 0) == 0)
    {
        strncpy(status->os_version, version, sizeof(status->os_version) - 1);
    }
    else
    {
        strcpy(status->os_version, "Unknown");
    }
}

void get_cpu_detailed_info_xnu(SystemStatus *status)
{
    // Obtener número de cores
    int mib[2] = {CTL_HW, HW_NCPU};
    size_t len = sizeof(status->cpu_cores);
    if (sysctl(mib, 2, &status->cpu_cores, &len, NULL, 0) != 0)
    {
        status->cpu_cores = 1;
    }

    // Obtener threads (en macOS es igual a cores para simplificar)
    status->cpu_threads = status->cpu_cores;

    // Obtener modelo de CPU
    char brand_string[128];
    size_t brand_size = sizeof(brand_string);
    if (sysctlbyname("machdep.cpu.brand_string", brand_string, &brand_size, NULL, 0) == 0)
    {
        strncpy(status->cpu_model, brand_string, sizeof(status->cpu_model) - 1);
    }
    else
    {
        strcpy(status->cpu_model, "Unknown CPU");
    }

    // Obtener uso de CPU
    status->cpu_usage_percent = get_cpu_usage_xnu();

    // Obtener frecuencia
    uint64_t freq_hz;
    size_t freq_size = sizeof(freq_hz);
    if (sysctlbyname("hw.cpufrequency", &freq_hz, &freq_size, NULL, 0) == 0)
    {
        status->cpu_frequency = (float)(freq_hz / 1000000); // Convertir a MHz
    }
    else
    {
        status->cpu_frequency = -1.0f;
    }

    // Información por núcleo (simplificada)
    status->num_cores = status->cpu_cores;
    for (int i = 0; i < status->cpu_cores && i < MAX_CORES; i++)
    {
        status->cores[i].core_id = i;
        status->cores[i].temperature = -1.0f; // Requiere IOKit específico
        status->cores[i].frequency = status->cpu_frequency;
        status->cores[i].voltage = -1.0f;                           // Requiere acceso a sensores específicos
        status->cores[i].usage_percent = status->cpu_usage_percent; // Simplificado
        status->cores[i].max_frequency = status->cpu_frequency;
        status->cores[i].min_frequency = status->cpu_frequency;
        status->cores[i].load = 0;
    }

    status->cpu_temperature = -1.0f;
}

void get_memory_detailed_info_xnu(SystemStatus *status)
{
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t info_count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64,
                      (host_info64_t)&vm_stats, &info_count);

    // Obtener tamaño de página
    vm_size_t page_size;
    host_page_size(mach_host_self(), &page_size);

    // Calcular memoria total y libre
    status->ram_total_kb = (vm_stats.active_count + vm_stats.inactive_count +
                            vm_stats.wire_count + vm_stats.free_count) *
                           page_size / 1024;
    status->ram_free_kb = vm_stats.free_count * page_size / 1024;
    status->ram_used_kb = status->ram_total_kb - status->ram_free_kb;
    status->ram_usage_percent = (float)status->ram_used_kb / status->ram_total_kb * 100.0f;

    // Información detallada de RAM
    strcpy(status->detailed_ram.type, "Unknown");
    status->detailed_ram.frequency_mhz = 0;
    status->detailed_ram.num_sticks = 0;
    status->detailed_ram.total_kb = status->ram_total_kb;
    status->detailed_ram.used_kb = status->ram_used_kb;
    status->detailed_ram.free_kb = status->ram_free_kb;
    status->detailed_ram.cached_kb = vm_stats.inactive_count * page_size / 1024;
    status->detailed_ram.swap_total_kb = 0; // Requiere APIs específicas
    status->detailed_ram.swap_used_kb = 0;

    // Información de caché (simplificada)
    status->detailed_ram.l1_cache_kb = 0;
    status->detailed_ram.l2_cache_kb = 0;
    status->detailed_ram.l3_cache_kb = 0;
}

void get_disk_info_xnu(SystemStatus *status)
{
    status->num_disks = 0;

    // Obtener discos usando IOKit
    io_registry_entry_t disk = IO_OBJECT_NULL;
    io_iterator_t iterator = IO_OBJECT_NULL;

    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                    IOServiceMatching("IOMedia"),
                                                    &iterator);
    if (kr == KERN_SUCCESS)
    {
        while ((disk = IOIteratorNext(iterator)) != IO_OBJECT_NULL && status->num_disks < MAX_DISKS)
        {
            CFBooleanRef is_whole = IORegistryEntryCreateCFProperty(disk,
                                                                    CFSTR(kIOMediaWholeKey),
                                                                    kCFAllocatorDefault, 0);
            if (is_whole && CFBooleanGetValue(is_whole))
            {
                DiskInfo *disk_info = &status->disks[status->num_disks];

                // Obtener nombre del disco
                CFStringRef name_cf = IORegistryEntryCreateCFProperty(disk,
                                                                      CFSTR(kIOBSDNameKey),
                                                                      kCFAllocatorDefault, 0);
                if (name_cf)
                {
                    CFStringGetCString(name_cf, disk_info->name, sizeof(disk_info->name), kCFStringEncodingUTF8);
                    snprintf(disk_info->device_path, sizeof(disk_info->device_path), "/dev/%s", disk_info->name);
                    CFRelease(name_cf);
                }
                else
                {
                    snprintf(disk_info->name, sizeof(disk_info->name), "disk%d", status->num_disks);
                    snprintf(disk_info->device_path, sizeof(disk_info->device_path), "/dev/disk%d", status->num_disks);
                }

                // Obtener modelo
                CFStringRef model_cf = IORegistryEntryCreateCFProperty(disk,
                                                                       CFSTR("IOMediaModel"),
                                                                       kCFAllocatorDefault, 0);
                if (model_cf)
                {
                    CFStringGetCString(model_cf, disk_info->model, sizeof(disk_info->model), kCFStringEncodingUTF8);
                    CFRelease(model_cf);
                }
                else
                {
                    strcpy(disk_info->model, "Generic Disk");
                }

                strcpy(disk_info->protocol, "Unknown");
                strcpy(disk_info->serial, "Unknown");

                // Obtener tamaño del disco
                CFNumberRef size_cf = IORegistryEntryCreateCFProperty(disk,
                                                                      CFSTR(kIOMediaSizeKey),
                                                                      kCFAllocatorDefault, 0);
                if (size_cf)
                {
                    uint64_t size_bytes;
                    CFNumberGetValue(size_cf, kCFNumberSInt64Type, &size_bytes);
                    disk_info->total_kb = size_bytes / 1024;
                    CFRelease(size_cf);
                }
                else
                {
                    disk_info->total_kb = 0;
                }

                // Para información de espacio libre, necesitaríamos montar el disco
                disk_info->free_kb = 0;
                disk_info->used_kb = disk_info->total_kb;
                disk_info->usage_percent = 100.0f;

                disk_info->temperature = -1.0f;
                disk_info->sectors_read = 0;
                disk_info->sectors_written = 0;
                disk_info->read_errors = 0;
                disk_info->write_errors = 0;
                disk_info->power_on_hours = 0;
                disk_info->power_cycles = 0;
                disk_info->smart_status = 0;
                disk_info->health = 100;

                strcpy(disk_info->mount_point, "/");
                strcpy(disk_info->filesystem, "APFS");

                status->num_disks++;
            }
            if (is_whole)
                CFRelease(is_whole);
            IOObjectRelease(disk);
        }
        IOObjectRelease(iterator);
    }
}

void get_network_info_xnu(SystemStatus *status)
{
    status->num_network_interfaces = 0;

    struct ifaddrs *interfaces = NULL;
    if (getifaddrs(&interfaces) == 0)
    {
        struct ifaddrs *temp_addr = interfaces;

        while (temp_addr != NULL && status->num_network_interfaces < MAX_NETWORK_INTERFACES)
        {
            if (temp_addr->ifa_addr && temp_addr->ifa_addr->sa_family == AF_INET)
            {
                NetworkInfo *net = &status->network_interfaces[status->num_network_interfaces];

                strncpy(net->name, temp_addr->ifa_name, sizeof(net->name) - 1);
                strcpy(net->mac, "Unknown");

                struct sockaddr_in *sa = (struct sockaddr_in *)temp_addr->ifa_addr;
                strncpy(net->ip, inet_ntoa(sa->sin_addr), sizeof(net->ip) - 1);

                strcpy(net->netmask, "Unknown");
                strcpy(net->gateway, "Unknown");
                strcpy(net->dns1, "Unknown");
                strcpy(net->dns2, "Unknown");
                strcpy(net->driver, "Unknown");
                strcpy(net->type, "Unknown");

                net->rx_bytes = 0;
                net->tx_bytes = 0;
                net->rx_packets = 0;
                net->tx_packets = 0;
                net->rx_errors = 0;
                net->tx_errors = 0;
                net->rx_dropped = 0;
                net->tx_dropped = 0;
                net->speed_mbps = 0;
                net->duplex = 0;
                net->mtu = 0;

                status->num_network_interfaces++;
            }
            temp_addr = temp_addr->ifa_next;
        }
        freeifaddrs(interfaces);
    }
}

void get_gpu_info_xnu(SystemStatus *status)
{
    // Información básica de GPU (requiere Metal o IOKit específico)
    strcpy(status->gpu_model, "Unknown GPU");
    strcpy(status->gpu_driver, "Unknown");
    strcpy(status->gpu_vendor, "Unknown");
    status->gpu_memory_total_mb = 0;
    status->gpu_memory_used_mb = 0;
    status->gpu_memory_free_mb = 0;
    status->gpu_usage_percent = 0.0f;
    status->gpu_temperature = -1.0f;
    status->gpu_memory_usage_percent = 0.0f;
    status->gpu_fan_speed = 0;
    status->gpu_power_usage_w = 0;
    status->gpu_clock_mhz = 0;
    status->gpu_memory_clock_mhz = 0;
}

void get_sensors_info_xnu(SystemStatus *status)
{
    status->num_sensors = 0;
    // Los sensores en macOS requieren IOKit específico y pueden requerir permisos especiales
}

void get_motherboard_info_xnu(SystemStatus *status)
{
    // Obtener información del sistema
    char model[128];
    size_t size = sizeof(model);
    if (sysctlbyname("hw.model", model, &size, NULL, 0) == 0)
    {
        strncpy(status->motherboard.model, model, sizeof(status->motherboard.model) - 1);
    }
    else
    {
        strcpy(status->motherboard.model, "Unknown");
    }

    strcpy(status->motherboard.manufacturer, "Apple");
    strcpy(status->motherboard.version, "Unknown");
    strcpy(status->motherboard.bios_version, "Unknown");
}

void get_system_load_xnu(SystemStatus *status)
{
    // Obtener load average
    double loadavg[3];
    if (getloadavg(loadavg, 3) == 3)
    {
        status->load_average_1min = (float)loadavg[0];
        status->load_average_5min = (float)loadavg[1];
        status->load_average_15min = (float)loadavg[2];

        // Convertir a porcentaje (asumiendo que 1.0 = 100% para un core)
        status->load_1min_percent = (float)(loadavg[0] * 100.0);
        status->load_5min_percent = (float)(loadavg[1] * 100.0);
        status->load_15min_percent = (float)(loadavg[2] * 100.0);
    }
    else
    {
        status->load_average_1min = 0.0f;
        status->load_average_5min = 0.0f;
        status->load_average_15min = 0.0f;
        status->load_1min_percent = 0.0f;
        status->load_5min_percent = 0.0f;
        status->load_15min_percent = 0.0f;
    }
}

void get_process_info_xnu(SystemStatus *status)
{
    // Obtener información de procesos usando sysctl
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;

    if (sysctl(mib, 4, NULL, &size, NULL, 0) == 0)
    {
        status->total_processes = (int)(size / sizeof(struct kinfo_proc));
        status->running_processes = status->total_processes; // Simplificado
        status->sleeping_processes = 0;
        status->stopped_processes = 0;
        status->zombie_processes = 0;
    }
    else
    {
        status->total_processes = 0;
        status->running_processes = 0;
        status->sleeping_processes = 0;
        status->stopped_processes = 0;
        status->zombie_processes = 0;
    }
}

void get_power_info_xnu(SystemStatus *status)
{
    // Información de energía usando IOKit Power Sources
    CFTypeRef power_sources = IOPSCopyPowerSourcesInfo();
    if (power_sources)
    {
        CFArrayRef sources_list = IOPSCopyPowerSourcesList(power_sources);
        if (sources_list)
        {
            CFIndex count = CFArrayGetCount(sources_list);
            for (CFIndex i = 0; i < count; i++)
            {
                CFDictionaryRef source = CFArrayGetValueAtIndex(sources_list, i);

                // Verificar si es batería
                CFStringRef type = CFDictionaryGetValue(source, CFSTR(kIOPSTypeKey));
                if (type && CFStringCompare(type, CFSTR(kIOPSInternalBatteryType), 0) == kCFCompareEqualTo)
                {
                    // Obtener nivel de batería
                    CFNumberRef capacity = CFDictionaryGetValue(source, CFSTR(kIOPSCurrentCapacityKey));
                    if (capacity)
                    {
                        int battery_percent;
                        CFNumberGetValue(capacity, kCFNumberIntType, &battery_percent);
                        status->battery_percent = (float)battery_percent;
                    }

                    // Verificar si está conectado a AC
                    CFStringRef power_source = CFDictionaryGetValue(source, CFSTR(kIOPSPowerSourceStateKey));
                    if (power_source)
                    {
                        if (CFStringCompare(power_source, CFSTR(kIOPSACPowerValue), 0) == kCFCompareEqualTo)
                        {
                            status->ac_connected = 1;
                        }
                        else
                        {
                            status->ac_connected = 0;
                        }
                    }
                }
            }
            CFRelease(sources_list);
        }
        CFRelease(power_sources);
    }
    else
    {
        status->battery_percent = -1.0f;
        status->ac_connected = -1;
    }

    status->power_consumption = -1.0f; // No fácilmente disponible
}

void get_uptime_info_xnu(SystemStatus *status)
{
    // Obtener tiempo de actividad del sistema
    struct timeval boottime;
    size_t size = sizeof(boottime);
    if (sysctlbyname("kern.boottime", &boottime, &size, NULL, 0) == 0)
    {
        time_t now = time(NULL);
        status->uptime_seconds = now - boottime.tv_sec;
    }
    else
    {
        status->uptime_seconds = 0;
    }

    status->idle_time_seconds = 0; // No fácilmente disponible en macOS
}
