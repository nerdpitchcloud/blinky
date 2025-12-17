# Blinky

[![GitHub Release](https://img.shields.io/github/v/release/nerdpitchcloud/blinky)](https://github.com/nerdpitchcloud/blinky/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/nerdpitchcloud/blinky/release.yml?branch=main)](https://github.com/nerdpitchcloud/blinky/actions)
[![License](https://img.shields.io/github/license/nerdpitchcloud/blinky)](https://github.com/nerdpitchcloud/blinky/blob/main/LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-blue)](https://github.com/nerdpitchcloud/blinky)
[![Architecture](https://img.shields.io/badge/arch-amd64%20%7C%20arm64-green)](https://github.com/nerdpitchcloud/blinky/releases)

Blinky is a lightweight, real-time monitoring system for Linux hosts, specifically designed for Debian-based systems. It provides comprehensive system monitoring with minimal dependencies and overhead.

## Installation

```bash
# Quick install (any Linux - auto-detects architecture)
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/install.sh | sudo bash

# Debian/Ubuntu DEB package (AMD64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-agent_VERSION_amd64.deb
sudo dpkg -i blinky-agent_VERSION_amd64.deb

# Debian/Ubuntu DEB package (ARM64)
wget https://github.com/nerdpitchcloud/blinky/releases/latest/download/blinky-agent_VERSION_arm64.deb
sudo dpkg -i blinky-agent_VERSION_arm64.deb

# Upgrade
sudo blinky-agent upgrade

# Uninstall
curl -fsSL https://raw.githubusercontent.com/nerdpitchcloud/blinky/main/uninstall.sh | sudo bash
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

### Collector (Docker)

The collector runs in a Docker container. See the deployment section below for Docker Compose configuration.

## License

MIT License - See LICENSE file for details 
