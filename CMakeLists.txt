cmake_minimum_required(VERSION 3.16)
project(AudioVisualizer VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Charts)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt6 not found. Please install Qt6:\n"
            "  macOS: brew install qtbase qtcharts\n"
            "  Linux: apt-get install qt6-base-dev qt6-charts-dev (Ubuntu/Debian)\n"
            "         dnf install qt6-qtbase-devel qt6-qtcharts-devel (Fedora/RHEL)")
endif()

# Enable Qt6 automatic MOC, UIC, RCC
qt6_standard_project_setup()

# Find FFTW3
find_library(FFTW3_LIB 
    NAMES fftw3
    PATHS 
        /opt/homebrew/opt/fftw/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

find_path(FFTW3_INCLUDE_DIR
    NAMES fftw3.h
    PATHS
        /opt/homebrew/opt/fftw/include
        /usr/include
        /usr/local/include
)

if(NOT FFTW3_LIB OR NOT FFTW3_INCLUDE_DIR)
    message(FATAL_ERROR "FFTW3 library not found. Please install:\n"
            "  macOS: brew install fftw\n"
            "  Linux: apt-get install libfftw3-dev (Ubuntu/Debian)\n"
            "         dnf install fftw-devel (Fedora/RHEL)")
endif()

# Find libmad
find_library(MAD_LIB
    NAMES mad
    PATHS
        /opt/homebrew/opt/mad/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

find_path(MAD_INCLUDE_DIR
    NAMES mad.h
    PATHS
        /opt/homebrew/opt/mad/include
        /usr/include
        /usr/local/include
)

if(NOT MAD_LIB OR NOT MAD_INCLUDE_DIR)
    message(FATAL_ERROR "libmad library not found. Please install:\n"
            "  macOS: brew install mad\n"
            "  Linux: apt-get install libmad0-dev (Ubuntu/Debian)\n"
            "         dnf install libmad-devel (Fedora/RHEL)")
endif()

# Find PortAudio
find_library(PORTAUDIO_LIB
    NAMES portaudio
    PATHS
        /opt/homebrew/opt/portaudio/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

find_path(PORTAUDIO_INCLUDE_DIR
    NAMES portaudio.h
    PATHS
        /opt/homebrew/opt/portaudio/include
        /usr/include
        /usr/local/include
)

if(NOT PORTAUDIO_LIB OR NOT PORTAUDIO_INCLUDE_DIR)
    message(FATAL_ERROR "PortAudio library not found. Please install:\n"
            "  macOS: brew install portaudio\n"
            "  Linux: apt-get install portaudio19-dev (Ubuntu/Debian)\n"
            "         dnf install portaudio-devel (Fedora/RHEL)")
endif()

# Platform-specific handling
if(APPLE)
    # macOS frameworks
    find_library(COREAUDIO CoreAudio)
    find_library(AUDIOTOOLBOX AudioToolbox)
    find_library(COREFOUNDATION CoreFoundation)
    
    if(NOT COREAUDIO OR NOT AUDIOTOOLBOX OR NOT COREFOUNDATION)
        message(FATAL_ERROR "Required macOS frameworks not found")
    endif()
endif()

# Include directories
include_directories(
    ${FFTW3_INCLUDE_DIR}
    ${MAD_INCLUDE_DIR}
    ${PORTAUDIO_INCLUDE_DIR}
)

# Source files
set(SOURCES
    main.cpp
    AudioDecoder.cpp
    FFTAnalyzer.cpp
    AudioPlayer.cpp
    MainWindow.cpp
    FrequencyFilter.cpp
    RadialVisualizationWidget.cpp
    AudioExporter.cpp
)

# Headers with Q_OBJECT macro (need MOC processing)
set(HEADERS_WITH_MOC
    MainWindow.h
    AudioPlayer.h
    RadialVisualizationWidget.h
)

# Auto-generate MOC files
qt6_wrap_cpp(MOC_SOURCES ${HEADERS_WITH_MOC})

# Create executable
add_executable(audio_visualizer ${SOURCES} ${MOC_SOURCES})

# Link Qt6 libraries
target_link_libraries(audio_visualizer
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Charts
)

# Link audio libraries
target_link_libraries(audio_visualizer
    ${FFTW3_LIB}
    ${MAD_LIB}
    ${PORTAUDIO_LIB}
    m  # Math library
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(audio_visualizer
        ${COREAUDIO}
        ${AUDIOTOOLBOX}
        ${COREFOUNDATION}
    )
endif()

# Set output directory (optional)
set_target_properties(audio_visualizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

