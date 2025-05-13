# Owon XDM1041 Control Software

This application provides a graphical interface for controlling and retrieving measurements from the Owon XDM1041 digital multimeter.

## Features

- Connect to the Owon XDM1041 via serial port
- Control all measurement functions (Voltage, Current, Resistance, etc.)
- Cycle through measurement ranges including auto-range
- Display real-time measurements
- Graph measurement data over time
- Dual measurement display
- Save and load measurement sessions
- Menu functionality for additional features

## Screenshots

(Screenshots will be added here)

## Pre-compiled Binary

For Windows users who want to run the application without compiling it, a pre-compiled executable is included in the `bin` directory.

## Repository Structure

This repository has two main branches:

- `source-code-only`: Contains only the source code files needed for compilation
- `main`: Contains the full project including compiled binaries

If you are interested in just reviewing or modifying the code, use the `source-code-only` branch. If you want to run the application without compiling, use the `main` branch and find the executable in the `bin` directory.

## Building from Source

### Prerequisites

- Qt 5 (tested with Qt 5.15)
- CMake 3.10 or higher
- C++ compiler with C++11 support (GCC, MSVC, Clang)

### Build Instructions

#### Windows

1. Clone the repository:

   ```bash
   git clone https://github.com/RepairYourTech/Owon1041.git
   cd Owon1041
   ```

2. Create a build directory:

   ```bash
   mkdir build
   cd build
   ```

3. Configure with CMake:

   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=C:/Qt5/5.15.2/msvc2019_64
   ```

   Note: Adjust the Qt path according to your installation.

4. Build the project:

   ```bash
   cmake --build . --config Release
   ```

5. The executable will be created in the `build/Release` directory.

#### Linux

1. Clone the repository:

   ```bash
   git clone https://github.com/RepairYourTech/Owon1041.git
   cd Owon1041
   ```

2. Create a build directory:

   ```bash
   mkdir build
   cd build
   ```

3. Configure with CMake:

   ```bash
   cmake ..
   ```

4. Build the project:

   ```bash
   make
   ```

5. The executable will be created in the `build` directory.

### Dependencies

- Qt Core
- Qt Widgets
- Qt SerialPort
- Qt Charts (for graphing functionality)

## Usage

1. Connect your Owon XDM1041 to your computer via USB
2. Launch the application
3. Select the correct COM port from the connection dialog
4. Click "Connect"
5. Use the buttons to select measurement functions and ranges

## Recent Changes

- Fixed range cycling logic to properly cycle through all available ranges
- Implemented menu functionality
- Added dual measurement display toggle
- Improved graph scaling

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Original code by Thomas Lamy (tlamy)
- Improvements by RepairYourTech
- Based on the Owon XDM1041 Programming Guide
