idf_component_register(
    SRCS
        "web_server.cpp"
        "cJSON_bundled.c"
    INCLUDE_DIRS "."
    REQUIRES
        fb
        buzzer
        config_store
        time_svc
        log
        esp_http_server
        spiffs
        app_update
)
