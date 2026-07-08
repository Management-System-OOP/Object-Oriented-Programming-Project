# @file   CMakeLists.txt
# @brief  Build configuration and dependency management for the project.
# @author Duong Anh Hao
# @date   2026-06-15

cmake_minimum_required(VERSION 3.21)
project(WarehouseMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# Force MSVC to fully conform to the C++ standard
if (MSVC)    
    add_compile_options(/Zc:__cplusplus)
endif()

#  Use forward slashes / to ajust the path to your local envỉonment
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH "QT file")

# Enable Qt auto-compilation features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Locate the required Qt6 Core library
find_package(Qt6 COMPONENTS Core REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/entities
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/states
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
)

# Gather all source files automatically
file(GLOB_RECURSE SOURCES 
    "src/domain/entities/*.cpp"
    "src/domain/states/*.cpp"
    "src/repository/*.cpp"
    "src/service/*.cpp"
    "main.cpp"
)

# Create the main executable
add_executable(${PROJECT_NAME} ${SOURCES})

#Link to the project
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core)
