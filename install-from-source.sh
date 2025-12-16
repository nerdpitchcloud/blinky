#!/bin/bash

set -e

REPO="nerdpitchcloud/blinky"
INSTALL_DIR="/usr/local/bin"

echo "Blinky Agent - Build from Source Installer"
echo "=========================================="
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges"
    echo "Please run with sudo"
    exit 1
fi

ARCH=$(uname -m)
echo "Detected architecture: $ARCH"
echo ""

echo "Installing build dependencies..."
apt-get update
apt-get install -y build-essential cmake libssl-dev git

echo ""
echo "Cloning repository..."
TMP_DIR=$(mktemp -d)
cd $TMP_DIR

git clone https://github.com/$REPO.git
cd blinky

LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "main")
echo "Building version: $LATEST_TAG"

if [ "$LATEST_TAG" != "main" ]; then
    git checkout $LATEST_TAG
fi

echo ""
echo "Building binaries..."
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

if [ ! -f "agent/blinky-agent" ]; then
    echo "Error: Build failed - binary not found"
    exit 1
fi

echo ""
echo "Installing to $INSTALL_DIR"
cp agent/blinky-agent $INSTALL_DIR/
chmod +x $INSTALL_DIR/blinky-agent

cd /
rm -rf $TMP_DIR

echo ""
echo "Installation complete"
echo ""
echo "Installed version:"
$INSTALL_DIR/blinky-agent --version
echo ""
echo "To run the agent:"
echo "  blinky-agent -s <collector-host> -p 9090"
echo ""
echo "For more information visit: https://github.com/$REPO"
