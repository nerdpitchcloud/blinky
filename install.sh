#!/bin/bash

set -e

echo "Blinky Installation Script"
echo "=========================="
echo ""

if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root or with sudo"
    exit 1
fi

INSTALL_DIR="/usr/local/bin"
SYSTEMD_DIR="/etc/systemd/system"

show_usage() {
    echo "Usage: $0 [agent|collector|both]"
    echo ""
    echo "Options:"
    echo "  agent      - Install only the agent"
    echo "  collector  - Install only the collector"
    echo "  both       - Install both agent and collector"
    exit 1
}

if [ $# -eq 0 ]; then
    show_usage
fi

MODE=$1

if [ ! -d "build" ]; then
    echo "Build directory not found. Please build the project first:"
    echo "  mkdir build && cd build && cmake .. && make"
    exit 1
fi

install_agent() {
    echo "Installing blinky-agent..."
    
    if [ ! -f "build/agent/blinky-agent" ]; then
        echo "Error: blinky-agent binary not found"
        exit 1
    fi
    
    cp build/agent/blinky-agent "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/blinky-agent"
    
    echo "Creating systemd service..."
    cat > "$SYSTEMD_DIR/blinky-agent.service" << EOF
[Unit]
Description=Blinky Monitoring Agent
After=network.target

[Service]
Type=simple
ExecStart=$INSTALL_DIR/blinky-agent -s localhost -p 9090 -i 5
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    
    echo ""
    echo "Agent installed successfully!"
    echo "Edit /etc/systemd/system/blinky-agent.service to configure collector host"
    echo "Then run:"
    echo "  systemctl daemon-reload"
    echo "  systemctl enable blinky-agent"
    echo "  systemctl start blinky-agent"
    echo ""
}

install_collector() {
    echo "Installing blinky-collector..."
    
    if [ ! -f "build/collector/blinky-collector" ]; then
        echo "Error: blinky-collector binary not found"
        exit 1
    fi
    
    cp build/collector/blinky-collector "$INSTALL_DIR/"
    chmod +x "$INSTALL_DIR/blinky-collector"
    
    echo "Creating systemd service..."
    cat > "$SYSTEMD_DIR/blinky-collector.service" << EOF
[Unit]
Description=Blinky Monitoring Collector
After=network.target

[Service]
Type=simple
ExecStart=$INSTALL_DIR/blinky-collector -w 9090 -p 9091
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    
    echo ""
    echo "Collector installed successfully!"
    echo "To start the collector:"
    echo "  systemctl daemon-reload"
    echo "  systemctl enable blinky-collector"
    echo "  systemctl start blinky-collector"
    echo ""
    echo "Dashboard will be available at: http://localhost:9091/"
    echo ""
}

case "$MODE" in
    agent)
        install_agent
        ;;
    collector)
        install_collector
        ;;
    both)
        install_agent
        install_collector
        ;;
    *)
        show_usage
        ;;
esac

echo "Installation complete!"
