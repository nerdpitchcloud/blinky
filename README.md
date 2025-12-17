# Blinky

[![GitHub Release](https://img.shields.io/github/v/release/nerdpitchcloud/blinky)](https://github.com/nerdpitchcloud/blinky/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/nerdpitchcloud/blinky/release.yml?branch=main)](https://github.com/nerdpitchcloud/blinky/actions)
[![License](https://img.shields.io/github/license/nerdpitchcloud/blinky)](https://github.com/nerdpitchcloud/blinky/blob/main/LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-blue)](https://github.com/nerdpitchcloud/blinky)
[![Architecture](https://img.shields.io/badge/arch-amd64%20%7C%20arm64-green)](https://github.com/nerdpitchcloud/blinky/releases)

Blinky is a lightweight, real-time monitoring system for Linux hosts, specifically designed for Debian-based systems. It provides comprehensive system monitoring with minimal dependencies and overhead.

## Installation

### Agent Installation

Install the monitoring agent on hosts you want to monitor:

```bash
# Quick install (any Linux - auto-detects architecture)
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/install-agent.sh | sudo bash

# Debian/Ubuntu DEB package (AMD64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-agent_VERSION_amd64.deb
sudo dpkg -i blinky-agent_VERSION_amd64.deb

# Debian/Ubuntu DEB package (ARM64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-agent_VERSION_arm64.deb
sudo dpkg -i blinky-agent_VERSION_arm64.deb

# Upgrade
sudo blinky-agent upgrade

# Uninstall
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/uninstall-agent.sh | sudo bash
```

### Collector Installation

Install the collector on your central monitoring server:

```bash
# Quick install (any Linux - auto-detects architecture)
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/install-collector.sh | sudo bash

# Binary tarball (AMD64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-collector-VERSION-linux-amd64.tar.gz
tar -xzf blinky-collector-VERSION-linux-amd64.tar.gz
sudo mv collector/blinky-collector /usr/local/bin/
sudo chmod +x /usr/local/bin/blinky-collector

# Binary tarball (ARM64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-collector-VERSION-linux-arm64.tar.gz
tar -xzf blinky-collector-VERSION-linux-arm64.tar.gz
sudo mv collector/blinky-collector /usr/local/bin/
sudo chmod +x /usr/local/bin/blinky-collector

# Uninstall
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/uninstall-collector.sh | sudo bash
```

## Operating Modes

Blinky supports multiple operating modes:

- **local**: Store metrics locally only (no network required)
- **pull**: Local storage + HTTP API for pulling metrics (default)
- **push**: Push metrics to collector via WebSocket
- **hybrid**: Both push to collector AND store locally with API

### Pull Mode (Default)

The agent stores metrics locally and exposes an HTTP API:

```bash
# Start agent (uses pull mode by default)
blinky-agent

# Access latest metrics
curl http://localhost:9092/metrics

# Get last 100 metrics
curl http://localhost:9092/metrics/latest?count=100

# Health check
curl http://localhost:9092/health
```

Metrics are stored in `/var/lib/blinky/metrics` with automatic rotation.

### Push Mode

Configure the agent to push metrics to a collector:

```toml
[agent]
mode = "push"

[collector]
enabled = true
host = "collector.example.com"
port = 9090
```

### Hybrid Mode

Get the best of both worlds - push to collector AND maintain local storage:

```toml
[agent]
mode = "hybrid"

[collector]
enabled = true
host = "collector.example.com"
```

## Configuration

Edit `/etc/blinky/config.toml` to configure the agent:

```toml
[agent]
mode = "pull"  # local, pull, push, or hybrid
interval = 5

[storage]
path = "/var/lib/blinky/metrics"
max_files = 100
max_file_size_mb = 10

[api]
enabled = true
port = 9092

[collector]
enabled = false
host = "localhost"
port = 9090
```

For complete configuration options, see:
- [CONFIGURATION.md](CONFIGURATION.md) - Complete configuration guide
- [config.toml.example](config.toml.example) - Example with all options

## Collector

The collector is a lightweight C++ server that receives metrics from agents and provides REST API endpoints for querying. It runs directly on the host (no Docker required).

### Running the Collector

```bash
# Start the collector
blinky-collector

# Or specify custom ports
blinky-collector --ws-port 9090 --http-port 9091
```

The collector provides:
- **WebSocket server** (port 9090) - Receives metrics from agents
- **HTTP REST API** (port 9091) - Query metrics and host status

### API Endpoints

- `GET /api/metrics` - Get all host metrics
- `GET /api/hosts` - List connected hosts  
- `GET /health` - Health check

### Configuration

Edit `/etc/blinky/collector.toml`:

```toml
[collector]
ws_port = 9090
http_port = 9091
bind_address = "0.0.0.0"

[storage]
path = "/var/lib/blinky/collector"
max_metrics_per_host = 1000
persistent = true
max_age_hours = 24
```

## License

MIT License - See LICENSE file for details 
