#!/bin/bash

# Audio Visualizer Build Script
# Simple wrapper for CMake build process

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Audio Visualizer...${NC}"

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: CMake is not installed or not in your PATH.${NC}"
    echo ""
    echo "Please install CMake first:"
    echo "  macOS: brew install cmake"
    echo "  Linux: sudo apt-get install cmake (Ubuntu/Debian)"
    echo "         sudo dnf install cmake (Fedora/RHEL)"
    echo ""
    echo "Or run the dependency installation script:"
    echo "  ./scripts/install_deps.sh"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake ..

# Determine number of CPU cores for parallel build
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4  # Default fallback
fi

# Build
echo -e "${YELLOW}Building with $CORES cores...${NC}"
make -j$CORES

echo -e "${GREEN}Build complete!${NC}"
echo -e "${GREEN}Run with: ./build/audio_visualizer${NC}"

