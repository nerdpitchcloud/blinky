#!/bin/bash
# Blinky Collector Uninstaller Script

INSTALL_DIR="/usr/local/bin"
CONFIG_DIR="/etc/blinky"
STORAGE_DIR="/var/lib/blinky/collector"
SERVICE_FILE="/etc/systemd/system/blinky-collector.service"

echo "Blinky Collector Uninstaller"
echo "============================="
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges"
    echo "Please run with sudo"
    exit 1
fi

if [ ! -f "$INSTALL_DIR/blinky-collector" ]; then
    echo "Blinky collector is not installed at $INSTALL_DIR/blinky-collector"
    exit 1
fi

echo "This will remove:"
echo "  - Binary: $INSTALL_DIR/blinky-collector"
echo "  - Config: $CONFIG_DIR/collector.toml"
echo "  - Storage: $STORAGE_DIR"
echo "  - Service: $SERVICE_FILE (if exists)"
echo ""

read -p "Are you sure you want to uninstall Blinky Collector? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled"
    exit 0
fi

echo ""
echo "Uninstalling Blinky Collector..."
echo ""

if [ -f "$SERVICE_FILE" ]; then
    echo "Stopping blinky-collector service..."
    systemctl stop blinky-collector 2>/dev/null || true
    
    echo "Disabling blinky-collector service..."
    systemctl disable blinky-collector 2>/dev/null || true
    
    echo "Removing service file..."
    rm -f "$SERVICE_FILE"
    
    systemctl daemon-reload
fi

echo "Checking for running blinky-collector processes..."
if pgrep -x "blinky-collector" > /dev/null; then
    echo "Stopping running blinky-collector processes..."
    pkill -TERM -x "blinky-collector"
    sleep 2
    
    if pgrep -x "blinky-collector" > /dev/null; then
        echo "Force killing remaining processes..."
        pkill -KILL -x "blinky-collector"
    fi
fi

if [ -f "/var/run/blinky-collector.pid" ]; then
    echo "Removing PID file..."
    rm -f /var/run/blinky-collector.pid
fi

if [ -f "$INSTALL_DIR/blinky-collector" ]; then
    echo "Removing binary..."
    rm -f "$INSTALL_DIR/blinky-collector"
fi

echo ""
read -p "Remove collector configuration file? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing configuration..."
    rm -f "$CONFIG_DIR/collector.toml"
    
    # Remove config dir if empty
    if [ -d "$CONFIG_DIR" ] && [ -z "$(ls -A $CONFIG_DIR)" ]; then
        rmdir "$CONFIG_DIR"
    fi
else
    echo "Keeping configuration at $CONFIG_DIR/collector.toml"
fi

echo ""
read -p "Remove collected metrics in $STORAGE_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing collected metrics..."
    rm -rf "$STORAGE_DIR"
else
    echo "Keeping collected metrics at $STORAGE_DIR"
fi

echo ""
echo "Uninstall complete!"
echo ""

REMAINING=""
if [ -f "$CONFIG_DIR/collector.toml" ]; then
    REMAINING="${REMAINING}  - Configuration: $CONFIG_DIR/collector.toml\n"
fi
if [ -d "$STORAGE_DIR" ]; then
    REMAINING="${REMAINING}  - Collected metrics: $STORAGE_DIR\n"
fi

if [ -n "$REMAINING" ]; then
    echo "The following items were preserved:"
    echo -e "$REMAINING"
    echo "You can manually remove them if needed."
fi
