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
    src/Preferences.cpp
    src/SettingsDialog.cpp
    src/TranslationManager.cpp
    include/AppImageManager/MainWindow.h
    include/AppImageManager/Preferences.h
    include/AppImageManager/SettingsDialog.h
    include/AppImageManager/TranslationManager.h
    resources/assets.qrc
    resources/translations.qrc
)

target_include_directories(appimagemanager PRIVATE include)

target_link_libraries(appimagemanager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_compile_definitions(appimagemanager PRIVATE APPIMAGEMANAGER_VERSION="${PROJECT_VERSION}")

add_executable(appimagemanager-daemon
    src/DaemonMain.cpp
    src/AutostartDaemon.cpp
    src/AppImageManager.cpp
    include/AppImageManager/AutostartDaemon.h
    include/AppImageManager/AppImageManager.h
)

target_include_directories(appimagemanager-daemon PRIVATE include)

target_link_libraries(appimagemanager-daemon PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(appimagemanager-daemon PRIVATE APPIMAGEMANAGER_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS appimagemanager RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS appimagemanager-daemon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES resources/appimagemanager.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES resources/icons/appimagemanager.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/1024x1024/apps)
install(FILES resources/icons/appimagemanager.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)

configure_file(resources/systemd/appimagemanager-daemon.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/appimagemanager-daemon.service @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/appimagemanager-daemon.service
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/systemd/user)
