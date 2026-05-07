idf_component_register(
    SRCS
        "epd.cpp"
        "gxepd2/GxEPD2_EPD.cpp"
        "gxepd2/gdey/GxEPD2_420_GDEY042T81.cpp"
    INCLUDE_DIRS "." "gxepd2" "gxepd2/gdey"
    REQUIRES esp_driver_spi esp_driver_gpio esp_timer log
)
