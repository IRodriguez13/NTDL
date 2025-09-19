#include <stdio.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <stdlib.h>
#include "disk_linux.h"

#define ATA_OP_SMART_READ_DATA 0xD0

int get_disk_smart_via_smartctl(const char *device, DiskInfo *info)
{
    char cmd[512];
    FILE *fp;
    char line[256];

    // Verificar si SMART está disponible
    snprintf(cmd, sizeof(cmd), "smartctl -i %s 2>/dev/null | grep -q 'SMART support is: Available'", device);
    if (system(cmd) != 0)
    {
        printf("DEBUG - SMART no disponible según smartctl\n");
        goto cleanup;
    }

    printf("DEBUG - SMART disponible, obteniendo datos...\n");

    // Obtener datos SMART
    snprintf(cmd, sizeof(cmd), "smartctl -A %s 2>/dev/null", device);
    fp = popen(cmd, "r");
    if (!fp)
    {
        goto cleanup;
    }

    info->temperature = -1;
    info->power_on_hours = -1;

    while (fgets(line, sizeof(line), fp))
    {
        printf("DEBUG - Línea: %s", line);

        // Buscar temperatura (varios nombres posibles)
        if (strstr(line, "Temperature_Celsius") ||
            strstr(line, "Airflow_Temperature_Cel") ||
            strstr(line, "Temperature"))
        {

            int temp;
            // Formato típico: ID ATTRIBUTE_NAME FLAG VALUE WORST THRESH TYPE UPDATED WHEN_FAILED RAW_VALUE
            if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &temp) == 1)
            {
                info->temperature = temp;
                printf("DEBUG - Temperatura encontrada: %d°C\n", temp);
            }
        }

        // Buscar Power On Hours
        if (strstr(line, "Power_On_Hours"))
        {
            int hours;
            if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &hours) == 1)
            {
                info->power_on_hours = hours;
                printf("DEBUG - Power On Hours: %d\n", hours);
            }
        }

        // Para SSDs, también buscar por Write/Erase cycles
        if (strstr(line, "Total_LBAs_Written"))
        {
            int lbas;
            if (sscanf(line, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", &lbas) == 1)
            {
                printf("DEBUG - Total LBAs Written: %d\n", lbas);
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

            snprintf(info->name, sizeof(info->name), "%.63s", model);
        }
        pclose(fp);
    }
    else
    {
        snprintf(info->name, sizeof(info->name), "SSD0");
    }

    return (info->temperature != -1 || info->power_on_hours != -1) ? 0 : -1;

cleanup:
    info->temperature = -1;
    info->power_on_hours = -1;
    snprintf(info->name, sizeof(info->name), "SSD0");
    return -1;
}

// ------------------ Funciones SMART ------------------
int get_disk_smart(const char *device, DiskInfo *info)
{
    int fd = open(device, O_RDONLY);
    if (fd < 0)
    {
        Kerror("open falló");
        goto cleanup_no_fd;
    }

    unsigned char cdb[16] = {0}; // Usar 16 bytes para mejor compatibilidad
    unsigned char sense[32] = {0};
    unsigned char data[512] = {0};
    sg_io_hdr_t io_hdr;

    // Inicializar estructura
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 16; // Comando de 16 bytes
    io_hdr.mx_sb_len = sizeof(sense);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = sizeof(data);
    io_hdr.dxferp = data;
    io_hdr.cmdp = cdb;
    io_hdr.sbp = sense;
    io_hdr.timeout = 20000;

    // ATA PASS-THROUGH (16) - Mejor para SSDs
    cdb[0] = 0x85;         // ATA PASS-THROUGH (16)
    cdb[1] = (4 << 1) | 1; // protocol=4 (PIO data-in), extend=0, t_length=01b
    cdb[2] = 0x0e;         // t_dir=1, byte_block=0, t_length=10b
    cdb[3] = 0;            // features (15:8)
    cdb[4] = 0;            // features (7:0)
    cdb[5] = 0;            // sector_count (15:8)
    cdb[6] = 1;            // sector_count (7:0)
    cdb[8] = 0xd0;         // lba (7:0) - SMART READ DATA
    cdb[10] = 0x4f;        // lba (23:16)
    cdb[12] = 0xc2;        // lba (39:32)
    cdb[13] = 0xa0;        // device (era 0, pero algunos SSDs necesitan 0xa0)
    cdb[14] = 0xb0;        // command - SMART READ DATA

    if (ioctl(fd, SG_IO, &io_hdr) < 0)
    {
        Kerror("ioctl SG_IO falló");
        goto cleanup_with_fallback;
    }

    printf("DEBUG - status: 0x%02x, host_status: 0x%02x, driver_status: 0x%02x\n",
           io_hdr.status, io_hdr.host_status, io_hdr.driver_status);

    // Para SSDs, a veces status puede ser 0x02 pero aún tener datos válidos
    if (io_hdr.status == 0x02 && io_hdr.sb_len_wr > 0)
    {

        // Verificar si es realmente un error o solo una advertencia
        if (io_hdr.sb_len_wr >= 3)
        {
            unsigned char sense_key = sense[2] & 0x0f;
            // Sense key 0x01 (RECOVERED_ERROR) es OK para continuar
            if (sense_key > 0x01)
            {
                printf("DEBUG - Error grave, abandonando\n");
                goto cleanup_with_fallback;
            }
        }
    }

    // Verificar si recibimos datos
    if (io_hdr.dxfer_len - io_hdr.resid < 512)
    {
        printf("DEBUG - Datos insuficientes: recibidos %d bytes\n",
               io_hdr.dxfer_len - io_hdr.resid);
        goto cleanup_with_fallback;
    }

    close(fd);

    for (int i = 2; i < 50; i += 12)
    {
        if (data[i] == 0)
            break;
        printf("  ID=%d, flags=%02x%02x, value=%d, raw=%02x%02x%02x%02x\n",
               data[i], data[i + 1], data[i + 2], data[i + 3],
               data[i + 5], data[i + 6], data[i + 7], data[i + 8]);
    }

    // Parsear datos SMART específicos para SSD
    info->temperature = -1;
    info->power_on_hours = -1;

    for (int i = 2; i < 362; i += 12)
    {
        unsigned char id = data[i];

        if (id == 0)
            break;

        // Temperatura (ID 194 o a veces ID 190 en algunos SSDs)
        if (id == 194 || id == 190)
        {
            info->temperature = data[i + 5]; // Raw value (LSB)
            printf("DEBUG - Temperatura (ID %d): %d°C\n", id, info->temperature);
        }

        // Power On Hours (ID 9)
        if (id == 9)
        {
            // Para SSDs, usar los primeros 4 bytes del raw value
            info->power_on_hours = data[i + 5] |
                                   (data[i + 6] << 8) |
                                   (data[i + 7] << 16) |
                                   (data[i + 8] << 24);
            printf("DEBUG - Power On Hours: %d\n", info->power_on_hours);
        }

        // Total LBAs Written (ID 241) - específico de SSD
        if (id == 241)
        {
            int lbas_written = data[i + 5] | (data[i + 6] << 8);
            printf("DEBUG - LBAs Written: %d\n", lbas_written);
        }
    }

    snprintf(info->name, sizeof(info->name), "SSD0");
    return 0;

cleanup_with_fallback:
    close(fd);

cleanup_no_fd:
    info->temperature = -1;
    info->power_on_hours = -1;
    snprintf(info->name, sizeof(info->name), "SSD0");
    return -1;
}

// alloc/free DiskInfo
DiskInfo *alloc_disk_info()
{
    DiskInfo *d = (DiskInfo *)malloc(sizeof(DiskInfo));
    if (d == NULL)
    {
        Kerror("malloc");
        goto cleanup;
    }
    if (d)
    {
        memset(d, 0, sizeof(DiskInfo));
        snprintf(d->name, sizeof(d->name), "DISCO0"); // nombre estático
    }

    return d;

cleanup:
    free(d);
    return NULL;
}
