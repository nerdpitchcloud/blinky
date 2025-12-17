# Blinky Operating Modes

Blinky supports four operating modes to fit different monitoring architectures and requirements.

## Overview

| Mode | Local Storage | HTTP API | Push to Collector | Use Case |
|------|--------------|----------|-------------------|----------|
| **local** | ✅ | ❌ | ❌ | Standalone monitoring, no network |
| **pull** | ✅ | ✅ | ❌ | Pull-based monitoring (default) |
| **push** | ❌ | ❌ | ✅ | Traditional centralized monitoring |
| **hybrid** | ✅ | ✅ | ✅ | Best of both worlds |

## Mode Details

### Local Mode

Store metrics locally without any network connectivity.

**Configuration:**
```toml
[agent]
mode = "local"
interval = 5

[storage]
path = "/var/lib/blinky/metrics"
max_files = 100
max_file_size_mb = 10
```

**Use Cases:**
- Air-gapped systems
- Development/testing
- Compliance requirements (data must stay on host)
- Backup monitoring data

**Access Metrics:**
```bash
# Metrics stored in JSONL format
cat /var/lib/blinky/metrics/metrics-20231216.jsonl

# Latest metric
tail -n 1 /var/lib/blinky/metrics/metrics-*.jsonl | jq .
```

### Pull Mode (Default)

Store metrics locally AND expose HTTP API for external systems to pull metrics.

**Configuration:**
```toml
[agent]
mode = "pull"
interval = 5

[storage]
path = "/var/lib/blinky/metrics"
max_files = 100
max_file_size_mb = 10

[api]
enabled = true
port = 9092
bind_address = "0.0.0.0"
```

**Use Cases:**
- Prometheus-style pull-based monitoring
- Service discovery environments
- Kubernetes deployments
- Multi-collector scenarios
- Backup when collector is down

**API Endpoints:**

```bash
# Get latest metrics
curl http://localhost:9092/metrics

# Get last 100 metrics
curl http://localhost:9092/metrics/latest?count=100

# Health check
curl http://localhost:9092/health

# Storage statistics
curl http://localhost:9092/stats
```

**Example Response:**
```json
{
  "timestamp": 1702742400,
  "hostname": "web-server-01",
  "uptime_seconds": 86400,
  "cpu": {
    "usage_percent": 45.2,
    "load_1min": 1.5,
    "load_5min": 1.2,
    "load_15min": 1.0,
    "core_count": 4
  },
  "memory": {
    "total_bytes": 8589934592,
    "used_bytes": 5368709120,
    "available_bytes": 3221225472,
    "usage_percent": 62.5
  },
  "disks": [...],
  "network": [...],
  "containers": [...]
}
```

### Push Mode

Traditional push-based monitoring - send metrics to a central collector.

**Configuration:**
```toml
[agent]
mode = "push"
interval = 5

[collector]
enabled = true
host = "collector.example.com"
port = 9090
timeout = 10

[collector.reconnect]
enabled = true
initial_delay = 5
max_delay = 300
```

**Use Cases:**
- Centralized monitoring
- Real-time alerting
- Large-scale deployments
- Traditional monitoring architecture

**Features:**
- Automatic reconnection with exponential backoff
- Connection health monitoring
- Version compatibility checking
- Minimal local storage overhead

### Hybrid Mode

Combines push and pull - metrics are both pushed to collector AND stored locally with API access.

**Configuration:**
```toml
[agent]
mode = "hybrid"
interval = 5

[storage]
path = "/var/lib/blinky/metrics"
max_files = 100
max_file_size_mb = 10

[api]
enabled = true
port = 9092

[collector]
enabled = true
host = "collector.example.com"
port = 9090
```

**Use Cases:**
- High-availability monitoring
- Backup when collector is unavailable
- Local debugging while pushing to central system
- Compliance (local copy + central monitoring)
- Multi-collector environments

**Benefits:**
- Metrics always available locally even if collector is down
- Can query individual hosts directly
- Historical data preserved on host
- No data loss during collector outages

## Switching Modes

### Via Configuration File

Edit `/etc/blinky/config.toml`:

```toml
[agent]
mode = "pull"  # Change to: local, pull, push, or hybrid
```

Restart the agent:
```bash
sudo systemctl restart blinky-agent
```

### Via Command Line

```bash
# Local mode
blinky-agent -m local

# Pull mode
blinky-agent -m pull

# Push mode
blinky-agent -m push -s collector.example.com

# Hybrid mode
blinky-agent -m hybrid -s collector.example.com
```

