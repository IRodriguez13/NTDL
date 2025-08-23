#!/usr/bin/env python3
"""
Ejemplo simple de uso del wrapper de Python para sysmon
"""

from sysmon_python import SysMon, get_system_metrics, get_system_metrics_dict
import time

def main():
    print("=== Ejemplo Simple de SysMon ===\n")
    
    # Método 1: Usando funciones de conveniencia (más simple)
    print("1. Usando funciones de conveniencia:")
    print("-" * 40)
    
    # Obtener métricas como objeto
    metrics = get_system_metrics()
    print(f"Sistema: {metrics.os_name}")
    print(f"CPU: {metrics.cpu_model} ({metrics.cpu_cores} cores)")
    print(f"Uso CPU: {metrics.cpu_usage:.1f}%")
    print(f"Uso RAM: {metrics.ram_usage_percent:.1f}%")
    print(f"Disco: {metrics.disk_model}")
    
    # Obtener como diccionario
    metrics_dict = get_system_metrics_dict()
    print(f"\nComo diccionario: {metrics_dict['cpu_usage_percent']:.1f}% CPU")
    

    
    print("\n" + "="*50 + "\n")
    
    # Método 2: Usando la clase SysMon (más control)
    print("2. Usando la clase SysMon:")
    print("-" * 40)
    
    # Usando context manager (recomendado)
    with SysMon() as sysmon:
        print(f"OS: {sysmon.get_os_name()}")
        print(f"CPU Modelo: {sysmon.get_cpu_model()}")
        print(f"CPU Cores: {sysmon.get_cpu_cores()}")
        print(f"CPU Uso: {sysmon.get_cpu_usage():.1f}%")
        print(f"RAM Total: {sysmon.get_ram_total() / (1024*1024):.1f} GB")
        print(f"RAM Libre: {sysmon.get_ram_free() / (1024*1024):.1f} GB")
        print(f"RAM Uso: {sysmon.get_ram_usage_percent():.1f}%")
        print(f"Disco: {sysmon.get_disk_model()}")
        print(f"Disco Salud: {sysmon.get_disk_health()}%")
    
    print("\n" + "="*50 + "\n")
    
    # Método 3: Monitoreo continuo
    print("3. Monitoreo continuo (5 segundos):")
    print("-" * 40)
    
    with SysMon() as sysmon:
        for i in range(5):
            cpu = sysmon.get_cpu_usage()
            ram = sysmon.get_ram_usage_percent()
            print(f"Tiempo {i+1}: CPU {cpu:.1f}% | RAM {ram:.1f}%")
            time.sleep(1)
    
    print("\n¡Listo!")

if __name__ == "__main__":
    main()
