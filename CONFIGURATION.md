# Blinky Configuration Guide

Blinky uses a TOML configuration file for flexible agent configuration.

## Configuration File Locations

The agent searches for configuration files in the following order:

1. `/etc/blinky/config.toml` (system-wide, created by installer)
2. `~/.blinky/config.toml` (user-specific)
3. `./config.toml` (current directory)

You can also specify a custom config file with the `-c` flag:

```bash
blinky-agent -c /path/to/custom/config.toml
```

## Quick Start

After installation, edit `/etc/blinky/config.toml`:

```toml
[collector]
host = "your-collector-hostname"
port = 9090

[agent]
interval = 5
```

Then start the agent:

```bash
blinky-agent
```

## Command Line Overrides

Command line arguments override config file settings:

```bash
# Override collector host
blinky-agent -s collector.example.com

# Override port
blinky-agent -p 9090

# Override interval
blinky-agent -i 10

# Combine multiple overrides
blinky-agent -s collector.example.com -p 9090 -i 10
```

## Configuration Sections

### Agent Settings

```toml
[agent]
# Collection interval in seconds
interval = 5

# Enable/disable specific monitors
[agent.monitors]
cpu = true
memory = true
disk = true
smart = true
network = true
systemd = true
containers = true
kubernetes = true
```

### Collector Connection

```toml
[collector]
# Collector server hostname or IP
host = "localhost"

# WebSocket port
port = 9090

# Connection timeout in seconds
timeout = 10

[collector.reconnect]
# Enable automatic reconnection
enabled = true

# Initial retry delay in seconds
initial_delay = 5

# Maximum retry delay in seconds
max_delay = 300

# Backoff multiplier (exponential backoff)
backoff_multiplier = 2.0

# Maximum reconnection attempts (0 = unlimited)
max_attempts = 0
```

### Logging

```toml
[logging]
# Log level: debug, info, warn, error
level = "info"

# Log output: stdout, stderr, file
output = "stdout"

# Log file path (only used if output = "file")
# file = "/var/log/blinky/agent.log"

# Enable timestamps
timestamps = true

# Enable colored output (stdout/stderr only)
colors = true
```

### Disk Monitoring

```toml
[disk]
# Exclude mount points
exclude_mounts = [
    "/dev",
    "/dev/shm",
    "/run",
    "/sys/fs/cgroup",
    "/snap"
]

# Exclude filesystem types
exclude_fs_types = [
    "tmpfs",
    "devtmpfs",
    "squashfs",
    "overlay"
]

# Minimum disk size to monitor (GB, 0 = all)
min_size_gb = 0
```

### SMART Monitoring

```toml
[smart]
# Enable SMART monitoring
enabled = true

# Path to smartctl binary
smartctl_path = "/usr/sbin/smartctl"

# Specific devices to monitor (empty = auto-detect)
devices = []

# Exclude devices
exclude_devices = []
```

### Network Monitoring

```toml
[network]
# Specific interfaces to monitor (empty = all)
interfaces = []

# Exclude interfaces
exclude_interfaces = [
    "lo",
    "docker0",
    "veth*"
]

# Include virtual interfaces
include_virtual = false
```

### Systemd Monitoring

```toml
[systemd]
# Enable systemd monitoring
enabled = true

# Specific services to monitor (empty = all active)
services = []

# Exclude services
exclude_services = []

# Only monitor failed services
only_failed = false
```

### Container Monitoring

```toml
[containers]
# Enable Docker monitoring
docker = true

# Docker socket path
docker_socket = "/var/run/docker.sock"

# Enable Podman monitoring
podman = true

# Podman socket path
podman_socket = "/run/podman/podman.sock"

# Include stopped containers
include_stopped = false
```

### Kubernetes Monitoring

```toml
[kubernetes]
# Enable Kubernetes monitoring
enabled = true

# Kubeconfig path (empty = default)
kubeconfig = ""

# Namespace to monitor (empty = all)
namespace = ""

# Include system namespaces
include_system = false
```

### Security (Future)

```toml
[security]
# Enable TLS for WebSocket
tls = false

# CA certificate path
# ca_cert = "/etc/blinky/ca.crt"

# Client certificate
# client_cert = "/etc/blinky/client.crt"

# Client key
# client_key = "/etc/blinky/client.key"

# Verify server certificate
verify_cert = true
```

### Performance Tuning

```toml
[performance]
# Maximum metrics buffer size
buffer_size = 100

# Worker threads for collection
worker_threads = 4

# Enable metric compression
compression = false

# Aggregation window in seconds
aggregation_window = 0
```

## Example Configurations

### Minimal Configuration

```toml
[collector]
host = "monitoring.example.com"
port = 9090
```

### Production Configuration

```toml
[agent]
interval = 10

[collector]
host = "monitoring.example.com"
port = 9090
timeout = 30

[collector.reconnect]
enabled = true
initial_delay = 10
max_delay = 600
backoff_multiplier = 2.0

[logging]
level = "warn"
output = "file"
file = "/var/log/blinky/agent.log"

[disk]
exclude_mounts = ["/dev", "/run", "/snap"]
min_size_gb = 1

[network]
exclude_interfaces = ["lo", "docker0", "veth*"]
```

### Development Configuration

```toml
[agent]
interval = 2

[collector]
host = "localhost"
port = 9090

[logging]
level = "debug"
output = "stdout"
colors = true
timestamps = true

[agent.monitors]
cpu = true
memory = true
disk = true
smart = false
network = true
systemd = false
containers = true
kubernetes = false
```

## Validation

To validate your configuration, run:

```bash
blinky-agent --version
```

If the config file has errors, the agent will fall back to defaults and log a warning.

## Environment-Specific Configs

You can maintain different configs for different environments:

```bash
# Development
blinky-agent -c /etc/blinky/config.dev.toml

# Staging
blinky-agent -c /etc/blinky/config.staging.toml

# Production
blinky-agent -c /etc/blinky/config.prod.toml
```

## Troubleshooting

### Config file not found

If no config file is found, the agent uses built-in defaults. To verify which config is loaded:

```bash
blinky-agent
# Output: "Loaded config from: /etc/blinky/config.toml"
# Or: "No config file found, using defaults"
```

### Invalid config values

Invalid values are ignored and defaults are used. Check logs for warnings.

### Permission issues

Ensure the config file is readable:

```bash
sudo chmod 644 /etc/blinky/config.toml
```

## See Also

- [config.toml.example](config.toml.example) - Complete example with all options
- [README.md](README.md) - Main documentation
- [QUICKSTART.md](QUICKSTART.md) - Quick start guide
