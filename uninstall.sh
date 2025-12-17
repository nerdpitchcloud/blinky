#!/bin/bash

INSTALL_DIR="/usr/local/bin"
CONFIG_DIR="/etc/blinky"
STORAGE_DIR="/var/lib/blinky"
SERVICE_FILE="/etc/systemd/system/blinky-agent.service"

echo "Blinky Agent Uninstaller"
echo "========================"
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges"
    echo "Please run with sudo"
    exit 1
fi

# Check if blinky-agent is installed
if [ ! -f "$INSTALL_DIR/blinky-agent" ]; then
    echo "Blinky agent is not installed at $INSTALL_DIR/blinky-agent"
    exit 1
fi

echo "This will remove:"
echo "  - Binary: $INSTALL_DIR/blinky-agent"
echo "  - Config: $CONFIG_DIR"
echo "  - Storage: $STORAGE_DIR"
echo "  - Service: $SERVICE_FILE (if exists)"
echo ""

read -p "Are you sure you want to uninstall Blinky? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled"
    exit 0
fi

echo ""
echo "Uninstalling Blinky..."
echo ""

# Stop and disable service if it exists
if [ -f "$SERVICE_FILE" ]; then
    echo "Stopping blinky-agent service..."
    systemctl stop blinky-agent 2>/dev/null || true
    
    echo "Disabling blinky-agent service..."
    systemctl disable blinky-agent 2>/dev/null || true
    
    echo "Removing service file..."
    rm -f "$SERVICE_FILE"
    
    systemctl daemon-reload
fi

# Remove binary
if [ -f "$INSTALL_DIR/blinky-agent" ]; then
    echo "Removing binary..."
    rm -f "$INSTALL_DIR/blinky-agent"
fi

# Ask about config and data
echo ""
read -p "Remove configuration files in $CONFIG_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing configuration..."
    rm -rf "$CONFIG_DIR"
else
    echo "Keeping configuration at $CONFIG_DIR"
fi

echo ""
read -p "Remove stored metrics in $STORAGE_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing stored metrics..."
    rm -rf "$STORAGE_DIR"
else
    echo "Keeping stored metrics at $STORAGE_DIR"
fi

echo ""
echo "Uninstall complete!"
echo ""

# Show what remains
REMAINING=""
if [ -d "$CONFIG_DIR" ]; then
    REMAINING="${REMAINING}  - Configuration: $CONFIG_DIR\n"
fi
if [ -d "$STORAGE_DIR" ]; then
    REMAINING="${REMAINING}  - Stored metrics: $STORAGE_DIR\n"
fi

if [ -n "$REMAINING" ]; then
    echo "The following items were preserved:"
    echo -e "$REMAINING"
    echo "You can manually remove them if needed."
fi
