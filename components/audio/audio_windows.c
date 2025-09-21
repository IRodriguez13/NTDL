/**
 * @file audio_windows.c
 * @brief Implementación de audio para Windows usando Windows Audio APIs
 */

#ifdef _WIN32

#include "audio_interface.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <devicetopology.h>
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
 * @brief Convierte WCHAR a char
 */
static void wchar_to_char(const WCHAR *wstr, char *str, size_t str_size)
{
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, (int)str_size, NULL, NULL);
}

/**
 * @brief Enumera dispositivos de audio usando WASAPI
 */
static int enumerate_audio_devices(void)
{
    HRESULT hr;
    IMMDeviceEnumerator *device_enumerator = NULL;
    IMMDeviceCollection *device_collection = NULL;
    IMMDevice *device = NULL;
    IPropertyStore *property_store = NULL;
    PROPVARIANT prop_variant;
    UINT device_count = 0;

    // Inicializar COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return -1;
    }

    // Crear enumerador de dispositivos
    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void **)&device_enumerator);
    if (FAILED(hr))
    {
        CoUninitialize();
        return -1;
    }

    // Enumerar dispositivos de audio activos
    hr = device_enumerator->lpVtbl->EnumAudioEndpoints(
        device_enumerator,
        eAll, // Todos los tipos (render y capture)
        DEVICE_STATE_ACTIVE,
        &device_collection);
    if (FAILED(hr))
    {
        device_enumerator->lpVtbl->Release(device_enumerator);
        CoUninitialize();
        return -1;
    }

    // Obtener número de dispositivos
    hr = device_collection->lpVtbl->GetCount(device_collection, &device_count);
    if (FAILED(hr))
    {
        device_collection->lpVtbl->Release(device_collection);
        device_enumerator->lpVtbl->Release(device_enumerator);
        CoUninitialize();
        return -1;
    }

    audio_device_count = 0;

    // Procesar cada dispositivo
    for (UINT i = 0; i < device_count && audio_device_count < MAX_AUDIO_DEVICES; i++)
    {
        hr = device_collection->lpVtbl->Item(device_collection, i, &device);
        if (FAILED(hr))
            continue;

        AudioDevice *audio_dev = &audio_devices[audio_device_count];
        memset(audio_dev, 0, sizeof(AudioDevice));

        // Obtener propiedades del dispositivo
        hr = device->lpVtbl->OpenPropertyStore(device, STGM_READ, &property_store);
        if (SUCCEEDED(hr))
        {
            // Nombre del dispositivo
            PropVariantInit(&prop_variant);
            hr = property_store->lpVtbl->GetValue(property_store, &PKEY_Device_FriendlyName, &prop_variant);
            if (SUCCEEDED(hr) && prop_variant.vt == VT_LPWSTR)
            {
                wchar_to_char(prop_variant.pwszVal, audio_dev->name, sizeof(audio_dev->name));
            }
            PropVariantClear(&prop_variant);

            // Descripción del dispositivo
            hr = property_store->lpVtbl->GetValue(property_store, &PKEY_Device_DeviceDesc, &prop_variant);
            if (SUCCEEDED(hr) && prop_variant.vt == VT_LPWSTR)
            {
                wchar_to_char(prop_variant.pwszVal, audio_dev->description, sizeof(audio_dev->description));
            }
            PropVariantClear(&prop_variant);

            property_store->lpVtbl->Release(property_store);
        }

        // Determinar tipo de dispositivo
        EDataFlow data_flow;
        IMMEndpoint *endpoint = NULL;
        hr = device->lpVtbl->QueryInterface(device, &IID_IMMEndpoint, (void **)&endpoint);
        if (SUCCEEDED(hr))
        {
            hr = endpoint->lpVtbl->GetDataFlow(endpoint, &data_flow);
            if (SUCCEEDED(hr))
            {
                switch (data_flow)
                {
                case eRender:
                    audio_dev->type = AUDIO_TYPE_OUTPUT;
                    break;
                case eCapture:
                    audio_dev->type = AUDIO_TYPE_INPUT;
                    break;
                default:
                    audio_dev->type = AUDIO_TYPE_UNKNOWN;
                    break;
                }
            }
            endpoint->lpVtbl->Release(endpoint);
        }

        // Obtener información de volumen
        IAudioEndpointVolume *endpoint_volume = NULL;
        hr = device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void **)&endpoint_volume);
        if (SUCCEEDED(hr))
        {
            float volume_level = 0.0f;
            hr = endpoint_volume->lpVtbl->GetMasterScalarVolume(endpoint_volume, &volume_level);
            if (SUCCEEDED(hr))
            {
                audio_dev->volume_percent = volume_level * 100.0f;
            }

            BOOL is_muted = FALSE;
            hr = endpoint_volume->lpVtbl->GetMute(endpoint_volume, &is_muted);
            if (SUCCEEDED(hr))
            {
                audio_dev->is_muted = is_muted ? 1 : 0;
            }

            endpoint_volume->lpVtbl->Release(endpoint_volume);
        }

        // Verificar si es el dispositivo por defecto
        IMMDevice *default_device = NULL;
        hr = device_enumerator->lpVtbl->GetDefaultAudioEndpoint(
            device_enumerator,
            data_flow,
            eConsole,
            &default_device);
        if (SUCCEEDED(hr))
        {
            LPWSTR device_id1 = NULL, device_id2 = NULL;
            device->lpVtbl->GetId(device, &device_id1);
            default_device->lpVtbl->GetId(default_device, &device_id2);

            if (device_id1 && device_id2 && wcscmp(device_id1, device_id2) == 0)
            {
                audio_dev->is_default = 1;
            }

            if (device_id1)
                CoTaskMemFree(device_id1);
            if (device_id2)
                CoTaskMemFree(device_id2);
            default_device->lpVtbl->Release(default_device);
        }

        // Información adicional (valores por defecto)
        audio_dev->is_enabled = 1;      // Asumir habilitado si está activo
        audio_dev->sample_rate = 44100; // Valor típico por defecto
        audio_dev->bit_depth = 16;      // Valor típico por defecto
        audio_dev->channels = 2;        // Estéreo por defecto

        device->lpVtbl->Release(device);
        audio_device_count++;
    }

    // Limpiar recursos
    device_collection->lpVtbl->Release(device_collection);
    device_enumerator->lpVtbl->Release(device_enumerator);
    CoUninitialize();

    return audio_device_count > 0 ? 0 : -1;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int audio_init(void)
{
    if (audio_initialized)
    {
        return 0;
    }

    // Limpiar lista de dispositivos de audio
    memset(audio_devices, 0, sizeof(audio_devices));
    audio_device_count = 0;

    // Enumerar dispositivos de audio
    if (enumerate_audio_devices() != 0)
    {
        return -1;
    }

    audio_initialized = TRUE;
    return 0;
}

