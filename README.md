# Audio-Visualizer
Attempting to build a working audio visualizer and real-time audio editor purely in C/C++.

## Description
This program decodes MP3 audio files into PCM data, performs FFT analysis using FFTW3, and provides real-time audio visualization and editing capabilities through a Qt-based GUI.

(Milestone 1 Done) The program will:

1. Decode the MP3 file into PCM samples
2. Process samples in chunks of 1024 (FFT_SIZE)
3. Perform FFT analysis on each chunk
4. Calculate frequency magnitudes
5. Write results to `output.txt`

(Milestone 2 Done) The program will:

1. Load File button - Opens a file dialog to select MP3 or WAV files
2. Play button - Starts playback and visualization
3. Pause button - Pauses/resumes playback
4. Stop button - Stops playback and resets visualization
5. Frequency Spectrum Chart - Real-time bar chart showing frequency magnitudes

(Milestone 3 Done) The program will:

1. Sliders to edit out frequencies
2. Low-pass and high-pass editing features
3. Band-stop editing feature
4. Radial plot, histogram, and line plot options
5. Real-time playback of editing

(Milestone 4 Done) The program will:

1. Provide a cleaner visualization for radial plot 
2. Option to export edited audio to WAV file
3. Reduced static on band-stop editing feature
4. Status messages for exporting and loading
5. Can load files back to back without leaking resources

## How to run the program:

### Prerequisites

- **CMake** 3.16 or higher
- **C++ compiler** with C++17 support (GCC 7+, Clang 5+, or MSVC 2017+)
- **Qt6** (Core, Widgets, Gui, Charts)
- **FFTW3** library
- **libmad** library
- **PortAudio** library

### Quick Start

1. **Install dependencies** (automated):
   ```bash
   ./scripts/install_deps.sh
   ```

2. **Build the project**:
   ```bash
   # Option 1: Use the build script (recommended)
   ./build.sh
   
   # Option 2: Manual build
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run the application**:
   ```bash
   ./build/audio_visualizer
   ```

### Platform-Specific Instructions

#### macOS

**Using Homebrew** (recommended):
```bash
# Install dependencies (includes CMake)
brew install cmake qtbase qtcharts fftw mad portaudio

# Or use the automated script:
./scripts/install_deps.sh

# Build
./build.sh
```

**Manual Installation**:
If Homebrew is not available, install the dependencies manually and ensure CMake can find them. You may need to set `CMAKE_PREFIX_PATH`:
```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/qt6 ..
```

#### Linux

**Ubuntu/Debian**:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake \
    qt6-base-dev qt6-charts-dev \
    libfftw3-dev libmad0-dev portaudio19-dev

./build.sh
```

**Fedora/RHEL**:
```bash
sudo dnf install -y gcc-c++ make cmake \
    qt6-qtbase-devel qt6-qtcharts-devel \
    fftw-devel libmad-devel portaudio-devel

./build.sh
```

**Arch Linux**:
```bash
sudo pacman -S --needed base-devel cmake \
    qt6-base qt6-charts \
    fftw libmad portaudio

./build.sh
```
