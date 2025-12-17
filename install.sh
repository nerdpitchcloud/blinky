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
    
    # Check for binary in both root and agent/ subdirectory
    if [ -f "blinky-agent" ]; then
        BINARY_PATH="blinky-agent"
    elif [ -f "agent/blinky-agent" ]; then
        BINARY_PATH="agent/blinky-agent"
    else
        echo "Error: Binary not found in archive"
        rm -rf $tmp_dir
        return 1
    fi
    
    echo "Installing to $INSTALL_DIR..."
    mv "$BINARY_PATH" $INSTALL_DIR/blinky-agent
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
CONFIG_FILE="$CONFIG_DIR/config.toml"
STORAGE_DIR="/var/lib/blinky/metrics"

mkdir -p "$CONFIG_DIR"
mkdir -p "$STORAGE_DIR"
chmod 755 "$STORAGE_DIR"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Creating default config at $CONFIG_FILE..."
    cat > "$CONFIG_FILE" << 'EOF'
# Blinky Agent Configuration File
# Edit this file to configure your agent

[agent]
# Operating mode: local, push, pull, hybrid
# - local: Store metrics locally only (default)
# - push: Push metrics to collector
# - pull: Local storage + HTTP API for pulling
# - hybrid: Both push and local storage
mode = "pull"

# Collection interval in seconds
interval = 5

[storage]
# Local storage path for metrics
path = "/var/lib/blinky/metrics"

# Maximum number of metric files to keep
max_files = 100

# Maximum file size in MB before rotation
max_file_size_mb = 10

[api]
# Enable HTTP API for pull-based collection
enabled = true

# API server port
port = 9092

# Bind address (0.0.0.0 for all interfaces)
bind_address = "0.0.0.0"

[collector]
# Enable pushing to collector (for push/hybrid modes)
enabled = false

# Collector server hostname or IP address
host = "localhost"

# Collector WebSocket port
port = 9090

# Connection timeout in seconds
timeout = 10

[collector.reconnect]
# Enable automatic reconnection
enabled = true

# Initial retry delay in seconds
initial_delay = 5

# Maximum retry delay in seconds
max_delay = 300

# Backoff multiplier (exponential backoff)
backoff_multiplier = 2.0

# Maximum number of reconnection attempts (0 = unlimited)
max_attempts = 0

[logging]
# Log level: debug, info, warn, error
level = "info"

# Log output: stdout, stderr, file
output = "stdout"

# Enable timestamps in logs
timestamps = true

# Enable colored output
colors = true

[agent.monitors]
cpu = true
memory = true
disk = true
smart = true
network = true
systemd = true
containers = true
kubernetes = true

[disk]
# Exclude specific mount points from monitoring
exclude_mounts = [
    "/dev",
    "/dev/shm",
    "/run",
    "/sys/fs/cgroup",
    "/snap"
]

# Exclude filesystems by type
exclude_fs_types = [
    "tmpfs",
    "devtmpfs",
    "squashfs",
    "overlay"
]

[network]
# Exclude specific interfaces
exclude_interfaces = [
    "lo",
    "docker0"
]

[containers]
# Enable Docker monitoring
docker = true

# Docker socket path
docker_socket = "/var/run/docker.sock"

# Enable Podman monitoring
podman = true

# Podman socket path
podman_socket = "/run/podman/podman.sock"

[kubernetes]
# Enable Kubernetes monitoring
enabled = true
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
$INSTALL_DIR/blinky-agent --version
echo ""
echo "Configuration file: $CONFIG_FILE"
echo "Storage directory: $STORAGE_DIR"
echo ""
echo "The agent is configured in 'pull' mode by default:"
echo "  - Metrics stored locally in $STORAGE_DIR"
echo "  - HTTP API available on port 9092"
echo "  - Access metrics: curl http://localhost:9092/metrics"
echo ""
echo "To run the agent:"
echo "  blinky-agent"
echo ""
echo "To change mode, edit $CONFIG_FILE:"
echo "  mode = \"local\"  - Local storage only"
echo "  mode = \"pull\"   - Local storage + HTTP API (default)"
echo "  mode = \"push\"   - Push to collector"
echo "  mode = \"hybrid\" - Both push and local storage"
echo ""
echo "For push/hybrid modes, configure collector:"
echo "  [collector]"
echo "  enabled = true"
echo "  host = \"your-collector-host\""
echo ""
echo "For more information visit: https://github.com/$REPO"
