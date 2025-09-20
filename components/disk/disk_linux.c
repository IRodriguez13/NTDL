/**
 * @file disk_linux.c
 * @brief Implementación de discos para Linux
 */

#include "disk_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int disk_count = 0;
static DiskInfo detected_disks[MAX_DISKS];

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int get_disk_smart_via_smartctl(const char *device, DiskInfo *info)
{
    char cmd[512];
    FILE *fp;
    char line[256];

    // Verificar si SMART está disponible
    snprintf(cmd, sizeof(cmd), "smartctl -i %s 2>/dev/null | grep -q 'SMART support is: Available'", device);
    if (system(cmd) != 0)
    {
        return -1;
    }

    // Obtener datos SMART
    snprintf(cmd, sizeof(cmd), "smartctl -A %s 2>/dev/null", device);
    fp = popen(cmd, "r");
    if (!fp)
    {
        return -1;
    }

    info->temperature = -1;
    info->power_on_hours = -1;

    while (fgets(line, sizeof(line), fp))
    {
        // Buscar temperatura
        if (strstr(line, "Temperature_Celsius") ||
            strstr(line, "Airflow_Temperature_Cel") ||
            strstr(line, "Temperature"))
        {
            int temp;
            if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &temp) == 1)
            {
                info->temperature = temp;
            }
        }

        // Buscar Power On Hours
        if (strstr(line, "Power_On_Hours"))
        {
            int hours;
            if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &hours) == 1)
            {
                info->power_on_hours = hours;
            }
        }
    }

    pclose(fp);

    // Obtener nombre del modelo
    snprintf(cmd, sizeof(cmd), "smartctl -i %s 2>/dev/null | grep 'Device Model'", device);
    fp = popen(cmd, "r");
    if (fp && fgets(line, sizeof(line), fp))
    {
        char *model = strstr(line, ":");
        if (model)
        {
            model++; // Saltar ':'
            while (*model == ' ')
                model++; // Saltar espacios

            // Quitar salto de línea
            char *newline = strchr(model, '\n');
            if (newline)
                *newline = '\0';

            strncpy(info->name, model, sizeof(info->name) - 1);
            info->name[sizeof(info->name) - 1] = '\0';
        }
        pclose(fp);
    }

    return (info->temperature != -1 || info->power_on_hours != -1) ? 0 : -1;
}

static int get_disk_usage_info(const char *mount_point, DiskInfo *info)
{
    struct statvfs stat;

    if (statvfs(mount_point, &stat) != 0)
    {
        return -1;
    }

    uint64_t total_bytes = (uint64_t)stat.f_blocks * stat.f_frsize;
    uint64_t free_bytes = (uint64_t)stat.f_bavail * stat.f_frsize;
    uint64_t used_bytes = total_bytes - free_bytes;

    info->total_gb = total_bytes / (1024ULL * 1024ULL * 1024ULL);
    info->free_gb = free_bytes / (1024ULL * 1024ULL * 1024ULL);
    info->used_gb = used_bytes / (1024ULL * 1024ULL * 1024ULL);

    if (total_bytes > 0)
    {
        info->usage_percent = ((float)used_bytes / total_bytes) * 100.0f;
    }

    return 0;
}

static int is_rotational_disk(const char *device)
{
    char path[256];
    char *device_name = strrchr(device, '/');
    if (!device_name)
        return -1;
    device_name++; // Saltar '/'

    snprintf(path, sizeof(path), "/sys/block/%s/queue/rotational", device_name);

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    int rotational;
    int ret = fscanf(f, "%d", &rotational);
    fclose(f);

    if (ret != 1)
        return -1;

    return rotational; // 1 = HDD, 0 = SSD
}

