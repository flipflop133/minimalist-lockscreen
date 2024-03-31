#!/bin/bash

BUILD_DIR="build"
BUILD_TYPE="release" # Default build type

export CC=clang
export CXX=clang++

# Check if the build type argument is provided
if [ "$#" -gt 0 ]; then
    BUILD_TYPE="$1"
fi

if [ "$BUILD_TYPE" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    exit 0
fi

if [ "$BUILD_TYPE" != "debug" ] && [ "$BUILD_TYPE" != "release" ]; then
    echo "Invalid build type: $BUILD_TYPE"
    echo "Usage: $0 [debug|release]"
    exit 1
fi

# Check if the build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating $BUILD_DIR directory..."
    mkdir "$BUILD_DIR" || exit 1
fi

cd "$BUILD_DIR" || exit 1

echo "Build type: $BUILD_TYPE"

# Run CMake with the specified build type
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# Build the project
make
