# Blinky Quick Start Guide

Get up and running with Blinky in 5 minutes!

## Quick Installation

### 1. Install the Agent

On each host you want to monitor:

```bash
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/install.sh | sudo bash
```

The installer automatically:
- Detects your architecture (AMD64/ARM64)
- Downloads pre-built binaries for AMD64
- Builds from source for ARM64

Start the agent:
```bash
blinky-agent -s <collector-ip> -p 9090 -i 5
```

You should see:
```
Blinky Agent v0.1.1
Connecting to collector at <collector-ip>:9090
Collection interval: 5 seconds

Attempting to connect to collector...
Connected to collector
Metrics sent successfully
```

### 2. Run the Collector (Docker)

On your monitoring server, create a `docker-compose.yml`:

```yaml
version: '3.8'
services:
  blinky-collector:
    image: ghcr.io/nerdpitchcloud/blinky-collector:latest
    ports:
      - "9090:9090"  # WebSocket
      - "9091:9091"  # Dashboard
    restart: unless-stopped
```

Start the collector:
```bash
docker-compose up -d
```

### 3. View the Dashboard

Open your browser and navigate to:
```
http://<collector-ip>:9091/
```

You should see your host with real-time metrics!

## Alternative: Build from Source

If you prefer to build from source:

```bash
git clone https://github.com/nerdpitchcloud/blinky.git
cd blinky
make
```

Then run the binaries from the build directory:
```bash
./build/collector/blinky-collector -w 9090 -p 9091
./build/agent/blinky-agent -s <collector-ip> -p 9090 -i 5
```

## Running as a Service

### Collector Service

Create `/etc/systemd/system/blinky-collector.service`:

```ini
[Unit]
Description=Blinky Collector
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/blinky-collector -w 9090 -p 9091
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable blinky-collector
sudo systemctl start blinky-collector
sudo systemctl status blinky-collector
```

### Agent Service

Create `/etc/systemd/system/blinky-agent.service`:

```ini
[Unit]
Description=Blinky Agent
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/blinky-agent -s <collector-ip> -p 9090 -i 5
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:
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
