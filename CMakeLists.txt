cmake_minimum_required(VERSION 3.16)
project(appimagemanager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(appimagemanager
    src/main.cpp
    src/AppImageManager.cpp
    src/MainWindow.cpp
)

target_include_directories(appimagemanager PRIVATE include)

target_link_libraries(appimagemanager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

include(GNUInstallDirs)
install(TARGETS appimagemanager RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
