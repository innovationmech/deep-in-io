#!/bin/bash

# Cross-platform build script for reactor server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if configure needs to be run
if [ ! -f "Makefile.config" ] || [ ! -f "config.h" ]; then
    print_status "Running configure..."
    if [ -x "./configure" ]; then
        ./configure
    else
        print_error "Configure script not found or not executable"
        exit 1
    fi
fi

# Clean previous build if requested
if [ "$1" = "clean" ]; then
    print_status "Cleaning previous build..."
    make clean
    shift
fi

# Detect number of CPU cores for parallel build
if [ "$(uname)" = "Darwin" ]; then
    CORES=$(sysctl -n hw.ncpu)
elif [ "$(uname)" = "Linux" ]; then
    CORES=$(nproc)
else
    CORES=1
fi

print_status "Building with $CORES parallel jobs..."

# Build the project
if make -j$CORES; then
    print_status "Build successful!"
    
    # Show build info
    echo ""
    echo "Build Information:"
    echo "=================="
    echo "Platform: $(uname -s)"
    echo "Architecture: $(uname -m)"
    echo "Compiler: $(make -s -f - <<< 'all:; @echo $(CC)' CC="${CC:-gcc}")"
    echo "Target: reactor_server"
    echo ""
    
    # Offer to run the server
    if [ "$1" != "norun" ]; then
        read -p "Would you like to run the server now? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            print_status "Starting reactor server..."
            ./reactor_server -p 8080 -i 4 -w 8
        fi
    fi
else
    print_error "Build failed!"
    exit 1
fi