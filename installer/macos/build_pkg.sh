#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$SCRIPT_DIR/../.."

# Read version
VERSION=$(cat "$PROJECT_DIR/VERSION" | tr -d '[:space:]')

# Paths
APP_PATH="$PROJECT_DIR/build/Cambrai.app"
PKG_OUTPUT="$PROJECT_DIR/build/Cambrai-${VERSION}.pkg"
COMPONENT_PKG="$PROJECT_DIR/build/Cambrai-component.pkg"

# Check app exists
if [ ! -d "$APP_PATH" ]; then
    echo "Error: $APP_PATH not found. Build the project first."
    exit 1
fi

echo "Building installer for Cambrai $VERSION..."

# Create component package
pkgbuild \
    --root "$APP_PATH" \
    --install-location "/Applications/Cambrai.app" \
    --identifier "com.cambrai.game" \
    --version "$VERSION" \
    "$COMPONENT_PKG"

# Create product archive (final installer)
productbuild \
    --package "$COMPONENT_PKG" \
    --identifier "com.cambrai.game.pkg" \
    --version "$VERSION" \
    "$PKG_OUTPUT"

# Clean up component package
rm "$COMPONENT_PKG"

echo "Created: $PKG_OUTPUT"
