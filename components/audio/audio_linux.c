/**
 * @file audio_linux.c
 * @brief Implementación de audio para Linux
 */

#include "audio_interface.h"
#include "../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 * VARIABLES ESTÁTICAS
 * ============================================================================ */

static int initialized = 0;
static int device_count = 0;
static AudioDeviceInfo detected_devices[MAX_AUDIO_DEVICES];

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

static int scan_alsa_devices(void)
{
    device_count = 0;

    // Intentar obtener información de ALSA
    FILE *fp = popen("aplay -l 2>/dev/null", "r");
    if (!fp)
        return -1;

    char line[512];
    while (fgets(line, sizeof(line), fp) && device_count < MAX_AUDIO_DEVICES)
    {
        // Parsear líneas como: "card 0: PCH [HDA Intel PCH], device 0: ALC887-VD Analog [ALC887-VD Analog]"
        if (strstr(line, "card") && strstr(line, "device"))
        {
            AudioDeviceInfo *device = &detected_devices[device_count];
            memset(device, 0, sizeof(AudioDeviceInfo));

            // Extraer nombre del dispositivo
            char *bracket_start = strchr(line, '[');
            char *bracket_end = strrchr(line, ']');
            if (bracket_start && bracket_end && bracket_end > bracket_start)
            {
                int name_len = bracket_end - bracket_start - 1;
                if (name_len > 0 && name_len < (int)sizeof(device->name) - 1)
                {
                    strncpy(device->name, bracket_start + 1, name_len);
                    device->name[name_len] = '\0';
                }
            }
            else
            {
                snprintf(device->name, sizeof(device->name), "Audio Device %d", device_count);
            }

            // Configuración por defecto
            device->is_enabled = 1;
            device->volume_percent = -1; // No disponible fácilmente
            device->is_muted = 0;
            device->sample_rate_hz = 44100; // Valor típico
            device->bit_depth = 16;         // Valor típico
            device->channels = 2;           // Estéreo típico
            strncpy(device->format, "PCM", sizeof(device->format) - 1);
            strncpy(device->driver, "ALSA", sizeof(device->driver) - 1);

            // El primer dispositivo es por defecto
            if (device_count == 0)
            {
                device->is_default_playback = 1;
            }

            device_count++;
        }
    }
    pclose(fp);

    // Si no encontramos dispositivos con aplay, crear uno genérico
    if (device_count == 0)
    {
        AudioDeviceInfo *device = &detected_devices[0];
        memset(device, 0, sizeof(AudioDeviceInfo));

        strncpy(device->name, "Default Audio Device", sizeof(device->name) - 1);
        strncpy(device->driver, "Unknown", sizeof(device->driver) - 1);
        device->is_default_playback = 1;
        device->is_enabled = 1;
        device->volume_percent = -1;
        device->sample_rate_hz = 44100;
        device->bit_depth = 16;
        device->channels = 2;
        strncpy(device->format, "PCM", sizeof(device->format) - 1);

        device_count = 1;
    }

    return device_count;
}

static int get_pulseaudio_volume(void)
{
    FILE *fp = popen("pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null", "r");
    if (!fp)
        return -1;

    char line[256];
    if (fgets(line, sizeof(line), fp))
    {
        // Buscar porcentaje en la línea
        char *percent_pos = strstr(line, "%");
        if (percent_pos)
        {
            // Buscar hacia atrás el número
            char *start = percent_pos - 1;
            while (start > line && (*start == ' ' || (*start >= '0' && *start <= '9')))
            {
                start--;
            }
            start++;

            int volume = atoi(start);
            pclose(fp);
            return volume;
        }
    }

    pclose(fp);
    return -1;
}

static int is_pulseaudio_muted(void)
{
    FILE *fp = popen("pactl get-sink-mute @DEFAULT_SINK@ 2>/dev/null", "r");
    if (!fp)
        return -1;

    char line[256];
    if (fgets(line, sizeof(line), fp))
    {
        pclose(fp);
        return (strstr(line, "yes") != NULL) ? 1 : 0;
    }

    pclose(fp);
    return -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int audio_init(void)
{
    if (initialized)
        return 0;

    // Escanear dispositivos de audio
    scan_alsa_devices();

    initialized = 1;
    return 0;
}

void audio_cleanup(void)
{
    initialized = 0;
    device_count = 0;
}

int audio_get_device_count(void)
{
    if (!initialized)
    {
        audio_init();
    }
    return device_count;
}

int audio_get_device_info(int device_id, AudioDeviceInfo *audio_info)
{
    if (!audio_info || device_id < 0 || device_id >= device_count)
    {
        return -1;
    }

    if (!initialized)
    {
        audio_init();
    }

    // Copiar información del dispositivo
    memcpy(audio_info, &detected_devices[device_id], sizeof(AudioDeviceInfo));

    // Intentar obtener volumen actual si es el dispositivo por defecto
    if (audio_info->is_default_playback)
    {
        int volume = get_pulseaudio_volume();
        if (volume >= 0)
        {
            audio_info->volume_percent = volume;
        }

        int muted = is_pulseaudio_muted();
        if (muted >= 0)
        {
            audio_info->is_muted = muted;
        }
    }

    return 0;
}

int audio_get_default_device_info(AudioDeviceInfo *audio_info)
{
    if (!initialized)
    {
        audio_init();
    }

    // Buscar el dispositivo por defecto
    for (int i = 0; i < device_count; i++)
    {
        if (detected_devices[i].is_default_playback)
        {
            return audio_get_device_info(i, audio_info);
        }
    }

    // Si no hay dispositivo por defecto, usar el primero
    if (device_count > 0)
    {
        return audio_get_device_info(0, audio_info);
    }

    return -1;
}

int audio_get_master_volume_percent(void)
{
    return get_pulseaudio_volume();
}

int audio_is_muted(void)
{
    return is_pulseaudio_muted();
}