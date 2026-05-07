idf_component_register(
    SRCS "test_peripheral_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES button buzzer encoder log freertos
)
