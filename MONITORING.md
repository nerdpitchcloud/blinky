# Blinky Monitoring

Simple Python script for viewing Blinky metrics in your terminal.

## Usage

```bash
# Quick snapshot
./blinky-me.py

# Live updates
./blinky-me.py --watch

# Show all details
./blinky-me.py --all --watch

# Remote agent
./blinky-me.py --url http://host:9092/metrics --watch

# Collector
./blinky-me.py --collector http://host:9091/api/metrics --watch
```

## Features

- Clean, colorful display with progress bars
- Live updates with `--watch`
- Works with agent or collector
- Shows CPU, memory, disks, network, containers, Kubernetes
- No external dependencies (pure Python 3)

## Options

```
--url URL             Agent URL (default: http://localhost:9092/metrics)
--collector URL       Collector URL
--watch, -w           Live updates
--interval, -i SEC    Refresh interval (default: 5)
--all, -a             Show all details
```

## Examples

```bash
# Monitor multiple hosts in tmux
tmux split-window -h
./blinky-me.py --url http://host1:9092/metrics --watch
./blinky-me.py --url http://host2:9092/metrics --watch

# Cron job
*/5 * * * * /opt/blinky/blinky-me.py >> /var/log/blinky.log
```
