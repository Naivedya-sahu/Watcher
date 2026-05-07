cmake_minimum_required(VERSION 3.16)

# Pull v7.1 components — only what the peripheral test needs
set(EXTRA_COMPONENT_DIRS "../components")
set(COMPONENTS main button buzzer encoder log freertos)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_peripheral)
