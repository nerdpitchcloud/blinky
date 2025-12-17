#!/bin/bash
# Blinky Agent Uninstaller Script

INSTALL_DIR="/usr/local/bin"
SYSTEM_CONFIG_DIR="/etc/blinky"
STORAGE_DIR="/var/lib/blinky"
SERVICE_FILE="/etc/systemd/system/blinky-agent.service"

# Determine user config directory
if [ -n "$SUDO_USER" ]; then
    USER_HOME=$(eval echo ~$SUDO_USER)
    USER_CONFIG_DIR="$USER_HOME/.blinky"
else
    USER_CONFIG_DIR=""
fi

echo "Blinky Agent Uninstaller"
echo "========================"
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Error: This script requires root privileges"
    echo "Please run with sudo"
    exit 1
fi

if [ ! -f "$INSTALL_DIR/blinky-agent" ]; then
    echo "Blinky agent is not installed at $INSTALL_DIR/blinky-agent"
    exit 1
fi

echo "This will remove:"
echo "  - Binary: $INSTALL_DIR/blinky-agent"
if [ -d "$USER_CONFIG_DIR" ]; then
    echo "  - User config: $USER_CONFIG_DIR"
fi
if [ -d "$SYSTEM_CONFIG_DIR" ]; then
    echo "  - System config: $SYSTEM_CONFIG_DIR"
fi
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

if [ -f "$SERVICE_FILE" ]; then
    echo "Stopping blinky-agent service..."
    systemctl stop blinky-agent 2>/dev/null || true
    
    echo "Disabling blinky-agent service..."
    systemctl disable blinky-agent 2>/dev/null || true
    
    echo "Removing service file..."
    rm -f "$SERVICE_FILE"
    
    systemctl daemon-reload
fi

if [ -f "$INSTALL_DIR/blinky-agent" ]; then
    echo "Removing binary..."
    rm -f "$INSTALL_DIR/blinky-agent"
fi

echo ""
if [ -d "$USER_CONFIG_DIR" ]; then
    read -p "Remove user configuration in $USER_CONFIG_DIR? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing user configuration..."
        rm -rf "$USER_CONFIG_DIR"
    else
        echo "Keeping user configuration at $USER_CONFIG_DIR"
    fi
fi

if [ -d "$SYSTEM_CONFIG_DIR" ]; then
    echo ""
    read -p "Remove system configuration in $SYSTEM_CONFIG_DIR? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing system configuration..."
        rm -rf "$SYSTEM_CONFIG_DIR"
    else
        echo "Keeping system configuration at $SYSTEM_CONFIG_DIR"
    fi
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

REMAINING=""
if [ -d "$USER_CONFIG_DIR" ]; then
    REMAINING="${REMAINING}  - User configuration: $USER_CONFIG_DIR\n"
fi
if [ -d "$SYSTEM_CONFIG_DIR" ]; then
    REMAINING="${REMAINING}  - System configuration: $SYSTEM_CONFIG_DIR\n"
fi
if [ -d "$STORAGE_DIR" ]; then
    REMAINING="${REMAINING}  - Stored metrics: $STORAGE_DIR\n"
fi

if [ -n "$REMAINING" ]; then
    echo "The following items were preserved:"
    echo -e "$REMAINING"
    echo "You can manually remove them if needed."
fi
