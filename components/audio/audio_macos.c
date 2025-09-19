/**
 * @file audio_macos.c
 * @brief Implementación de audio para macOS usando Core Audio
 */

#ifdef __APPLE__

#include "audio_interface.h"
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL audio_initialized = FALSE;
static AudioDevice audio_devices[MAX_AUDIO_DEVICES];
static int audio_device_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene el nombre de un dispositivo de audio
 */
static OSStatus get_device_name(AudioDeviceID device_id, char *name, size_t name_size) {
    AudioObjectPropertyAddress property_address = {
        kAudioDevicePropertyDeviceNameCFString,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    CFStringRef device_name = NULL;
    UInt32 property_size = sizeof(CFStringRef);
    
    OSStatus status = AudioObjectGetPropertyData(device_id, &property_address, 0, NULL,
                                                &property_size, &device_name);
    
    if (status == noErr && device_name) {
        CFStringGetCString(device_name, name, name_size, kCFStringEncodingUTF8);
        CFRelease(device_name);
        return noErr;
    }
    
    strncpy(name, "Unknown Device", name_size - 1);
    name[name_size - 1] = '\0';
    return status;
}

/**
 * @brief Obtiene el fabricante de un dispositivo de audio
 */
static OSStatus get_device_manufacturer(AudioDeviceID device_id, char *manufacturer, size_t manufacturer_size) {
    AudioObjectPropertyAddress property_address = {
        kAudioDevicePropertyDeviceManufacturerCFString,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    CFStringRef manufacturer_name = NULL;
    UInt32 property_size = sizeof(CFStringRef);
    
    OSStatus status = AudioObjectGetPropertyData(device_id, &property_address, 0, NULL,
                                                &property_size, &manufacturer_name);
    
    if (status == noErr && manufacturer_name) {
        CFStringGetCString(manufacturer_name, manufacturer, manufacturer_size, kCFStringEncodingUTF8);
        CFRelease(manufacturer_name);
        return noErr;
    }
    
    strncpy(manufacturer, "Unknown", manufacturer_size - 1);
    manufacturer[manufacturer_size - 1] = '\0';
    return status;
}

/**
 * @brief Obtiene el volumen de un dispositivo de audio
 */
static float get_device_volume(AudioDeviceID device_id, AudioObjectPropertyScope scope) {
    AudioObjectPropertyAddress property_address = {
        kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
        scope,
        kAudioObjectPropertyElementMaster
    };
    
    Float32 volume = 0.0f;
    UInt32 property_size = sizeof(Float32);
    
    OSStatus status = AudioObjectGetPropertyData(device_id, &property_address, 0, NULL,
                                                &property_size, &volume);
    
    if (status == noErr) {
        return volume * 100.0f; // Convertir a porcentaje
    }
    
    return -1.0f;
}

/**
 * @brief Verifica si un dispositivo está silenciado
 */
static int is_device_muted(AudioDeviceID device_id, AudioObjectPropertyScope scope) {
    AudioObjectPropertyAddress property_address = {
        kAudioDevicePropertyMute,
        scope,
        kAudioObjectPropertyElementMaster
    };
    
    UInt32 muted = 0;
    UInt32 property_size = sizeof(UInt32);
    
    OSStatus status = AudioObjectGetPropertyData(device_id, &property_address, 0, NULL,
                                                &property_size, &muted);
    
    if (status == noErr) {
        return muted ? 1 : 0;
    }
    
    return 0; // Asumir no silenciado si no se puede obtener
}

/**
 * @brief Obtiene información de formato de audio
 */
static void get_device_format_info(AudioDeviceID device_id, AudioObjectPropertyScope scope, AudioDevice *audio_dev) {
    AudioObjectPropertyAddress property_address = {
        kAudioDevicePropertyStreamFormat,
        scope,
        kAudioObjectPropertyElementMaster
    };
    
    AudioStreamBasicDescription format;
    UInt32 property_size = sizeof(AudioStreamBasicDescription);
    
    OSStatus status = AudioObjectGetPropertyData(device_id, &property_address, 0, NULL,
                                                &property_size, &format);
    
    if (status == noErr) {
        audio_dev->sample_rate = (int)format.mSampleRate;
        audio_dev->bit_depth = (int)format.mBitsPerChannel;
        audio_dev->channels = (int)format.mChannelsPerFrame;
    } else {
        // Valores por defecto
        audio_dev->sample_rate = 44100;
        audio_dev->bit_depth = 16;
        audio_dev->channels = 2;
    }
}

/**
 * @brief Verifica si un dispositivo es el por defecto
 */
static int is_default_device(AudioDeviceID device_id, AudioObjectPropertyScope scope) {
    AudioObjectPropertySelector selector;
    
    if (scope == kAudioDevicePropertyScopeOutput) {
        selector = kAudioHardwarePropertyDefaultOutputDevice;
    } else {
        selector = kAudioHardwarePropertyDefaultInputDevice;
    }
    
    AudioObjectPropertyAddress property_address = {
        selector,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    AudioDeviceID default_device = kAudioDeviceUnknown;
    UInt32 property_size = sizeof(AudioDeviceID);
    
    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 
                                                0, NULL, &property_size, &default_device);
    
    return (status == noErr && default_device == device_id) ? 1 : 0;
}

/**
 * @brief Enumera dispositivos de audio usando Core Audio
 */
static int enumerate_audio_devices_coreaudio(void) {
    AudioObjectPropertyAddress property_address = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    UInt32 property_size = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property_address,
                                                    0, NULL, &property_size);
    
    if (status != noErr) {
        return -1;
    }
    
    UInt32 device_count = property_size / sizeof(AudioDeviceID);
    AudioDeviceID *device_ids = malloc(property_size);
    
    if (!device_ids) {
        return -1;
    }
    
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address,
                                       0, NULL, &property_size, device_ids);
    
    if (status != noErr) {
        free(device_ids);
        return -1;
    }
    
    audio_device_count = 0;
    
    for (UInt32 i = 0; i < device_count && audio_device_count < MAX_AUDIO_DEVICES; i++) {
        AudioDeviceID device_id = device_ids[i];
        
        // Verificar si el dispositivo tiene streams de entrada o salida
        AudioObjectPropertyAddress streams_address = {
            kAudioDevicePropertyStreams,
            kAudioDevicePropertyScopeOutput,
            kAudioObjectPropertyElementMaster
        };
        
        UInt32 streams_size = 0;
        status = AudioObjectGetPropertyDataSize(device_id, &streams_address, 0, NULL, &streams_size);
        
        BOOL has_output = (status == noErr && streams_size > 0);
        
        streams_address.mScope = kAudioDevicePropertyScopeInput;
        streams_size = 0;
        status = AudioObjectGetPropertyDataSize(device_id, &streams_address, 0, NULL, &streams_size);
        
        BOOL has_input = (status == noErr && streams_size > 0);
        
        if (!has_output && !has_input) {
            continue; // Saltar dispositivos sin streams
        }
        
        // Crear entrada para dispositivo de salida
        if (has_output) {
            AudioDevice *audio_dev = &audio_devices[audio_device_count];
            memset(audio_dev, 0, sizeof(AudioDevice));
            
            audio_dev->type = AUDIO_TYPE_OUTPUT;
            
            // Nombre del dispositivo
            get_device_name(device_id, audio_dev->name, sizeof(audio_dev->name));
            
            // Descripción (fabricante + nombre)
            char manufacturer[256];
            get_device_manufacturer(device_id, manufacturer, sizeof(manufacturer));
            snprintf(audio_dev->description, sizeof(audio_dev->description),
                    "%s %s", manufacturer, audio_dev->name);
            
            // Volumen y mute
            audio_dev->volume_percent = get_device_volume(device_id, kAudioDevicePropertyScopeOutput);
            audio_dev->is_muted = is_device_muted(device_id, kAudioDevicePropertyScopeOutput);
            
            // Información de formato
            get_device_format_info(device_id, kAudioDevicePropertyScopeOutput, audio_dev);
            
            // Verificar si es el dispositivo por defecto
            audio_dev->is_default = is_default_device(device_id, kAudioDevicePropertyScopeOutput);
            
            // Estado (asumir habilitado)
            audio_dev->is_enabled = 1;
            
            audio_device_count++;
        }
        
        // Crear entrada para dispositivo de entrada
        if (has_input && audio_device_count < MAX_AUDIO_DEVICES) {
            AudioDevice *audio_dev = &audio_devices[audio_device_count];
            memset(audio_dev, 0, sizeof(AudioDevice));
            
            audio_dev->type = AUDIO_TYPE_INPUT;
            
            // Nombre del dispositivo
            get_device_name(device_id, audio_dev->name, sizeof(audio_dev->name));
            
            // Descripción (fabricante + nombre)
            char manufacturer[256];
            get_device_manufacturer(device_id, manufacturer, sizeof(manufacturer));
            snprintf(audio_dev->description, sizeof(audio_dev->description),
                    "%s %s (Input)", manufacturer, audio_dev->name);
            
            // Volumen y mute
            audio_dev->volume_percent = get_device_volume(device_id, kAudioDevicePropertyScopeInput);
            audio_dev->is_muted = is_device_muted(device_id, kAudioDevicePropertyScopeInput);
            
            // Información de formato
            get_device_format_info(device_id, kAudioDevicePropertyScopeInput, audio_dev);
            
            // Verificar si es el dispositivo por defecto
            audio_dev->is_default = is_default_device(device_id, kAudioDevicePropertyScopeInput);
            
            // Estado (asumir habilitado)
            audio_dev->is_enabled = 1;
            
            audio_device_count++;
        }
    }
    
    free(device_ids);
    return audio_device_count > 0 ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int audio_init(void) {
    if (audio_initialized) {
        return 0;
    }
    
    // Limpiar lista de dispositivos de audio
    memset(audio_devices, 0, sizeof(audio_devices));
    audio_device_count = 0;
    
    // Enumerar dispositivos de audio
    if (enumerate_audio_devices_coreaudio() != 0) {
        return -1;
    }
    
    audio_initialized = TRUE;
    return 0;
}

void audio_cleanup(void) {
    audio_initialized = FALSE;
    audio_device_count = 0;
}

int audio_get_device_count(void) {
    return audio_initialized ? audio_device_count : -1;
}

int audio_get_device_info(int device_id, AudioDevice *info) {
    if (!info || !audio_initialized || device_id < 0 || device_id >= audio_device_count) {
        return -1;
    }
    
    memcpy(info, &audio_devices[device_id], sizeof(AudioDevice));
    return 0;
}

int audio_get_metrics(AudioMetrics *metrics) {
    if (!metrics || !audio_initialized) {
        return -1;
    }
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(AudioMetrics));
    
    metrics->num_devices = audio_device_count;
    
    // Copiar información de todos los dispositivos
    for (int i = 0; i < audio_device_count && i < MAX_AUDIO_DEVICES; i++) {
        memcpy(&metrics->devices[i], &audio_devices[i], sizeof(AudioDevice));
    }
    
    return 0;
}

float audio_get_volume_percent(int device_id) {
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count) {
        return -1.0f;
    }
    
    return audio_devices[device_id].volume_percent;
}

int audio_is_muted(int device_id) {
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count) {
        return -1;
    }
    
    return audio_devices[device_id].is_muted ? 1 : 0;
}

int audio_get_device_name(int device_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !audio_initialized || 
        device_id < 0 || device_id >= audio_device_count) {
        return -1;
    }
    
    strncpy(buffer, audio_devices[device_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

AudioType audio_get_device_type(int device_id) {
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count) {
        return AUDIO_TYPE_UNKNOWN;
    }
    
    return audio_devices[device_id].type;
}

int audio_is_default_device(int device_id) {
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count) {
        return -1;
    }
    
    return audio_devices[device_id].is_default ? 1 : 0;
}

#endif /* __APPLE__ */