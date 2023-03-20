cmake_minimum_required(VERSION 3.1.0)
project(Fritzing VERSION 0.6.4 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build application
add_subdirectory(src)

# Build parts library
add_custom_target(fritzing_parts 
  COMMAND fritzing -db "${CMAKE_CURRENT_BINARY_DIR}/parts.db" -pp ${CMAKE_SOURCE_DIR}/parts
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/parts.db)