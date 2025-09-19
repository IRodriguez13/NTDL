/**
 * @file network_windows.c
 * @brief Implementación de red para Windows usando IP Helper API y WMI
 */

#ifdef _WIN32

#include "network_interface.h"
#include <windows.h>
#include <iphlpapi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "iphlpapi.lib")

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static BOOL network_initialized = FALSE;
static NetworkInterface network_list[MAX_NETWORK_INTERFACES];
static int network_count = 0;

/* ============================================================================
 * FUNCIONES AUXILIARES
 * ============================================================================ */

/**
 * @brief Convierte dirección MAC de bytes a string
 */
static void mac_bytes_to_string(BYTE *mac_bytes, int length, char *mac_str, size_t mac_str_size) {
    if (length >= 6) {
        snprintf(mac_str, mac_str_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac_bytes[0], mac_bytes[1], mac_bytes[2],
                mac_bytes[3], mac_bytes[4], mac_bytes[5]);
    } else {
        strncpy(mac_str, "00:00:00:00:00:00", mac_str_size - 1);
        mac_str[mac_str_size - 1] = '\0';
    }
}

/**
 * @brief Obtiene información de interfaces usando IP Helper API
 */
static int get_network_interfaces_iphelper(void) {
    PIP_ADAPTER_INFO adapter_info = NULL;
    PIP_ADAPTER_INFO adapter = NULL;
    DWORD buffer_size = 0;
    DWORD result;
    
    // Obtener tamaño del buffer necesario
    result = GetAdaptersInfo(NULL, &buffer_size);
    if (result != ERROR_BUFFER_OVERFLOW) {
        return -1;
    }
    
    // Asignar memoria
    adapter_info = (PIP_ADAPTER_INFO)malloc(buffer_size);
    if (!adapter_info) {
        return -1;
    }
    
    // Obtener información de adaptadores
    result = GetAdaptersInfo(adapter_info, &buffer_size);
    if (result != ERROR_SUCCESS) {
        free(adapter_info);
        return -1;
    }
    
    network_count = 0;
    adapter = adapter_info;
    
    // Procesar cada adaptador
    while (adapter && network_count < MAX_NETWORK_INTERFACES) {
        NetworkInterface *net_if = &network_list[network_count];
        memset(net_if, 0, sizeof(NetworkInterface));
        
        // Nombre del adaptador
        strncpy(net_if->name, adapter->AdapterName, sizeof(net_if->name) - 1);
        
        // Descripción
        strncpy(net_if->description, adapter->Description, sizeof(net_if->description) - 1);
        
        // Dirección MAC
        mac_bytes_to_string(adapter->Address, adapter->AddressLength, 
                           net_if->mac_address, sizeof(net_if->mac_address));
        
        // Dirección IP (primera en la lista)
        if (adapter->IpAddressList.IpAddress.String[0] != '\0') {
            strncpy(net_if->ip_address, adapter->IpAddressList.IpAddress.String, 
                   sizeof(net_if->ip_address) - 1);
        }
        
        // Máscara de subred
        if (adapter->IpAddressList.IpMask.String[0] != '\0') {
            strncpy(net_if->netmask, adapter->IpAddressList.IpMask.String, 
                   sizeof(net_if->netmask) - 1);
        }
        
        // Gateway
        if (adapter->GatewayList.IpAddress.String[0] != '\0') {
            strncpy(net_if->gateway, adapter->GatewayList.IpAddress.String, 
                   sizeof(net_if->gateway) - 1);
        }
        
        // Tipo de adaptador
        switch (adapter->Type) {
            case MIB_IF_TYPE_ETHERNET:
                strncpy(net_if->type, "Ethernet", sizeof(net_if->type) - 1);
                break;
            case MIB_IF_TYPE_PPP:
                strncpy(net_if->type, "PPP", sizeof(net_if->type) - 1);
                break;
            case MIB_IF_TYPE_LOOPBACK:
                strncpy(net_if->type, "Loopback", sizeof(net_if->type) - 1);
                break;
            default:
                if (strstr(adapter->Description, "Wireless") || 
                    strstr(adapter->Description, "WiFi") ||
                    strstr(adapter->Description, "802.11")) {
                    strncpy(net_if->type, "WiFi", sizeof(net_if->type) - 1);
                    net_if->is_wireless = 1;
                } else {
                    strncpy(net_if->type, "Other", sizeof(net_if->type) - 1);
                }
                break;
        }
        
        // Estado (simplificado)
        net_if->is_up = 1; // Asumir que está activo si aparece en la lista
        
        // Estadísticas (inicializar en 0, se actualizarán después)
        net_if->bytes_sent = 0;
        net_if->bytes_received = 0;
        net_if->packets_sent = 0;
        net_if->packets_received = 0;
        net_if->errors_in = 0;
        net_if->errors_out = 0;
        net_if->speed_mbps = -1.0f; // No disponible en esta API
        
        network_count++;
        adapter = adapter->Next;
    }
    
    free(adapter_info);
    return network_count > 0 ? 0 : -1;
}

