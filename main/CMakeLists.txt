set(SOURCE_FILES_LIST
    "main.cpp"
    "Application/NetworkManager/NetworkManager.cpp"
    "Application/NetworkManager/NetworkInterface.cpp"
    "Application/NetworkManager/WiFiInterface.cpp"
    "Application/NetworkManager/DnsConfiguration.cpp"
    "Application/WebServerManager/WebServerManager.cpp"
    "Application/WebServerManager/StaticFileHandler.cpp"
    "Application/WebServerManager/WebSocketHandler.cpp"
    "Application/CommandManager/CommandManager.cpp"
    "Application/UpdateManager/UpdateManager.cpp"
    "Application/SettingsManager/SettingsManager.cpp"
    "Application/LogManager/LogManager.cpp"
    "Application/DisplayManager/DisplayManager.cpp"
    "Application/DisplayManager/DisplaySettings.cpp"
    "hardware/display/Display_WT32SC01.cpp"
    "Application/SensorManager/SensorManager.cpp"
    "Application/TemperatureHistory/TemperatureHistory.cpp"
    "Application/TimeManager/TimeManager.cpp"
    "lib/system/DateTime.cpp"
    "lib/system/TimeSpan.cpp"
)

set(INCLUDE_DIRS_LIST
    .
    "Application"
    "Application/NetworkManager"
    "Application/WebServerManager"
    "Application/CommandManager"
    "Application/UpdateManager"
    "Application/SettingsManager"
    "Application/LogManager"
    "Application/DisplayManager"
    "Application/SensorManager"
    "Application/TemperatureHistory"
    "Application/TimeManager"
    "hardware"
    "hardware/display"
    "lib"
    "lib/common"
    "lib/rtos"
    "lib/json"
    "lib/system"
)

set(COMPONENT_REQUIRES
    esp_timer
    esp_netif
    esp_event
    esp_wifi
    nvs_flash
    esp_http_server
    esp_app_format
    app_update
    esp_partition
    fatfs
    wear_levelling
    esp_driver_spi
    esp_driver_ledc
    esp_driver_i2c
    esp_driver_gpio
    onewire_bus
    esp_lcd
    lvgl
    esp_lcd_touch
    mdns
)

# Register component with ESP-IDF
idf_component_register(
    SRCS ${SOURCE_FILES_LIST}
    INCLUDE_DIRS ${INCLUDE_DIRS_LIST}
    REQUIRES ${COMPONENT_REQUIRES}
)
