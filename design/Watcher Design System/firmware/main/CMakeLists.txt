# firmware/main/CMakeLists.txt
idf_component_register(
    SRCS
        "main.c"
        "watcher_nvs.c"
        "watcher_http.c"
        "watcher_epd.c"
        "watcher_buttons.c"
    INCLUDE_DIRS "."
    EMBED_FILES "../data/index.html"
    REQUIRES
        nvs_flash esp_wifi esp_http_server esp_event esp_netif
        spi_flash driver json esp_timer
)
