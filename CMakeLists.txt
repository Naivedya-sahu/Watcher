cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(watcher VERSION 7.1.0)

# Flash SPIFFS image alongside firmware
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)
