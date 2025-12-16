# Blinky

Blinky is a lightweight, real-time monitoring system for Linux hosts, specifically designed for Debian-based systems. It provides comprehensive system monitoring with minimal dependencies and overhead.

## Features

### Agent Monitoring Capabilities
- **System Metrics**: CPU usage, memory, disk space, load average, uptime
- **SMART Disk Health**: Monitor disk health status and temperature
- **Network Statistics**: Interface traffic, packet counts, error rates
- **Systemd Services**: Monitor service status and health
- **Container Support**: Auto-detect and monitor Docker and Podman containers
- **Kubernetes Integration**: Auto-discover k8s/k3s clusters and monitor pods/nodes
- **Real-time Streaming**: Live metrics via WebSocket connection

### Collector Features
- **WebSocket Server**: Receive real-time metrics from multiple agents
- **HTTP Dashboard**: Live web interface with auto-refresh
- **Metrics Storage**: Historical data retention with configurable limits
- **REST API**: Programmatic access to metrics
- **Visual Indicators**: Color-coded alerts for CPU, memory, and disk usage

## Architecture

```
┌─────────────────┐         WebSocket          ┌──────────────────┐
│  Debian Host 1  │         (Port 8080)         │                  │
│  ┌───────────┐  │─────────────────────────────│   Collector      │
│  │  Agent    │  │                             │                  │
│  └───────────┘  │                             │  ┌────────────┐  │
└─────────────────┘                             │  │ Dashboard  │  │
                                                │  │ (Port 8081)│  │
┌─────────────────┐                             │  └────────────┘  │
│  Debian Host 2  │                             │                  │
│  ┌───────────┐  │─────────────────────────────│  ┌────────────┐  │
│  │  Agent    │  │                             │  │ REST API   │  │
│  └───────────┘  │                             │  └────────────┘  │
└─────────────────┘                             └──────────────────┘
```

## Building

### Prerequisites
```bash
apt-get update
apt-get install -y build-essential cmake libssl-dev
```

### Build Instructions
```bash
git clone https://github.com/nerdpitchcloud/blinky.git
cd blinky
mkdir build && cd build
cmake ..
make -j$(nproc)
```

This will create two binaries:
- `agent/blinky-agent` - The monitoring agent
- `collector/blinky-collector` - The collector server

## Usage

### Running the Collector

Start the collector server on your monitoring host:

```bash
./collector/blinky-collector
```

Options:
- `-w, --ws-port PORT` - WebSocket port (default: 8080)
- `-p, --http-port PORT` - HTTP dashboard port (default: 8081)

The dashboard will be available at `http://localhost:8081/`

### Running the Agent

On each host you want to monitor:

```bash
./agent/blinky-agent -s <collector-host> -p 8080
```

Options:
- `-s, --server HOST` - Collector server hostname/IP (default: localhost)
- `-p, --port PORT` - Collector WebSocket port (default: 8080)
- `-i, --interval SECONDS` - Metrics collection interval (default: 5)

### Example Setup

1. Start collector on monitoring server:
```bash
./collector/blinky-collector -w 8080 -p 8081
```

2. Start agents on monitored hosts:
```bash
# On host1
./agent/blinky-agent -s monitoring.example.com -p 8080 -i 5

# On host2
./agent/blinky-agent -s monitoring.example.com -p 8080 -i 5
```

3. Open dashboard in browser:
```
http://monitoring.example.com:8081/
```

## API Endpoints

### REST API

- `GET /` - Dashboard homepage
- `GET /api/metrics` - JSON metrics for all hosts

Example API response:
```json
{
  "hosts": [
    {
      "hostname": "web-server-01",
      "online": true,
      "metrics": {
        "timestamp": 1702742400,
        "cpu": {"usage": 45.2, "load_1": 1.5},
        "memory": {"usage": 62.3, "total": 8589934592},
        "disks": [...],
        "containers": [...]
      }
    }
  ]
}
```

## Monitored Metrics

### System Metrics
- CPU usage percentage
- Load averages (1, 5, 15 minutes)
- Memory usage and availability
- Disk usage per mount point
- System uptime

### SMART Data
- Disk temperature
- Power-on hours
- Reallocated sectors
- Pending sectors
- Health status (PASSED/FAILED)

### Network
- Bytes/packets received and transmitted
- Error counts per interface

### Systemd Services
- Service state (active/inactive)
- Sub-state
- Enabled status

### Containers
- Docker and Podman support
- Container state
- CPU and memory usage
- Runtime information

### Kubernetes
- Cluster type detection (k8s/k3s)
- Pod count
- Node count
- Namespace list

## Dependencies

### Runtime Dependencies
- Linux kernel with /proc filesystem
- systemd (for service monitoring)
- smartctl (for SMART disk monitoring, optional)
- docker/podman (for container monitoring, optional)
- kubectl/k3s (for Kubernetes monitoring, optional)

### Build Dependencies
- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.14+
- OpenSSL development libraries

## Project Structure

```
blinky/
├── agent/              # Monitoring agent
│   ├── include/        # Agent headers
│   └── src/            # Agent implementation
├── collector/          # Collector server
│   ├── include/        # Collector headers
│   └── src/            # Collector implementation
├── shared/             # Shared protocol library
│   ├── include/        # Shared headers
│   └── src/            # Shared implementation
└── build/              # Build output
```

## License

MIT License - See LICENSE file for details 
