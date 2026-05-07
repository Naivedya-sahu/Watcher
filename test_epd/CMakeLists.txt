cmake_minimum_required(VERSION 3.16)

# Pull v7.1 components into this standalone test project
set(EXTRA_COMPONENT_DIRS "../components")

# Limit build to ONLY what the test needs — prevents web_server/button/etc.
# from being scanned (web_server requires 'json' which may not exist in IDF v6)
set(COMPONENTS main epd fb fonts)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_epd)
