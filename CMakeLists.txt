cmake_minimum_required(VERSION 3.31)
project(Owon1041)

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

add_executable(Owon1041 main.cpp
    MainWindow.cpp
    MainWindow.h
    Settings.cpp
    Settings.h
    ConnectDialog.cpp
    ConnectDialog.h
)

target_link_libraries(Owon1041
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
)