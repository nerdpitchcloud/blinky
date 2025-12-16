# Blinky Monitoring Tools

This document describes the monitoring tools included with Blinky for checking collector endpoints and viewing live metrics.

## Tools Overview

### 1. blinky-monitor.py - Live Terminal Monitor

A real-time monitoring tool that displays all metrics in a btop-style interface with auto-refresh.

**Features:**
- Live updates every 5 seconds
- Color-coded status indicators (green/yellow/red)
- Progress bars for CPU, memory, and disk usage
- Displays all available metrics
- Version mismatch warnings
- Clean, readable terminal UI

**Usage:**
```bash
./blinky-monitor.py <collector-url>
```

**Examples:**
```bash
# Monitor localhost collector
./blinky-monitor.py http://localhost:9091/api/metrics

# Monitor remote collector
./blinky-monitor.py http://monitoring.example.com:9091/api/metrics

# Short form (auto-adds http and /api/metrics)
./blinky-monitor.py localhost:9091
./blinky-monitor.py monitoring.example.com:9091
```

**Controls:**
- Press `Ctrl+C` to exit

**Output Example:**
```
================================================================================
BLINKY MONITOR - 2025-12-16 17:00:00
================================================================================

Total Hosts: 2

────────────────────────────────────────────────────────────────────────────────
HOST: web-server-01 [ONLINE]
Agent Version: 0.1.0
Uptime: 10d 0h 0m

CPU:
  Usage:    [████████████████░░░░░░░░░░░░░░░░░░░░░░░░] 45.2%
  Cores:    8
  Load Avg: 1.50 / 1.20 / 0.90

MEMORY:
  Usage:     [████████████████████████░░░░░░░░░░░░░░░░] 62.3%
  Total:     16.00 GB
  Used:      10.00 GB
  Available: 6.00 GB

DISKS:
  /
    Usage:     [████████████████████░░░░░░░░░░░░░░░] 50.0%
    Total:     100.00 GB
    Used:      50.00 GB
    Available: 50.00 GB

CONTAINERS: 3
  web-app [docker] running
    CPU: 15.5% | Memory: 512.00 MB
  api-server [docker] running
    CPU: 8.2% | Memory: 256.00 MB
  cache [docker] running
    CPU: 2.1% | Memory: 128.00 MB

KUBERNETES:
  Type:       k3s
  Pods:       25
  Nodes:      3
  Namespaces: 3

================================================================================
Press Ctrl+C to exit | Refreshing every 5 seconds
```

### 2. blinky-check.sh - Quick Status Check

A simple one-time status check tool for quick verification or scripting.

**Features:**
- Single snapshot of current status
- Color-coded output
- Compact display
- Exit codes for scripting
- No dependencies beyond curl and python3

**Usage:**
```bash
./blinky-check.sh <collector-url>
```

**Examples:**
```bash
# Check localhost collector
./blinky-check.sh http://localhost:9091/api/metrics

# Check remote collector
./blinky-check.sh monitoring.example.com:9091

# Use in scripts
if ./blinky-check.sh localhost:9091 > /dev/null 2>&1; then
    echo "Collector is healthy"
else
    echo "Collector has issues"
fi
```

**Output Example:**
```
Checking Blinky Collector: http://localhost:9091/api/metrics

Total Hosts: 2

● web-server-01 [ONLINE]
  Agent Version: 0.1.0
  CPU: 45.2% (Load: 1.50, Cores: 8)
  Memory: 62.3% (10.0GB / 16.0GB)
  Disks: 2 mounted
    /: 50.0% (50.0GB / 100.0GB free)
    /data: 75.5% (24.5GB / 100.0GB free)
  Containers: 3 running / 3 total
  Kubernetes: k3s (25 pods, 3 nodes)

● db-server-01 [ONLINE]
  Agent Version: 0.1.0
  CPU: 25.8% (Load: 2.10, Cores: 16)
  Memory: 85.2% (54.4GB / 64.0GB)
  Disks: 1 mounted
    /: 45.0% (550.0GB / 1000.0GB free)

Collector is working correctly
```

## Color Coding

Both tools use color coding to indicate status:

- **Green (●)**: Healthy / Online / < 60% usage
- **Yellow (●)**: Warning / 60-80% usage
- **Red (●)**: Critical / Offline / > 80% usage

## Requirements

### blinky-monitor.py
- Python 3.6 or higher
- No additional packages required (uses stdlib only)

### blinky-check.sh
- Bash
- curl
- Python 3 (for JSON parsing)

## Troubleshooting

### Connection Refused
```
ERROR: Failed to connect to collector
```

**Solutions:**
- Verify the collector is running
- Check the URL and port are correct
- Ensure firewall allows connections to port 9091
- Try: `curl http://localhost:9091/api/metrics`

### Empty Response
```
ERROR: Empty response from collector
```

**Solutions:**
- Collector may be starting up, wait a moment
- Check collector logs for errors
- Verify agents are connected

### Invalid JSON
```
ERROR: Invalid JSON response
```

**Solutions:**
- Collector may be returning an error page
- Check the URL is correct (should end with /api/metrics)
- Verify collector version is compatible

## Integration Examples

### Monitoring Script
```bash
#!/bin/bash
# Check all collectors and alert if any are down

COLLECTORS=(
    "http://collector1.example.com:9091/api/metrics"
    "http://collector2.example.com:9091/api/metrics"
)

for collector in "${COLLECTORS[@]}"; do
    if ! ./blinky-check.sh "$collector" > /dev/null 2>&1; then
        echo "ALERT: Collector $collector is down!"
        # Send alert (email, slack, etc.)
    fi
done
```

### Cron Job
```bash
# Check collector health every 5 minutes
*/5 * * * * /opt/blinky/blinky-check.sh http://localhost:9091/api/metrics || /usr/local/bin/alert-admin.sh
```

### Systemd Service Health Check
```ini
[Unit]
Description=Blinky Collector Health Check
After=blinky-collector.service

[Service]
Type=oneshot
ExecStart=/opt/blinky/blinky-check.sh http://localhost:9091/api/metrics

[Install]
WantedBy=multi-user.target
```

## Tips

1. **Use tmux/screen** for persistent monitoring:
   ```bash
   tmux new -s blinky-monitor
   ./blinky-monitor.py http://collector:9091/api/metrics
   # Detach with Ctrl+B, D
   ```

2. **Monitor multiple collectors** by running multiple instances in different terminals

3. **Pipe to log files** for historical tracking:
   ```bash
   while true; do
       ./blinky-check.sh localhost:9091 >> /var/log/blinky-checks.log
       sleep 300
   done
   ```

4. **Use with watch** for periodic checks:
   ```bash
   watch -n 5 -c './blinky-check.sh localhost:9091'
   ```
