#!/bin/bash

BUILD_DIR="build"
BUILD_TYPE="release" # Default build type

export CC=clang
export CXX=clang++

# Check if the build type argument is provided
if [ "$#" -gt 0 ]; then
    BUILD_TYPE=$(echo "$1" | tr '[:upper:]' '[:lower:]') # Normalize to lowercase
fi

if [ "$BUILD_TYPE" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    exit 0
fi

if [ "$BUILD_TYPE" = "debug" ]; then
    BUILD_TYPE="Debug"
elif [ "$BUILD_TYPE" = "release" ]; then
    BUILD_TYPE="Release"
else
    echo "Invalid build type: $BUILD_TYPE"
    echo "Usage: $0 [debug|release|clean]"
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
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" .. || {
    echo "CMake configuration failed."
    exit 1
}

# Build the project
make || {
    echo "Build failed."
    exit 1
}

echo "Build successful. Output in $BUILD_DIR."
