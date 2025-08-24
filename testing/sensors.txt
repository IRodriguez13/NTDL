SENSOR_MAPPING = {
    "Intel": {
        "temperature": ["CPU Package", "Core Max", "Core Average"],
        "voltage": ["CPU Core", "CPU Package"],
        "clock": ["CPU Core #1"],
        "load": ["CPU Total"]
    },
    "AMD": {
        "temperature": ["Core (Tctl/Tdie)", "CCD1 (Tdie)", "SoC", "CPU Cores"],
        "voltage": ["Core (SVI2 TFN)", "SoC (SVI2 TFN)", "CPU Cores"],
        "clock": ["Core #1", "Fabric", "Uncore"],
        "load": ["CPU Total", "CPU Core Max"]
    }
}