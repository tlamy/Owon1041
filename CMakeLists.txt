cmake_minimum_required(VERSION 3.16)
project(Owon1041 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Try to find Qt in common installation paths
set(QT_COMMON_PATHS
    # Qt6 paths
    "/opt/homebrew/opt/qt@6"
    "/opt/homebrew/opt/qt6"
    "/usr/local/opt/qt@6"
    "/usr/local/opt/qt6"
    "/usr/local/qt6"
    "C:/Qt/6.5.0/msvc2019_64"
    "C:/Qt/6.5.0/mingw_64"
    # Qt5 paths as fallback
    "/opt/homebrew/opt/qt@5"
    "/opt/homebrew/opt/qt5"
    "/usr/local/opt/qt@5"
    "/usr/local/opt/qt5"
    "/usr/local/qt5"
    "C:/Qt/5.15.2/msvc2019_64"
    "C:/Qt/5.15.2/mingw81_64"
)

# Find the first valid Qt path
foreach(QT_PATH ${QT_COMMON_PATHS})
    if(EXISTS "${QT_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
        message(STATUS "Found Qt at: ${QT_PATH}")
        break()
    endif()
endforeach()

# Try to find Qt6 first, fall back to Qt5 if not found
set(QT_VERSION_MAJOR 6)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
    Core
    Gui
    Widgets
    SerialPort
    QUIET
)

# If Qt6 not found, try Qt5
if(NOT Qt${QT_VERSION_MAJOR}_FOUND)
    set(QT_VERSION_MAJOR 5)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
        Core
        Gui
        Widgets
        SerialPort
        REQUIRED
    )
    message(STATUS "Using Qt5 as Qt6 was not found")
else()
    message(STATUS "Using Qt6")
endif()

# Define macOS bundle properties
set(MACOSX_BUNDLE_BUNDLE_NAME "Owon1041")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "de.macwake.Owon1041")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 MacWake. All rights reserved.")
set(MACOSX_BUNDLE_INFO_STRING "OWON XDM-1041 Multimeter Interface")

# Icons and resources setup for macOS
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE Owon.icns)
    set(APP_ICON_MACOSX ${CMAKE_CURRENT_SOURCE_DIR}/Owon.icns)
    set_source_files_properties(${APP_ICON_MACOSX} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

# Add executable
add_executable(Owon1041 MACOSX_BUNDLE
    main.cpp
    MainWindow.cpp
    MainWindow.h
    Settings.cpp
    Settings.h
    ConnectDialog.cpp
    ConnectDialog.h
    ${APP_ICON_MACOSX}
)

target_link_libraries(Owon1041
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
)

# Set additional macOS bundle properties
if(APPLE)
    set_target_properties(Owon1041 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${MACOSX_BUNDLE_BUNDLE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${MACOSX_BUNDLE_LONG_VERSION_STRING}"
        MACOSX_BUNDLE_COPYRIGHT "${MACOSX_BUNDLE_COPYRIGHT}"
        MACOSX_BUNDLE_INFO_STRING "${MACOSX_BUNDLE_INFO_STRING}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
    )
    
    # Set High DPI support
    target_compile_definitions(Owon1041 PRIVATE
        "QT_MAC_WANTS_LAYER=1"
    )
endif()