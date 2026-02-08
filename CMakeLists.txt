cmake_minimum_required(VERSION 3.16)
project(Capture VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore)

include_directories(src/core)

add_executable(Capture
    src/main.cpp
    src/core/browser.cpp
    src/core/tabmanager.cpp
    src/core/browser.h
    src/core/tabmanager.h
)

target_link_libraries(Capture PRIVATE 
    Qt6::Widgets 
    Qt6::WebEngineWidgets 
    Qt6::WebEngineCore
)