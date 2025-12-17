# Blinky Monitoring

Simple Python script for viewing Blinky metrics in your terminal.

## Usage

```bash
# Local agent
./blinky-me.py

# Remote agent
./blinky-me.py 192.168.1.100

# Live updates
./blinky-me.py 192.168.1.100 -w

# Show all details
./blinky-me.py 192.168.1.100 -w -a
```

## Features

- Clean, colorful display with progress bars
- Live updates with `--watch`
- Works with agent or collector
- Shows CPU, memory, disks, network, containers, Kubernetes
- No external dependencies (pure Python 3)

## Options

```
host                  Host IP or hostname (default: localhost)
--watch, -w           Live updates
--interval, -i SEC    Refresh interval (default: 5)
--all, -a             Show all details
```

## Examples

```bash
# Monitor multiple hosts in tmux
tmux split-window -h
./blinky-me.py 192.168.1.10 -w
./blinky-me.py 192.168.1.20 -w

# Cron job
*/5 * * * * /opt/blinky/blinky-me.py 192.168.1.100 >> /var/log/blinky.log
```
