# @file   CMakeLists.txt
# @brief  Build configuration and dependency management for the project.
# @author Duong Anh Hao
# @date   2026-06-15

# @update
# @author Nguyen Viet Bach
# @date   2026-06-23
# @changelog  
#    - Add configuration, Qt6 multi-module dependencies, and automatic source file "GUI" gathering.


cmake_minimum_required(VERSION 3.21)
project(WarehouseMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

# Force MSVC to fully conform to the C++ standard
if (MSVC)    
    add_compile_options(/Zc:__cplusplus)
endif()

# Use forward slashes / to adjust the path to your local environment
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64" CACHE PATH "QT file")

# Enable Qt auto-compilation features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Locate the required Qt6 libraries including UI components
find_package(Qt6 COMPONENTS Core Widgets Gui Charts REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Gather all source and UI files automatically with dependency tracking to update cache
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/domain/entities/*.h" "src/domain/entities/*.cpp"
    "src/domain/states/*.h" "src/domain/states/*.cpp"
    "src/repository/*.h" "src/repository/*.cpp"
    "src/service/*.h" "src/service/*.cpp"
    "src/gui/*.h" "src/gui/*.cpp" "src/gui/*.ui"
    "src/gui/dialogs/*.h" "src/gui/dialogs/*.cpp" 
    "main.cpp"
)

# Create the main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link to the required Qt6 modules
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/data/packages.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/data/packages.json"
    COMMENT "Deploying default package data file"
)
