/**
 * @file sensors_windows.c
 * @brief Implementación de sensores para Windows usando WMI y APIs del sistema
 */

#ifdef _WIN32

#include "sensors_interface.h"
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL sensors_initialized = FALSE;
static SensorInfo sensor_list[MAX_SENSORS];
static int sensor_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Obtiene sensores de temperatura usando WMI
 */
static int get_temperature_sensors_wmi(void)
{
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *class_object = NULL;
    ULONG returned = 0;
    VARIANT variant;

    // Inicializar COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

    // Crear locator WMI
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID *)&locator);
    if (FAILED(hr))
    {
        CoUninitialize();
        return -1;
    }

    // Conectar al namespace WMI
    hr = locator->lpVtbl->ConnectServer(
        locator,
        L"ROOT\\WMI",
        NULL, NULL, 0, NULL, 0, 0,
        &services);
    if (FAILED(hr))
    {
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Configurar seguridad
    hr = CoSetProxyBlanket(
        (IUnknown *)services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);
    if (FAILED(hr))
    {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Intentar obtener sensores de temperatura MSAcpi_ThermalZoneTemperature
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT CurrentTemperature, InstanceName FROM MSAcpi_ThermalZoneTemperature",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    if (SUCCEEDED(hr))
    {
        // Procesar sensores de temperatura
        while (SUCCEEDED(enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned)) &&
               returned != 0 && sensor_count < MAX_SENSORS)
        {

            SensorInfo *sensor = &sensor_list[sensor_count];
            memset(sensor, 0, sizeof(SensorInfo));

            // Tipo de sensor
            sensor->type = SENSOR_TYPE_TEMPERATURE;

            // Nombre del sensor
            VariantInit(&variant);
            hr = class_object->lpVtbl->Get(class_object, L"InstanceName", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sensor->name, sizeof(sensor->name), NULL, NULL);
            }
            else
            {
                snprintf(sensor->name, sizeof(sensor->name), "Thermal Zone %d", sensor_count);
            }
            VariantClear(&variant);

            // Temperatura actual
            hr = class_object->lpVtbl->Get(class_object, L"CurrentTemperature", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_I4)
            {
                // La temperatura viene en décimas de Kelvin, convertir a Celsius
                float temp_kelvin = variant.lVal / 10.0f;
                sensor->value = temp_kelvin - 273.15f;
            }
            VariantClear(&variant);

            // Etiqueta descriptiva
            strncpy(sensor->label, "Temperature", sizeof(sensor->label) - 1);

            // Valores por defecto
            sensor->min_value = 0.0f;
            sensor->max_value = 100.0f;
            sensor->critical_value = 90.0f;

            class_object->lpVtbl->Release(class_object);
            class_object = NULL;
            sensor_count++;
        }

        enumerator->lpVtbl->Release(enumerator);
    }

    // Limpiar recursos
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();

    return 0;
}

/**
 * @brief Obtiene información de ventiladores usando WMI
 */
static int get_fan_sensors_wmi(void)
{
    HRESULT hr;
    IWbemLocator *locator = NULL;
    IWbemServices *services = NULL;
    IEnumWbemClassObject *enumerator = NULL;
    IWbemClassObject *class_object = NULL;
    ULONG returned = 0;
    VARIANT variant;

    // Inicializar COM
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return -1;

    // Crear locator WMI
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID *)&locator);
    if (FAILED(hr))
    {
        CoUninitialize();
        return -1;
    }

    // Conectar al namespace WMI
    hr = locator->lpVtbl->ConnectServer(
        locator,
        L"ROOT\\CIMV2",
        NULL, NULL, 0, NULL, 0, 0,
        &services);
    if (FAILED(hr))
    {
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Configurar seguridad
    hr = CoSetProxyBlanket(
        (IUnknown *)services,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);
    if (FAILED(hr))
    {
        services->lpVtbl->Release(services);
        locator->lpVtbl->Release(locator);
        CoUninitialize();
        return -1;
    }

    // Intentar obtener información de ventiladores
    hr = services->lpVtbl->ExecQuery(
        services,
        L"WQL",
        L"SELECT Name, DesiredSpeed FROM Win32_Fan",
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    if (SUCCEEDED(hr))
    {
        // Procesar ventiladores
        while (SUCCEEDED(enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &class_object, &returned)) &&
               returned != 0 && sensor_count < MAX_SENSORS)
        {

            SensorInfo *sensor = &sensor_list[sensor_count];
            memset(sensor, 0, sizeof(SensorInfo));

            // Tipo de sensor
            sensor->type = SENSOR_TYPE_FAN;

            // Nombre del ventilador
            VariantInit(&variant);
            hr = class_object->lpVtbl->Get(class_object, L"Name", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_BSTR)
            {
                WideCharToMultiByte(CP_UTF8, 0, variant.bstrVal, -1,
                                    sensor->name, sizeof(sensor->name), NULL, NULL);
            }
            else
            {
                snprintf(sensor->name, sizeof(sensor->name), "Fan %d", sensor_count);
            }
            VariantClear(&variant);

            // Velocidad deseada (puede no estar disponible)
            hr = class_object->lpVtbl->Get(class_object, L"DesiredSpeed", 0, &variant, 0, 0);
            if (SUCCEEDED(hr) && variant.vt == VT_I4)
            {
                sensor->value = (float)variant.lVal;
            }
            else
            {
                sensor->value = -1.0f; // No disponible
            }
            VariantClear(&variant);

            // Etiqueta descriptiva
            strncpy(sensor->label, "Fan Speed", sizeof(sensor->label) - 1);

            // Valores por defecto para ventiladores
            sensor->min_value = 0.0f;
            sensor->max_value = 5000.0f;     // RPM típico máximo
            sensor->critical_value = 100.0f; // RPM mínimo crítico

            class_object->lpVtbl->Release(class_object);
            class_object = NULL;
            sensor_count++;
        }

        enumerator->lpVtbl->Release(enumerator);
    }

    // Limpiar recursos
    services->lpVtbl->Release(services);
    locator->lpVtbl->Release(locator);
    CoUninitialize();

    return 0;
}

