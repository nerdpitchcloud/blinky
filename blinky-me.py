#!/usr/bin/env python3
"""
Blinky Me - Simple metrics viewer for Blinky agent
Displays live metrics from local agent or remote collector
"""

import sys
import json
import time
import argparse
from urllib.request import urlopen
from urllib.error import URLError

def clear_screen():
    """Clear terminal screen"""
    print("\033[2J\033[H", end="")

def get_metrics(url):
    """Fetch metrics from agent or collector"""
    try:
        with urlopen(url, timeout=5) as response:
            return json.loads(response.read().decode())
    except URLError as e:
        return {"error": str(e)}
    except Exception as e:
        return {"error": f"Failed to fetch metrics: {e}"}

def format_bytes(bytes_val):
    """Format bytes to human readable"""
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if bytes_val < 1024.0:
            return f"{bytes_val:.1f}{unit}"
        bytes_val /= 1024.0
    return f"{bytes_val:.1f}PB"

def format_percent(value, width=50):
    """Format percentage as progress bar"""
    filled = int(width * value / 100)
    bar = '█' * filled + '░' * (width - filled)
    
    if value < 50:
        color = '\033[92m'  # Green
    elif value < 80:
        color = '\033[93m'  # Yellow
    else:
        color = '\033[91m'  # Red
    
    return f"{color}{bar}\033[0m {value:.1f}%"

