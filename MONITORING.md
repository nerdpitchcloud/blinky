# Blinky Monitoring

Simple Python script for viewing Blinky metrics in your terminal with live auto-refresh.

## Usage

```bash
# Local agent (live updates)
./blinky-me.py

# Remote agent (live updates)
./blinky-me.py 192.168.1.100

# Show all details (network, containers, k8s)
./blinky-me.py 192.168.1.100 --all

# Custom refresh interval
./blinky-me.py --interval 2
```

## Features

- Clean, colorful display with progress bars
- Auto-refreshes by default (live updates)
- Works with agent or collector
- Shows CPU, memory, disks, network, containers, Kubernetes
- No external dependencies (pure Python 3)

## Options

```
host                  Host IP or hostname (default: localhost)
--interval, -i SEC    Refresh interval in seconds (default: 5)
--all, -a             Show all details (network, containers, k8s)
```

## Examples

```bash
# Monitor multiple hosts in tmux
tmux split-window -h
./blinky-me.py 192.168.1.10
./blinky-me.py 192.168.1.20

# Faster refresh rate
./blinky-me.py 192.168.1.100 --interval 2

# Full details view
./blinky-me.py 192.168.1.100 --all
```
