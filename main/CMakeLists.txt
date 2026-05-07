idf_component_register(
    SRCS
        "main.cpp"
        "screen_mgr.cpp"
        "ota_task.cpp"
        "screens/clock_screen.cpp"
        "screens/alarm_screen.cpp"
        "screens/pomo_screen.cpp"
        "screens/cal_screen.cpp"
        "screens/tasks_screen.cpp"
        "screens/settings_screen.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        epd fb fonts gfx
        button buzzer
        encoder
        time_svc
        config_store
        web_server
        nvs_flash
        esp_wifi esp_event esp_netif
        lwip log freertos
        spiffs
        app_update
        esp_https_ota
        esp_http_client
)