def display_metrics(data, show_all=False):
    """Display metrics in a clean format"""
    clear_screen()
    
    print("\033[1;36m╔════════════════════════════════════════════════════════════════╗\033[0m")
    print("\033[1;36m║                        BLINKY METRICS                          ║\033[0m")
    print("\033[1;36m╚════════════════════════════════════════════════════════════════╝\033[0m")
    print()
    
    if "error" in data:
        print(f"\033[91m✗ Error: {data['error']}\033[0m")
        return
    
    # Handle both agent (direct) and collector (hosts array) responses
    if "hosts" in data:
        # Collector response
        if not data["hosts"]:
            print("\033[93m⚠ No hosts connected\033[0m")
            return
        metrics = data["hosts"][0]["metrics"]
        hostname = data["hosts"][0]["hostname"]
        online = data["hosts"][0]["online"]
        status = "\033[92m●\033[0m ONLINE" if online else "\033[91m●\033[0m OFFLINE"
    else:
        # Direct agent response
        metrics = data
        hostname = data.get("hostname", "unknown")
        status = "\033[92m●\033[0m ONLINE"
    
    print(f"\033[1mHost:\033[0m {hostname} {status}")
    print(f"\033[1mTime:\033[0m {time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # System Info - Always show
    if "system_info" in metrics:
        sysinfo = metrics["system_info"]
        print()
        print("\033[1;36m▶ SYSTEM INFO\033[0m")
        if sysinfo.get("os_name"):
            print(f"  OS:           {sysinfo['os_name']}")
        if sysinfo.get("kernel"):
            print(f"  Kernel:       {sysinfo['kernel']}")
        if sysinfo.get("architecture"):
            print(f"  Architecture: {sysinfo['architecture']}")
        if sysinfo.get("cpu_model"):
            print(f"  CPU Model:    {sysinfo['cpu_model']}")
        # Fix: Ensure cpu_cores and cpu_threads are valid integers
        cpu_cores = sysinfo.get("cpu_cores", 0)
        cpu_threads = sysinfo.get("cpu_threads", 0)
        if cpu_cores and cpu_threads and cpu_cores < 1000 and cpu_threads < 1000:
            print(f"  CPU:          {cpu_cores} cores / {cpu_threads} threads")
        if sysinfo.get("total_memory"):
            print(f"  Total Memory: {format_bytes(sysinfo['total_memory'])}")
    
    print()
    
    # CPU
    if "cpu" in metrics:
        cpu = metrics["cpu"]
        print("\033[1;33m▶ CPU\033[0m")
        print(f"  Usage:  {format_percent(cpu.get('usage_percent', 0))}")
        print(f"  Load:   {cpu.get('load_1min', 0):.2f} / {cpu.get('load_5min', 0):.2f} / {cpu.get('load_15min', 0):.2f}")
        print(f"  Cores:  {cpu.get('core_count', 0)}")
        print()
    
    # Memory
    if "memory" in metrics:
        mem = metrics["memory"]
        print("\033[1;35m▶ MEMORY\033[0m")
        print(f"  Usage:  {format_percent(mem.get('usage_percent', 0))}")
        print(f"  Used:   {format_bytes(mem.get('used_bytes', 0))} / {format_bytes(mem.get('total_bytes', 0))}")
        print(f"  Free:   {format_bytes(mem.get('available_bytes', 0))}")
        print()
    
    # Disks
    if "disks" in metrics and metrics["disks"]:
        print("\033[1;34m▶ DISKS\033[0m")
        for disk in metrics["disks"][:3 if not show_all else None]:
            print(f"  {disk['mount_point']}")
            print(f"    {format_percent(disk.get('usage_percent', 0))}")
            print(f"    {format_bytes(disk.get('used_bytes', 0))} / {format_bytes(disk.get('total_bytes', 0))}")
        if len(metrics["disks"]) > 3 and not show_all:
            print(f"  ... and {len(metrics['disks']) - 3} more (use --all to show)")
        print()
    
    # Network - Show top 3 by default, all with --all
    if "network" in metrics and metrics["network"]:
        print("\033[1;32m▶ NETWORK\033[0m")
        ifaces = metrics["network"][:None if show_all else 5]
        for iface in ifaces:
            print(f"  {iface['interface']}")
            print(f"    RX: {format_bytes(iface.get('rx_bytes', 0))}  TX: {format_bytes(iface.get('tx_bytes', 0))}")
            if iface.get('rx_errors', 0) > 0 or iface.get('tx_errors', 0) > 0:
                print(f"    \033[91mErrors: RX {iface.get('rx_errors', 0)} TX {iface.get('tx_errors', 0)}\033[0m")
        if len(metrics["network"]) > 5 and not show_all:
            print(f"  ... and {len(metrics['network']) - 5} more (use --all to show)")
        print()
    
    # Containers - Always show if present
    if "containers" in metrics and metrics["containers"]:
        print("\033[1;36m▶ CONTAINERS\033[0m")
        containers = metrics["containers"][:None if show_all else 10]
        for container in containers:
            state_color = '\033[92m' if container['state'] == 'running' else '\033[91m'
            print(f"  {state_color}●\033[0m {container['name']} ({container['runtime']})")
            if container.get('cpu_percent', 0) > 0:
                print(f"    CPU: {container['cpu_percent']:.1f}%  MEM: {format_bytes(container.get('memory_bytes', 0))}")
        if len(metrics["containers"]) > 10 and not show_all:
            print(f"  ... and {len(metrics['containers']) - 10} more (use --all to show)")
        print()
    
    # Kubernetes - Always show if detected
    if "kubernetes" in metrics and metrics["kubernetes"].get("detected"):
        k8s = metrics["kubernetes"]
        print("\033[1;35m▶ KUBERNETES\033[0m")
        print(f"  Type:       {k8s.get('cluster_type', 'unknown')}")
        print(f"  Pods:       {k8s.get('pod_count', 0)}")
        print(f"  Nodes:      {k8s.get('node_count', 0)}")
        print(f"  Namespaces: {len(k8s.get('namespaces', []))}")
        if show_all and k8s.get('namespaces'):
            print(f"  Namespace list: {', '.join(k8s.get('namespaces', []))}")
        print()
    
    # SMART - Always show if there are warnings
    if "smart_data" in metrics and metrics["smart_data"]:
        failed = [s for s in metrics["smart_data"] if not s.get("passed", True)]
        healthy = [s for s in metrics["smart_data"] if s.get("passed", True)]
        
        if failed:
            print("\033[1;31m▶ SMART WARNINGS\033[0m")
            for smart in failed:
                print(f"  \033[91m✗\033[0m {smart['device']}: {smart.get('health_status', 'UNKNOWN')}")
                if smart.get('temperature', 0) > 50:
                    print(f"    Temp: {smart['temperature']}°C")
                if smart.get('reallocated_sectors', 0) > 0:
                    print(f"    Reallocated sectors: {smart['reallocated_sectors']}")
            print()
        
        if show_all and healthy:
            print("\033[1;32m▶ SMART STATUS\033[0m")
            for smart in healthy:
                temp = smart.get('temperature', 0)
                temp_color = '\033[92m' if temp < 45 else '\033[93m' if temp < 55 else '\033[91m'
                print(f"  \033[92m✓\033[0m {smart['device']}: {smart.get('health_status', 'OK')} {temp_color}({temp}°C)\033[0m")
            print()
    
    print("\033[90m" + "─" * 64 + "\033[0m")
    if show_all:
        print("\033[90mPress Ctrl+C to exit | Showing all details\033[0m")
    else:
        print("\033[90mPress Ctrl+C to exit | Use --all for more details\033[0m")

def main():
    parser = argparse.ArgumentParser(
        description='Blinky Me - Live metrics viewer (auto-refreshes by default)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Local agent (live updates)
  %(prog)s 192.168.1.100      # Remote agent (live updates)
  %(prog)s host.example.com   # Remote agent by hostname
  %(prog)s 10.0.0.5 --all     # Show all details (network, containers, k8s)
  %(prog)s --interval 2       # Faster refresh (2 seconds)
        """
    )
    
    parser.add_argument('host',
                       nargs='?',
                       default='localhost',
                       help='Host IP or hostname (default: localhost)')
    parser.add_argument('--interval', '-i',
                       type=int,
                       default=5,
                       help='Refresh interval in seconds (default: 5)')
    parser.add_argument('--all', '-a',
                       action='store_true',
                       help='Show all details (network, containers, k8s)')
    
    args = parser.parse_args()
    
    # Build URL from host
    host = args.host
    if not host.startswith('http'):
        url = f'http://{host}:9092/metrics'
    else:
        url = host if '/metrics' in host else f'{host}/metrics'
    
    try:
        print("\033[?25l", end="")  # Hide cursor
        while True:
            data = get_metrics(url)
            display_metrics(data, args.all)
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\033[?25h")  # Show cursor
        print("\n\033[92m✓ Goodbye!\033[0m")
        sys.exit(0)
    except Exception as e:
        print("\033[?25h")  # Show cursor
        print(f"\033[91m✗ Error: {e}\033[0m")
        sys.exit(1)

if __name__ == '__main__':
    main()