/**
 * @brief Obtiene estadísticas de red usando MIB
 */
static int get_network_statistics(void) {
    PMIB_IFTABLE if_table = NULL;
    DWORD size = 0;
    DWORD result;
    
    // Obtener tamaño necesario
    result = GetIfTable(NULL, &size, FALSE);
    if (result != ERROR_INSUFFICIENT_BUFFER) {
        return -1;
    }
    
    // Asignar memoria
    if_table = (PMIB_IFTABLE)malloc(size);
    if (!if_table) {
        return -1;
    }
    
    // Obtener tabla de interfaces
    result = GetIfTable(if_table, &size, FALSE);
    if (result != NO_ERROR) {
        free(if_table);
        return -1;
    }
    
    // Actualizar estadísticas para cada interfaz
    for (DWORD i = 0; i < if_table->dwNumEntries && i < (DWORD)network_count; i++) {
        MIB_IFROW *if_row = &if_table->table[i];
        
        // Buscar la interfaz correspondiente por descripción
        for (int j = 0; j < network_count; j++) {
            NetworkInterface *net_if = &network_list[j];
            
            // Comparación simplificada por índice
            if (i == (DWORD)j) {
                net_if->bytes_sent = if_row->dwOutOctets;
                net_if->bytes_received = if_row->dwInOctets;
                net_if->packets_sent = if_row->dwOutUcastPkts + if_row->dwOutNUcastPkts;
                net_if->packets_received = if_row->dwInUcastPkts + if_row->dwInNUcastPkts;
                net_if->errors_in = if_row->dwInErrors;
                net_if->errors_out = if_row->dwOutErrors;
                
                // Velocidad
                if (if_row->dwSpeed > 0) {
                    net_if->speed_mbps = (float)(if_row->dwSpeed / 1000000.0);
                }
                
                // Estado operacional
                net_if->is_up = (if_row->dwOperStatus == IF_OPER_STATUS_OPERATIONAL);
                
                break;
            }
        }
    }
    
    free(if_table);
    return 0;
}

/* ============================================================================
 * IMPLEMENTACIÓN DE LA INTERFAZ
 * ============================================================================ */

int network_init(void) {
    if (network_initialized) {
        return 0;
    }
    
    // Limpiar lista de interfaces
    memset(network_list, 0, sizeof(network_list));
    network_count = 0;
    
    // Obtener información de interfaces
    if (get_network_interfaces_iphelper() != 0) {
        return -1;
    }
    
    // Obtener estadísticas
    get_network_statistics();
    
    network_initialized = TRUE;
    return 0;
}

void network_cleanup(void) {
    network_initialized = FALSE;
    network_count = 0;
}

int network_get_count(void) {
    return network_initialized ? network_count : -1;
}

int network_get_interface_info(int interface_id, NetworkInterface *info) {
    if (!info || !network_initialized || interface_id < 0 || interface_id >= network_count) {
        return -1;
    }
    
    memcpy(info, &network_list[interface_id], sizeof(NetworkInterface));
    return 0;
}

int network_get_metrics(NetworkMetrics *metrics) {
    if (!metrics || !network_initialized) {
        return -1;
    }
    
    // Actualizar estadísticas antes de devolver métricas
    get_network_statistics();
    
    // Limpiar estructura
    memset(metrics, 0, sizeof(NetworkMetrics));
    
    metrics->num_interfaces = network_count;
    
    // Copiar información de todas las interfaces
    for (int i = 0; i < network_count && i < MAX_NETWORK_INTERFACES; i++) {
        memcpy(&metrics->interfaces[i], &network_list[i], sizeof(NetworkInterface));
    }
    
    return 0;
}

int network_get_interface_name(int interface_id, char *buffer, size_t size) {
    if (!buffer || size == 0 || !network_initialized || 
        interface_id < 0 || interface_id >= network_count) {
        return -1;
    }
    
    strncpy(buffer, network_list[interface_id].name, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

float network_get_bytes_sent(int interface_id) {
    if (!network_initialized || interface_id < 0 || interface_id >= network_count) {
        return -1.0f;
    }
    
    return (float)network_list[interface_id].bytes_sent;
}

float network_get_bytes_received(int interface_id) {
    if (!network_initialized || interface_id < 0 || interface_id >= network_count) {
        return -1.0f;
    }
    
    return (float)network_list[interface_id].bytes_received;
}

int network_is_interface_up(int interface_id) {
    if (!network_initialized || interface_id < 0 || interface_id >= network_count) {
        return -1;
    }
    
    return network_list[interface_id].is_up ? 1 : 0;
}

#endif /* _WIN32 */