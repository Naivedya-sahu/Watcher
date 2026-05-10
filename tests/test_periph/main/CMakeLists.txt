idf_component_register(
    SRCS "test_periph.c"
    INCLUDE_DIRS "."
    REQUIRES 
    esp_driver_gpio
    esp_driver_ledc
    log 
    freertos
)
