idf_component_register(
    SRCS "time_svc.cpp"
    INCLUDE_DIRS "."
    REQUIRES lwip esp_netif esp_wifi log
)
