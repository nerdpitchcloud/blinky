#!/bin/bash
set -e

REPO="nerdpitchcloud/blinky"
INSTALL_DIR="/usr/local/bin"
BINARY_NAME="blinky-collector"

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
        download_url=$(curl -s https://api.github.com/repos/$REPO/releases/latest | grep "browser_download_url.*blinky-collector.*linux-$arch.tar.gz" | cut -d '"' -f 4)
    else
        download_url="https://github.com/$REPO/releases/download/$version/blinky-collector-${version#v}-linux-$arch.tar.gz"
    fi
    
    if [ -z "$download_url" ]; then
        return 1
    fi
    
    echo "Downloading from: $download_url"
    
    local tmp_dir=$(mktemp -d)
    cd $tmp_dir
    
    if ! curl -L -f -o blinky-collector.tar.gz "$download_url" 2>/dev/null; then
        rm -rf $tmp_dir
        return 1
    fi
    
    echo "Extracting archive..."
    tar -xzf blinky-collector.tar.gz
    
    # Check for binary in both root and collector/ subdirectory
    if [ -f "blinky-collector" ]; then
        BINARY_PATH="blinky-collector"
    elif [ -f "collector/blinky-collector" ]; then
        BINARY_PATH="collector/blinky-collector"
    else
        echo "Error: Binary not found in archive"
        rm -rf $tmp_dir
        return 1
    fi
    
    echo "Installing to $INSTALL_DIR..."
    mv "$BINARY_PATH" $INSTALL_DIR/blinky-collector
    chmod +x $INSTALL_DIR/blinky-collector
    
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
    
    if [ ! -f "collector/blinky-collector" ]; then
        echo "Error: Build failed - binary not found"
        cd /
        rm -rf $tmp_dir
        exit 1
    fi
    
    echo ""
    echo "Installing to $INSTALL_DIR..."
    cp collector/blinky-collector $INSTALL_DIR/
    chmod +x $INSTALL_DIR/blinky-collector
    
    cd /
    rm -rf $tmp_dir
}

ARCH=$(detect_arch)
OS=$(detect_os)

echo "Blinky Collector Installer"
echo "==========================="
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

if install_from_binary "$VERSION" "$ARCH"; then
    echo ""
    echo "Installation complete (from pre-built binary)"
else
    echo "Failed to download pre-built binary for $ARCH, falling back to source build..."
    echo ""
    install_from_source
    echo ""
    echo "Installation complete (built from source)"
fi

echo ""
echo "Setting up configuration..."

CONFIG_DIR="/etc/blinky"
CONFIG_FILE="$CONFIG_DIR/collector.toml"
STORAGE_DIR="/var/lib/blinky/collector"

mkdir -p "$CONFIG_DIR"
mkdir -p "$STORAGE_DIR"
chmod 755 "$STORAGE_DIR"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Creating default config at $CONFIG_FILE..."
    cat > "$CONFIG_FILE" << 'EOF'
# Blinky Collector Configuration File

[collector]
# WebSocket server port for receiving metrics from agents
ws_port = 9090

# HTTP API server port
http_port = 9091

# Bind address (0.0.0.0 for all interfaces)
bind_address = "0.0.0.0"

[storage]
# Storage path for collected metrics
path = "/var/lib/blinky/collector"

# Maximum number of metrics to keep per host in memory
max_metrics_per_host = 1000

# Enable persistent storage to disk
persistent = true

# Maximum age of metrics in hours (0 = unlimited)
max_age_hours = 24

[api]
# Enable REST API
enabled = true

# API authentication (optional)
# auth_token = "your-secret-token"

[logging]
# Log level: debug, info, warn, error
level = "info"

# Log output: stdout, stderr, file
output = "stdout"

# Enable timestamps in logs
timestamps = true

# Enable colored output
colors = true
EOF
    chmod 644 "$CONFIG_FILE"
    echo "Default config created at $CONFIG_FILE"
else
    echo "Config file already exists at $CONFIG_FILE (not overwriting)"
fi

echo ""
echo "Installation complete!"
echo ""
echo "Installed version:"
$INSTALL_DIR/blinky-collector --version
echo ""
echo "Configuration file: $CONFIG_FILE"
echo "Storage directory: $STORAGE_DIR"
echo ""
echo "The collector provides:"
echo "  - WebSocket server on port 9090 (for agents)"
echo "  - HTTP API on port 9091 (for querying metrics)"
echo ""
echo "To run the collector:"
echo "  blinky-collector"
echo ""
echo "API Endpoints:"
echo "  GET /api/metrics        - Get all host metrics"
echo "  GET /api/hosts          - List connected hosts"
echo "  GET /health             - Health check"
echo ""
echo "Configure your agents to connect:"
echo "  [collector]"
echo "  enabled = true"
echo "  host = \"<this-server-ip>\""
echo "  port = 9090"
echo ""
echo "For more information visit: https://github.com/$REPO"