## Storage Management

### Storage Location

Default: `/var/lib/blinky/metrics`

Metrics are stored in JSONL (JSON Lines) format:
```
/var/lib/blinky/metrics/
├── metrics-20231216.jsonl
├── metrics-20231217.jsonl
└── metrics-20231218.jsonl
```

### Rotation

Files are automatically rotated when they reach the configured size:

```toml
[storage]
max_file_size_mb = 10  # Rotate after 10MB
max_files = 100        # Keep last 100 files
```

### Cleanup

Old files are automatically deleted when `max_files` is reached.

Manual cleanup:
```bash
# Remove old metrics
find /var/lib/blinky/metrics -name "metrics-*.jsonl" -mtime +30 -delete

# Check storage usage
du -sh /var/lib/blinky/metrics
```

## API Reference

### GET /metrics

Returns the latest metric snapshot.

**Response:** Single JSON object with current metrics

### GET /metrics/latest?count=N

Returns the last N metrics.

**Parameters:**
- `count` (optional): Number of metrics to return (default: 100)

**Response:** JSON array of metrics

### GET /health

Health check endpoint.

**Response:**
```json
{
  "status": "ok"
}
```

### GET /stats

Storage statistics.

**Response:**
```json
{
  "total_metrics": 1523,
  "storage_path": "/var/lib/blinky/metrics"
}
```

## Performance Considerations

### Local/Pull Modes

- **Disk I/O**: One write per collection interval
- **Storage**: ~1-2KB per metric (compressed)
- **Memory**: Minimal (metrics not kept in memory)
- **CPU**: Negligible overhead

### Push Mode

- **Network**: One WebSocket message per interval
- **Bandwidth**: ~1-2KB per metric
- **Latency**: Real-time (< 1 second)

### Hybrid Mode

- **Overhead**: Sum of local + push
- **Recommended**: For critical systems where data loss is unacceptable

## Security Considerations

### API Access

By default, the API binds to `0.0.0.0` (all interfaces). To restrict access:

```toml
[api]
bind_address = "127.0.0.1"  # Localhost only
```

Use firewall rules to restrict access:
```bash
# Allow only from specific network
sudo ufw allow from 10.0.0.0/8 to any port 9092
```

### Storage Permissions

Metrics are stored with restricted permissions:
```bash
drwxr-xr-x /var/lib/blinky/metrics
-rw-r--r-- metrics-*.jsonl
```

### TLS Support

TLS support for API and collector connections is planned for future releases.

## Troubleshooting

### API Not Accessible

Check if API is running:
```bash
curl http://localhost:9092/health
```

Check firewall:
```bash
sudo ufw status
sudo ufw allow 9092/tcp
```

Check binding:
```bash
sudo netstat -tlnp | grep 9092
```

### Storage Full

Check disk space:
```bash
df -h /var/lib/blinky
```

Reduce retention:
```toml
[storage]
max_files = 50  # Reduce from 100
max_file_size_mb = 5  # Reduce from 10
```

### Collector Connection Failed (Push/Hybrid)

Check collector is running:
```bash
telnet collector.example.com 9090
```

Check agent logs:
```bash
journalctl -u blinky-agent -f
```

Verify configuration:
```bash
grep -A 5 "\[collector\]" /etc/blinky/config.toml
```

## Migration Guide

### From Push-Only to Hybrid

1. Edit config:
```toml
[agent]
mode = "hybrid"  # Was: push

[storage]
path = "/var/lib/blinky/metrics"
max_files = 100

[api]
enabled = true
port = 9092
```

2. Restart agent:
```bash
sudo systemctl restart blinky-agent
```

3. Verify API:
```bash
curl http://localhost:9092/health
```

### From Local to Pull

1. Edit config:
```toml
[agent]
mode = "pull"  # Was: local

[api]
enabled = true
port = 9092
```

2. Restart agent
3. Configure firewall if needed

## Best Practices

1. **Use pull mode by default** - Provides flexibility and backup
2. **Use hybrid mode for critical systems** - Ensures no data loss
3. **Monitor storage usage** - Set up alerts for disk space
4. **Secure the API** - Use firewall rules or bind to localhost
5. **Regular backups** - Back up `/var/lib/blinky/metrics` for compliance
6. **Tune retention** - Balance storage vs. historical data needs

## See Also

- [README.md](README.md) - Main documentation
- [CONFIGURATION.md](CONFIGURATION.md) - Configuration guide
- [QUICKSTART.md](QUICKSTART.md) - Quick start guide
