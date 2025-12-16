# Blinky Quick Start Guide

Get up and running with Blinky in 5 minutes!

## Prerequisites

On Debian/Ubuntu systems:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev git
```

## Installation

### 1. Clone and Build

```bash
git clone https://github.com/nerdpitchcloud/blinky.git
cd blinky
make
```

This will build both the agent and collector.

### 2. Start the Collector

On your monitoring server:

```bash
cd build
./collector/blinky-collector
```

You should see:
```
Blinky Collector v0.1.0
WebSocket server port: 9090
HTTP dashboard port: 9091

Collector is running...
Dashboard available at: http://localhost:9091/
```

### 3. Start the Agent

On the same machine (for testing) or on remote hosts:

```bash
cd build
./agent/blinky-agent -s localhost -p 9090
```

You should see:
```
Blinky Agent v0.1.0
Connecting to collector at localhost:9090
Collection interval: 5 seconds

Attempting to connect to collector...
Connected to collector
Metrics sent successfully
```

### 4. View the Dashboard

Open your browser and navigate to:
```
http://localhost:9091/
```

You should see your host with real-time metrics!

## Testing on Multiple Hosts

### On the Collector Host

1. Start the collector:
```bash
./collector/blinky-collector -w 9090 -p 9091
```

2. Note the IP address:
```bash
ip addr show
```

### On Each Monitored Host

1. Copy the agent binary:
```bash
scp build/agent/blinky-agent user@remote-host:/tmp/
```

2. SSH to the remote host and run:
```bash
/tmp/blinky-agent -s <collector-ip> -p 9090 -i 5
```

3. Check the dashboard - you should see the new host appear!

## Installing as a Service

### Install the Collector

On your monitoring server:
```bash
sudo ./install.sh collector
sudo systemctl daemon-reload
sudo systemctl enable blinky-collector
sudo systemctl start blinky-collector
sudo systemctl status blinky-collector
```

### Install the Agent

On each monitored host:

1. Edit the service file to point to your collector:
```bash
sudo ./install.sh agent
sudo nano /etc/systemd/system/blinky-agent.service
```

2. Change the ExecStart line:
```
ExecStart=/usr/local/bin/blinky-agent -s <collector-ip> -p 9090 -i 5
```

3. Start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable blinky-agent
sudo systemctl start blinky-agent
sudo systemctl status blinky-agent
```

## Monitoring Features

Once running, Blinky will automatically monitor:

- **System Resources**: CPU, memory, disk usage
- **SMART Data**: Disk health (if smartctl is installed)
- **Network**: Interface statistics
- **Systemd Services**: Service status
- **Docker Containers**: If Docker is running
- **Podman Containers**: If Podman is installed
- **Kubernetes**: If k8s or k3s is detected

## Troubleshooting

### Agent can't connect to collector

Check firewall rules:
```bash
sudo ufw allow 9090/tcp
sudo ufw allow 9091/tcp
```

### Dashboard shows "No hosts connected"

1. Check if collector is running:
```bash
sudo systemctl status blinky-collector
```

2. Check if agent is running:
```bash
sudo systemctl status blinky-agent
```

3. Check agent logs:
```bash
sudo journalctl -u blinky-agent -f
```

### SMART data not showing

Install smartmontools:
```bash
sudo apt-get install -y smartmontools
```

### Container monitoring not working

Ensure the agent has permission to access Docker socket:
```bash
sudo usermod -aG docker $USER
```

Or run the agent as root (for systemd services, it runs as root by default).

## Next Steps

- Configure collection intervals with `-i` flag
- Set up multiple agents across your infrastructure
- Access metrics via REST API at `http://collector:9091/api/metrics`
- Customize the dashboard (see docs/CUSTOMIZATION.md)

## Getting Help

- Check the main README.md for detailed documentation
- Report issues on GitHub: https://github.com/nerdpitchcloud/blinky/issues
- Review logs: `journalctl -u blinky-agent` or `journalctl -u blinky-collector`
