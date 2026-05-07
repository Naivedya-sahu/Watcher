idf_component_register(
    SRCS "buzzer.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_ledc log
)
