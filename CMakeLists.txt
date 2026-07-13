# @file   CMakeLists.txt
# @brief  Build configuration and dependency management for the project.
# @author Duong Anh Hao
# @date   2026-06-15
# 
# @update
# @author Do Minh Khang
# @date   2026-07-11
# @changelog
#   - Remove hardcoded absolute path for Qt
#   - Change from include_directories to target_include_directories
#     and re-positioning it after the creating executable
#   - Add headers (.h) to the build
# 

cmake_minimum_required(VERSION 3.21)
project(WarehouseMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# Force MSVC to fully conform to the C++ standard
if (MSVC)    
    add_compile_options(/Zc:__cplusplus)
endif()

# Use forward slashes / to ajust the path to your local envỉonment
# set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH "QT file")

# Look for Qt6 in standard locations first, fallback to environment variable
if(NOT CMAKE_PREFIX_PATH)
    if(DEFINED ENV{Qt6_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}" CACHE PATH "Qt6 installation path")
    else()
        message(WARNING "Qt6_DIR environment variable not set. Please set it to your Qt6 installation directory.")
    endif()
endif()

# Enable Qt auto-compilation features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Locate the required Qt6 Core library
find_package(Qt6 COMPONENTS Core REQUIRED)

file(GLOB_RECURSE HEADERS
    "src/domain/**/*.h"
    "src/repository/**/*.h"
    "src/service/**/*.h"
    "src/gui/**/*.h"
)

file(GLOB_RECURSE SOURCES
    "src/domain/**/*.cpp"
    "src/repository/**/*.cpp"
    "src/service/**/*.cpp"
    "src/gui/**/*.cpp"
    "main.cpp"
)

# Create the main executable
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/entities
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/states
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    #${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

#Link to the project
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core)