void audio_cleanup(void)
{
    audio_initialized = FALSE;
    audio_device_count = 0;
}

int audio_get_device_count(void)
{
    return audio_initialized ? audio_device_count : -1;
}

int audio_get_device_info(int device_id, AudioDevice *info)
{
    if (!info || !audio_initialized || device_id < 0 || device_id >= audio_device_count)
    {
        return -1;
    }

    memcpy(info, &audio_devices[device_id], sizeof(AudioDevice));
    return 0;
}

int audio_get_metrics(AudioMetrics *metrics)
{
    if (!metrics || !audio_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(AudioMetrics));

    metrics->num_devices = audio_device_count;

    // Copiar información de todos los dispositivos
    for (int i = 0; i < audio_device_count && i < MAX_AUDIO_DEVICES; i++)
    {
        memcpy(&metrics->devices[i], &audio_devices[i], sizeof(AudioDevice));
    }

    return 0;
}

float audio_get_volume_percent(int device_id)
{
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count)
    {
        return -1.0f;
    }

    return audio_devices[device_id].volume_percent;
}

int audio_is_muted(int device_id)
{
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count)
    {
        return -1;
    }

    return audio_devices[device_id].is_muted ? 1 : 0;
}

int audio_get_device_name(int device_id, char *buffer, size_t size)
{
    if (!buffer || size == 0 || !audio_initialized ||
        device_id < 0 || device_id >= audio_device_count)
    {
        return -1;
    }

    strncpy(buffer, audio_devices[device_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

AudioType audio_get_device_type(int device_id)
{
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count)
    {
        return AUDIO_TYPE_UNKNOWN;
    }

    return audio_devices[device_id].type;
}

int audio_is_default_device(int device_id)
{
    if (!audio_initialized || device_id < 0 || device_id >= audio_device_count)
    {
        return -1;
    }

    return audio_devices[device_id].is_default ? 1 : 0;
}

#endif /* _WIN32 */