idf_component_register(
    SRCS "test_epd_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES epd fb fonts log freertos
)