/**
 * @brief Agrega sensores simulados si no se encuentran sensores reales
 */
static void add_simulated_sensors(void)
{
    if (sensor_count == 0)
    {
        // Agregar sensor de CPU simulado
        SensorInfo *cpu_sensor = &sensor_list[sensor_count++];
        memset(cpu_sensor, 0, sizeof(SensorInfo));
        cpu_sensor->type = SENSOR_TYPE_TEMPERATURE;
        strncpy(cpu_sensor->name, "CPU Temperature", sizeof(cpu_sensor->name) - 1);
        strncpy(cpu_sensor->label, "CPU Temp", sizeof(cpu_sensor->label) - 1);
        cpu_sensor->value = 45.0f; // Valor simulado
        cpu_sensor->min_value = 20.0f;
        cpu_sensor->max_value = 100.0f;
        cpu_sensor->critical_value = 85.0f;

        // Agregar sensor de sistema simulado
        if (sensor_count < MAX_SENSORS)
        {
            SensorInfo *sys_sensor = &sensor_list[sensor_count++];
            memset(sys_sensor, 0, sizeof(SensorInfo));
            sys_sensor->type = SENSOR_TYPE_TEMPERATURE;
            strncpy(sys_sensor->name, "System Temperature", sizeof(sys_sensor->name) - 1);
            strncpy(sys_sensor->label, "System Temp", sizeof(sys_sensor->label) - 1);
            sys_sensor->value = 40.0f; // Valor simulado
            sys_sensor->min_value = 20.0f;
            sys_sensor->max_value = 80.0f;
            sys_sensor->critical_value = 70.0f;
        }
    }
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int sensors_init(void)
{
    if (sensors_initialized)
    {
        return 0;
    }

    // Limpiar lista de sensores
    memset(sensor_list, 0, sizeof(sensor_list));
    sensor_count = 0;

    // Intentar obtener sensores de temperatura
    get_temperature_sensors_wmi();

    // Intentar obtener sensores de ventiladores
    get_fan_sensors_wmi();

    // Si no se encontraron sensores, agregar algunos simulados
    add_simulated_sensors();

    sensors_initialized = TRUE;
    return 0;
}

void sensors_cleanup(void)
{
    sensors_initialized = FALSE;
    sensor_count = 0;
}

int sensors_get_count(void)
{
    return sensors_initialized ? sensor_count : -1;
}

int sensors_get_info(int sensor_id, SensorInfo *info)
{
    if (!info || !sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count)
    {
        return -1;
    }

    memcpy(info, &sensor_list[sensor_id], sizeof(SensorInfo));
    return 0;
}

int sensors_get_metrics(SensorMetrics *metrics)
{
    if (!metrics || !sensors_initialized)
    {
        return -1;
    }

    // Limpiar estructura
    memset(metrics, 0, sizeof(SensorMetrics));

    metrics->num_sensors = sensor_count;

    // Copiar información de todos los sensores
    for (int i = 0; i < sensor_count && i < MAX_SENSORS; i++)
    {
        memcpy(&metrics->sensors[i], &sensor_list[i], sizeof(SensorInfo));
    }

    return 0;
}

float sensors_get_value(int sensor_id)
{
    if (!sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count)
    {
        return -1.0f;
    }

    return sensor_list[sensor_id].value;
}

int sensors_get_name(int sensor_id, char *buffer, size_t size)
{
    if (!buffer || size == 0 || !sensors_initialized ||
        sensor_id < 0 || sensor_id >= sensor_count)
    {
        return -1;
    }

    strncpy(buffer, sensor_list[sensor_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

SensorType sensors_get_type(int sensor_id)
{
    if (!sensors_initialized || sensor_id < 0 || sensor_id >= sensor_count)
    {
        return SENSOR_TYPE_UNKNOWN;
    }

    return sensor_list[sensor_id].type;
}

#endif /* _WIN32 */