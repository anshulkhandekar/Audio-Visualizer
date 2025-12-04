#!/bin/bash

# Audio Visualizer Dependency Installation Script
# Supports macOS (Homebrew) and Linux (apt-get, dnf, pacman)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Try to detect Linux distribution
        if [ -f /etc/debian_version ]; then
            echo "debian"
        elif [ -f /etc/redhat-release ]; then
            if command -v dnf &> /dev/null; then
                echo "fedora"
            else
                echo "rhel"
            fi
        elif [ -f /etc/arch-release ]; then
            echo "arch"
        else
            echo "linux"
        fi
    else
        echo "unknown"
    fi
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install dependencies on macOS
install_macos() {
    print_info "Detected macOS"
    
    if ! command_exists brew; then
        print_error "Homebrew not found. Please install Homebrew first:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
    
    print_info "Installing dependencies via Homebrew..."
    
    brew install cmake qtbase qtcharts fftw mad portaudio
    
    print_info "macOS dependencies installed successfully!"
}

# Install dependencies on Debian/Ubuntu
install_debian() {
    print_info "Detected Debian/Ubuntu Linux"
    
    if ! command_exists apt-get; then
        print_error "apt-get not found"
        exit 1
    fi
    
    print_info "Updating package list..."
    sudo apt-get update
    
    print_info "Installing dependencies..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        qt6-base-dev \
        qt6-charts-dev \
        libfftw3-dev \
        libmad0-dev \
        portaudio19-dev
    
    print_info "Debian/Ubuntu dependencies installed successfully!"
}

# Install dependencies on Fedora/RHEL
install_fedora() {
    print_info "Detected Fedora/RHEL Linux"
    
    if command_exists dnf; then
        PKG_MGR="dnf"
    elif command_exists yum; then
        PKG_MGR="yum"
    else
        print_error "Neither dnf nor yum found"
        exit 1
    fi
    
    print_info "Installing dependencies via $PKG_MGR..."
    
    sudo $PKG_MGR install -y \
        gcc-c++ \
        make \
        cmake \
        qt6-qtbase-devel \
        qt6-qtcharts-devel \
        fftw-devel \
        libmad-devel \
        portaudio-devel
    
    print_info "Fedora/RHEL dependencies installed successfully!"
}

# Install dependencies on Arch Linux
install_arch() {
    print_info "Detected Arch Linux"
    
    if ! command_exists pacman; then
        print_error "pacman not found"
        exit 1
    fi
    
    print_info "Installing dependencies..."
    sudo pacman -S --needed \
        base-devel \
        cmake \
        qt6-base \
        qt6-charts \
        fftw \
        libmad \
        portaudio
    
    print_info "Arch Linux dependencies installed successfully!"
}

# Main installation logic
main() {
    print_info "Audio Visualizer Dependency Installer"
    print_info "===================================="
    
    OS=$(detect_os)
    
    case $OS in
        macos)
            install_macos
            ;;
        debian)
            install_debian
            ;;
        fedora|rhel)
            install_fedora
            ;;
        arch)
            install_arch
            ;;
        *)
            print_error "Unsupported operating system: $OSTYPE"
            echo ""
            echo "Please install the following dependencies manually:"
            echo "  - CMake 3.16+"
            echo "  - Qt6 (Core, Widgets, Gui, Charts)"
            echo "  - FFTW3"
            echo "  - libmad"
            echo "  - PortAudio"
            echo "  - C++ compiler with C++17 support"
            exit 1
            ;;
    esac
    
    print_info ""
    print_info "All dependencies installed successfully!"
    print_info "You can now build the project:"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make"
}

# Run main function
main

