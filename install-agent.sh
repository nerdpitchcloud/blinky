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

ARCH=$(detect_arch)
OS=$(detect_os)

echo "Detected OS: $OS"
echo "Detected architecture: $ARCH"

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

echo "Installing Blinky Agent version: $VERSION"

DOWNLOAD_URL="https://github.com/$REPO/releases/download/$VERSION/blinky-agent-*-linux-$ARCH.tar.gz"

if [ "$VERSION" = "latest" ]; then
    echo "Fetching latest release information"
    DOWNLOAD_URL=$(curl -s https://api.github.com/repos/$REPO/releases/latest | grep "browser_download_url.*blinky-agent.*linux-$ARCH.tar.gz" | cut -d '"' -f 4)
    
    if [ -z "$DOWNLOAD_URL" ]; then
        echo "Error: Could not find latest release for $ARCH"
        exit 1
    fi
fi

echo "Downloading from: $DOWNLOAD_URL"

TMP_DIR=$(mktemp -d)
cd $TMP_DIR

curl -L -o blinky-agent.tar.gz "$DOWNLOAD_URL"

if [ $? -ne 0 ]; then
    echo "Error: Failed to download binary"
    rm -rf $TMP_DIR
    exit 1
fi

echo "Extracting archive"
tar -xzf blinky-agent.tar.gz

if [ ! -f "blinky-agent" ]; then
    echo "Error: Binary not found in archive"
    rm -rf $TMP_DIR
    exit 1
fi

echo "Installing to $INSTALL_DIR"
mv blinky-agent $INSTALL_DIR/
chmod +x $INSTALL_DIR/blinky-agent

rm -rf $TMP_DIR

echo "Installation complete"
echo ""
echo "Installed version:"
$INSTALL_DIR/blinky-agent --version
echo ""
echo "To run the agent:"
echo "  blinky-agent -s <collector-host> -p 8080"
echo ""
echo "For more information visit: https://github.com/$REPO"
