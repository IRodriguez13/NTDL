#include "disk_linux.h"
#include <stdio.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <stdlib.h>

#define ATA_OP_SMART_READ_DATA 0xD0

// ------------------ Funciones SMART ------------------
int get_disk_smart(const char *device, DiskInfo *info)
{
    int fd = open(device, O_RDONLY);
    if (fd < 0)
    {
        perror("No se puede abrir disco (fallback a /sys)");
        // Fallback: leer info básica desde /sys/block
        char path[256];
        FILE *f;

        snprintf(path, sizeof(path), "/sys/block/%s/device/model", device + 5); // "/dev/sda" -> "sda"
        f = fopen(path, "r");
        if (f)
        {
            if (fgets(info->name, sizeof(info->name), f))
            {
                info->name[strcspn(info->name, "\n")] = 0;
            }
            
            fclose(f);
        }

        // Temperatura y power-on: no disponibles sin root, dejamos -1
        info->temperature = -1;
        info->power_on_hours = -1;

    }

    unsigned char cdb[12], sense[32], data[512];
    sg_io_hdr_t io_hdr;

    memset(cdb, 0, sizeof(cdb));
    memset(sense, 0, sizeof(sense));
    memset(&io_hdr, 0, sizeof(io_hdr));
    memset(data, 0, sizeof(data));

    // ATA PASS-THROUGH (12 bytes)
    cdb[0] = 0xA1;                   // ATA PASS-THROUGH(12)
    cdb[1] = 0x08;                   // PIO Data-In
    cdb[2] = 0x0E;                   // Protocol bits
    cdb[3] = ATA_OP_SMART_READ_DATA; // SMART READ DATA

    io_hdr.interface_id = 'S';
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.cmd_len = sizeof(cdb);
    io_hdr.mx_sb_len = sizeof(sense);
    io_hdr.dxfer_len = sizeof(data);
    io_hdr.dxferp = data;
    io_hdr.cmdp = cdb;
    io_hdr.sbp = sense;
    io_hdr.timeout = 20000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0)
    {
        perror("ioctl SG_IO falló (fallback a /sys)");
        close(fd);
        info->temperature = -1;
        info->power_on_hours = -1;
        snprintf(info->name, sizeof(info->name), "DISCO0");
        return -1;
    }

    close(fd);

    // Parsear SMART
    info->temperature = -1;
    info->power_on_hours = -1;
    
    for (int i = 2; i < 512; i += 12)
    {
        unsigned char id = data[i];
        if (id == 194)
            info->temperature = data[i + 5];

        if (id == 9)
            info->power_on_hours = data[i + 5] | (data[i + 6] << 8);
    }

    snprintf(info->name, sizeof(info->name), "DISCO0"); // nombre estático
    return 0;
}


// alloc/free DiskInfo
DiskInfo *alloc_disk_info()
{
    DiskInfo *d = (DiskInfo *)malloc(sizeof(DiskInfo));
    if (d)
    {
        memset(d, 0, sizeof(DiskInfo));
        snprintf(d->name, sizeof(d->name), "DISCO0"); // nombre estático
    }
    
    return d;
}

void free_disk_info(DiskInfo *d)
{
    free(d);
}
