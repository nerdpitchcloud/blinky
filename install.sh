#!/bin/bash
set -e

REPO="nerdpitchcloud/blinky"
INSTALL_DIR="/usr/local/bin"
BINARY_NAME="blinky-agent"

detect_arch() {
    local arch=$(uname -m)
    case $arch in
        x86_64)
            echo "amd64"
            ;;
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            echo "Error: Unsupported architecture: $arch" >&2
            exit 1
            ;;
    esac
}

detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    else
        echo "unknown"
    fi
}

install_from_binary() {
    local version=$1
    local arch=$2
    
    echo "Attempting to install from pre-built binary..."
    
    local download_url
    if [ "$version" = "latest" ]; then
        echo "Fetching latest release information..."
        download_url=$(curl -s https://api.github.com/repos/$REPO/releases/latest | grep "browser_download_url.*blinky-agent.*linux-$arch.tar.gz" | cut -d '"' -f 4)
    else
        download_url="https://github.com/$REPO/releases/download/$version/blinky-agent-${version#v}-linux-$arch.tar.gz"
    fi
    
    if [ -z "$download_url" ]; then
        return 1
    fi
    
    echo "Downloading from: $download_url"
    
    local tmp_dir=$(mktemp -d)
    cd $tmp_dir
    
    if ! curl -L -f -o blinky-agent.tar.gz "$download_url" 2>/dev/null; then
        rm -rf $tmp_dir
        return 1
    fi
    
    echo "Extracting archive..."
    tar -xzf blinky-agent.tar.gz
    
    if [ ! -f "blinky-agent" ]; then
        echo "Error: Binary not found in archive"
        rm -rf $tmp_dir
        return 1
    fi
    
    echo "Installing to $INSTALL_DIR..."
    mv blinky-agent $INSTALL_DIR/
    chmod +x $INSTALL_DIR/blinky-agent
    
    rm -rf $tmp_dir
    return 0
}

install_from_source() {
    echo "Building from source..."
    echo ""
    
    echo "Installing build dependencies..."
    apt-get update -qq
    apt-get install -y build-essential cmake libssl-dev git
    
    echo ""
    echo "Cloning repository..."
    local tmp_dir=$(mktemp -d)
    cd $tmp_dir
    
    git clone --quiet https://github.com/$REPO.git
    cd blinky
    
    local latest_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "main")
    echo "Building version: $latest_tag"
    
    if [ "$latest_tag" != "main" ]; then
        git checkout --quiet $latest_tag
    fi
    
    echo ""
    echo "Compiling binaries (this may take a few minutes)..."
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release .. > /dev/null
    make -j$(nproc)
    
    if [ ! -f "agent/blinky-agent" ]; then
        echo "Error: Build failed - binary not found"
        cd /
        rm -rf $tmp_dir
        exit 1
    fi
    
    echo ""
    echo "Installing to $INSTALL_DIR..."
    cp agent/blinky-agent $INSTALL_DIR/
    chmod +x $INSTALL_DIR/blinky-agent
    
    cd /
    rm -rf $tmp_dir
}

ARCH=$(detect_arch)
OS=$(detect_os)

echo "Blinky Agent Installer"
echo "====================="
echo ""
echo "Detected OS: $OS"
echo "Detected architecture: $ARCH"
echo ""

if [ "$OS" != "debian" ] && [ "$OS" != "ubuntu" ]; then
    echo "Warning: This installer is designed for Debian/Ubuntu systems"
    echo "It may work on other distributions but is not officially supported"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges"
    echo "Please run with sudo"
    exit 1
fi

VERSION=${1:-latest}
echo "Target version: $VERSION"
echo ""

if [ "$ARCH" = "amd64" ]; then
    if install_from_binary "$VERSION" "$ARCH"; then
        echo ""
        echo "Installation complete (from pre-built binary)"
    else
        echo "Failed to download pre-built binary, falling back to source build..."
        echo ""
        install_from_source
        echo ""
        echo "Installation complete (built from source)"
    fi
elif [ "$ARCH" = "arm64" ]; then
    echo "ARM64 detected - pre-built binaries not available"
    echo "Will build from source instead"
    echo ""
    install_from_source
    echo ""
    echo "Installation complete (built from source)"
else
    echo "Error: Unsupported architecture: $ARCH"
    exit 1
fi

echo ""
echo "Installed version:"
$INSTALL_DIR/blinky-agent --version
echo ""
echo "To run the agent:"
echo "  blinky-agent -s <collector-host> -p 9090"
echo ""
echo "For more information visit: https://github.com/$REPO"
