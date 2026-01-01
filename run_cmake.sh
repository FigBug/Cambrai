#!/bin/bash

set -e

cd "$(dirname "$0")"

if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Generating Xcode project..."
    cmake -B build -G Xcode
    echo ""
    echo "Xcode project created at: build/Cambrai.xcodeproj"
    echo "Run: open build/Cambrai.xcodeproj"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    echo "Generating Visual Studio 2025 project..."
    cmake -B build -G "Visual Studio 18 2025"
    echo ""
    echo "Visual Studio solution created at: build/Cambrai.sln"
else
    echo "Generating Makefiles..."
    cmake -B build -G "Unix Makefiles"
    echo ""
    echo "Build with: cmake --build build"
fi
