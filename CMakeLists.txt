cmake_minimum_required(VERSION 3.31)
project(Owon1041)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add Homebrew Qt5 path
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@5")

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        SerialPort
        REQUIRED)

add_executable(Owon1041 main.cpp
        MainWindow.cpp
        MainWindow.h
        Settings.cpp
        Settings.h
        ConnectDialog.cpp
        ConnectDialog.h
)
target_link_libraries(Owon1041
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::SerialPort
)