static int get_disk_io_stats_from_proc(const char *device, DiskInfo *info)
{
    char *device_name = strrchr(device, '/');
    if (!device_name)
        return -1;
    device_name++; // Saltar '/'

    FILE *f = fopen("/proc/diskstats", "r");
    if (!f)
        return -1;

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        char disk_name[64];
        uint64_t read_ios, read_sectors, write_ios, write_sectors;

        if (sscanf(line, "%*d %*d %63s %lu %*u %lu %*u %lu %*u %lu",
                   disk_name, &read_ios, &read_sectors, &write_ios, &write_sectors) == 5)
        {

            if (strcmp(disk_name, device_name) == 0)
            {
                // Convertir sectores a bytes (asumiendo 512 bytes por sector)
                info->read_bytes = read_sectors * 512;
                info->write_bytes = write_sectors * 512;
                info->read_ops = (uint32_t)read_ios;
                info->write_ops = (uint32_t)write_ios;

                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return -1;
}

static int scan_mounted_disks(void)
{
    FILE *f = fopen("/proc/mounts", "r");
    if (!f)
        return -1;

    char line[512];
    disk_count = 0;

    while (fgets(line, sizeof(line), f) && disk_count < MAX_DISKS)
    {
        char device[256], mount_point[256], filesystem[64];

        if (sscanf(line, "%255s %255s %63s", device, mount_point, filesystem) == 3)
        {
            // Solo procesar dispositivos de bloque reales
            if (strncmp(device, "/dev/", 5) != 0)
                continue;
            if (strstr(device, "loop") != NULL)
                continue;
            if (strstr(mount_point, "/snap/") != NULL)
                continue; // Saltar snap mounts

            // Saltar sistemas de archivos virtuales
            if (strcmp(filesystem, "proc") == 0 || strcmp(filesystem, "sysfs") == 0 || strcmp(filesystem, "devpts") == 0 || strcmp(filesystem, "tmpfs") == 0)
            {
                continue;
            }

            DiskInfo *disk = &detected_disks[disk_count];
            memset(disk, 0, sizeof(DiskInfo));

            // Información básica
            snprintf(disk->name, sizeof(disk->name), "%s", device);

            snprintf(disk->mount_point, sizeof(disk->mount_point), "%s", mount_point);

            snprintf(disk->filesystem, sizeof(disk->filesystem), "%s", filesystem);

            // Información de uso
            get_disk_usage_info(mount_point, disk);

            // Información SMART (si está disponible)
            get_disk_smart_via_smartctl(device, disk);

            // Verificar si es SSD
            int rotational = is_rotational_disk(device);

            switch (rotational)
            {
                case 0:
                    disk->is_ssd = 1;
                    break;

                case 1:
                    disk->is_ssd = 0;
                    break;

                default:
                    disk->is_ssd = -1;
                    break;
            }

            // Estadísticas de I/O
            get_disk_io_stats_from_proc(device, disk);

            // Si no tenemos nombre del modelo, usar uno genérico
            if (strlen(disk->name) == 0)
            {
                if (disk->is_ssd == 1)
                {
                    snprintf(disk->name, sizeof(disk->name), "SSD %d", disk_count);
                }
                else if (disk->is_ssd == 0)
                {
                    snprintf(disk->name, sizeof(disk->name), "HDD %d", disk_count);
                }
                else
                {
                    snprintf(disk->name, sizeof(disk->name), "Disk %d", disk_count);
                }
            }

            disk_count++;
        }
    }

    fclose(f);
    return disk_count;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int disk_init(void)
{
    if (initialized)
        return 0;

    // Escanear discos montados
    scan_mounted_disks();

    initialized = 1;
    return 0;
}

void disk_cleanup(void)
{
    initialized = 0;
    disk_count = 0;
}

int disk_get_count(void)
{
    if (!initialized)
    {
        disk_init();
    }
    return disk_count;
}

int disk_get_info(int disk_id, DiskInfo *disk_info)
{
    if (!disk_info || disk_id < 0 || disk_id >= disk_count)
    {
        return -1;
    }

    if (!initialized)
    {
        disk_init();
    }

    // Actualizar información de uso (puede cambiar)
    get_disk_usage_info(detected_disks[disk_id].mount_point, &detected_disks[disk_id]);

    // Copiar información
    memcpy(disk_info, &detected_disks[disk_id], sizeof(DiskInfo));
    return 0;
}

int disk_get_smart_info(const char *device_path, DiskInfo *disk_info)
{
    if (!device_path || !disk_info)
        return -1;

    return get_disk_smart_via_smartctl(device_path, disk_info);
}

float disk_get_temperature(const char *device_path)
{
    if (!device_path)
        return -1.0f;

    DiskInfo temp_info;
    if (get_disk_smart_via_smartctl(device_path, &temp_info) == 0)
    {
        return (float)temp_info.temperature;
    }

    return -1.0f;
}

int disk_get_power_on_hours(const char *device_path)
{
    if (!device_path)
        return -1;

    DiskInfo temp_info;
    if (get_disk_smart_via_smartctl(device_path, &temp_info) == 0)
    {
        return temp_info.power_on_hours;
    }

    return -1;
}

int disk_is_ssd(const char *device_path)
{
    if (!device_path)
        return -1;

    int rotational = is_rotational_disk(device_path);
    if (rotational == 0)
    {
        return 1; // SSD
    }
    else if (rotational == 1)
    {
        return 0; // HDD
    }

    return -1; // No se puede determinar
}

int disk_get_io_stats(const char *device_path, uint64_t *read_bytes, uint64_t *write_bytes,
                      uint32_t *read_ops, uint32_t *write_ops)
{
    if (!device_path)
        return -1;

    DiskInfo temp_info;
    if (get_disk_io_stats_from_proc(device_path, &temp_info) == 0)
    {
        if (read_bytes)
            *read_bytes = temp_info.read_bytes;
        if (write_bytes)
            *write_bytes = temp_info.write_bytes;
        if (read_ops)
            *read_ops = temp_info.read_ops;
        if (write_ops)
            *write_ops = temp_info.write_ops;
        return 0;
    }

    return -1;
}