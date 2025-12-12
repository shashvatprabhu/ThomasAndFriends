# ESP-IDF Project - LSV Potentiostat for ESP32-WROOM-32E
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lsv_potentiostat)

# Set custom partition table
set(EXTRA_PARTITION_CSV "${CMAKE_CURRENT_SOURCE_DIR}/partition_table.csv")
