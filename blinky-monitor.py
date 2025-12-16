#!/usr/bin/env python3

import sys
import json
import urllib.request
import urllib.error
import time
import os
from datetime import datetime

def clear_screen():
    os.system('clear' if os.name != 'nt' else 'cls')

def get_terminal_width():
    try:
        return os.get_terminal_size().columns
    except:
        return 80

def format_bytes(bytes_val):
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if bytes_val < 1024.0:
            return f"{bytes_val:.2f} {unit}"
        bytes_val /= 1024.0
    return f"{bytes_val:.2f} PB"

def format_uptime(seconds):
    days = seconds // 86400
    hours = (seconds % 86400) // 3600
    minutes = (seconds % 3600) // 60
    return f"{days}d {hours}h {minutes}m"

def draw_bar(percentage, width=40, filled_char='█', empty_char='░'):
    filled = int(width * percentage / 100)
    empty = width - filled
    
    if percentage >= 80:
        color = '\033[91m'
    elif percentage >= 60:
        color = '\033[93m'
    else:
        color = '\033[92m'
    
    reset = '\033[0m'
    bar = f"{color}{filled_char * filled}{empty_char * empty}{reset}"
    return f"[{bar}] {percentage:5.1f}%"

def fetch_metrics(url):
    try:
        with urllib.request.urlopen(url, timeout=5) as response:
            data = response.read()
            return json.loads(data)
    except urllib.error.URLError as e:
        return None
    except Exception as e:
        return None

def display_metrics(data):
    clear_screen()
    width = get_terminal_width()
    
    print("=" * width)
    print(f"BLINKY MONITOR - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * width)
    print()
    
    if not data or 'hosts' not in data:
        print("ERROR: No data available")
        print("Make sure the collector is running and accessible")
        return
    
    hosts = data.get('hosts', [])
    
    if not hosts:
        print("No hosts connected to collector")
        return
    
    print(f"Total Hosts: {len(hosts)}")
    print()
    
    for host_data in hosts:
        hostname = host_data.get('hostname', 'unknown')
        online = host_data.get('online', False)
        agent_version = host_data.get('agent_version', 'unknown')
        version_mismatch = host_data.get('version_mismatch', False)
        metrics = host_data.get('metrics', {})
        
        status = '\033[92mONLINE\033[0m' if online else '\033[91mOFFLINE\033[0m'
        print(f"{'─' * width}")
        print(f"HOST: {hostname} [{status}]")
        
        version_warn = ' \033[93m[VERSION MISMATCH]\033[0m' if version_mismatch else ''
        print(f"Agent Version: {agent_version}{version_warn}")
        
        if not online or not metrics:
            print("  No metrics available")
            print()
            continue
        
        uptime = metrics.get('uptime', 0)
        print(f"Uptime: {format_uptime(uptime)}")
        print()
        
        cpu = metrics.get('cpu', {})
        if cpu:
            print("CPU:")
            usage = cpu.get('usage', 0)
            cores = cpu.get('cores', 0)
            load_1 = cpu.get('load_1', 0)
            load_5 = cpu.get('load_5', 0)
            load_15 = cpu.get('load_15', 0)
            
            print(f"  Usage:    {draw_bar(usage, 40)}")
            print(f"  Cores:    {cores}")
            print(f"  Load Avg: {load_1:.2f} / {load_5:.2f} / {load_15:.2f}")
            print()
        
        memory = metrics.get('memory', {})
        if memory:
            print("MEMORY:")
            usage = memory.get('usage', 0)
            total = memory.get('total', 0)
            used = memory.get('used', 0)
            available = memory.get('available', 0)
            
            print(f"  Usage:     {draw_bar(usage, 40)}")
            print(f"  Total:     {format_bytes(total)}")
            print(f"  Used:      {format_bytes(used)}")
            print(f"  Available: {format_bytes(available)}")
            print()
        
        disks = metrics.get('disks', [])
        if disks:
            print("DISKS:")
            for disk in disks:
                mount = disk.get('mount', '/')
                usage = disk.get('usage', 0)
                total = disk.get('total', 0)
                used = disk.get('used', 0)
                available = disk.get('available', 0)
                
                print(f"  {mount}")
                print(f"    Usage:     {draw_bar(usage, 35)}")
                print(f"    Total:     {format_bytes(total)}")
                print(f"    Used:      {format_bytes(used)}")
                print(f"    Available: {format_bytes(available)}")
            print()
        
        smart = metrics.get('smart', [])
        if smart:
            print("SMART DISK HEALTH:")
            for disk in smart:
                device = disk.get('device', 'unknown')
                health = disk.get('health', 'UNKNOWN')
                temp = disk.get('temperature', 0)
                hours = disk.get('power_on_hours', 0)
                
                health_color = '\033[92m' if health == 'PASSED' else '\033[91m'
                print(f"  {device}: {health_color}{health}\033[0m (Temp: {temp}°C, Hours: {hours})")
            print()
        
        network = metrics.get('network', [])
        if network:
            print("NETWORK:")
            for net in network:
                iface = net.get('interface', 'unknown')
                rx_bytes = net.get('rx_bytes', 0)
                tx_bytes = net.get('tx_bytes', 0)
                rx_errors = net.get('rx_errors', 0)
                tx_errors = net.get('tx_errors', 0)
                
                print(f"  {iface}:")
                print(f"    RX: {format_bytes(rx_bytes)} ({rx_errors} errors)")
                print(f"    TX: {format_bytes(tx_bytes)} ({tx_errors} errors)")
            print()
        
        containers = metrics.get('containers', [])
        if containers:
            print(f"CONTAINERS: {len(containers)}")
            for container in containers[:5]:
                name = container.get('name', 'unknown')
                runtime = container.get('runtime', 'unknown')
                state = container.get('state', 'unknown')
                cpu = container.get('cpu', 0)
                memory = container.get('memory', 0)
                
                state_color = '\033[92m' if state == 'running' else '\033[93m'
                print(f"  {name} [{runtime}] {state_color}{state}\033[0m")
                print(f"    CPU: {cpu:.1f}% | Memory: {format_bytes(memory)}")
            
            if len(containers) > 5:
                print(f"  ... and {len(containers) - 5} more")
            print()
        
        k8s = metrics.get('kubernetes', {})
        if k8s and k8s.get('detected', False):
            print("KUBERNETES:")
            cluster_type = k8s.get('type', 'unknown')
            pods = k8s.get('pods', 0)
            nodes = k8s.get('nodes', 0)
            namespaces = k8s.get('namespaces', [])
            
            print(f"  Type:       {cluster_type}")
            print(f"  Pods:       {pods}")
            print(f"  Nodes:      {nodes}")
            print(f"  Namespaces: {len(namespaces)}")
            print()
        
        systemd = metrics.get('systemd', [])
        if systemd:
            active_services = [s for s in systemd if s.get('active', False)]
            inactive_services = [s for s in systemd if not s.get('active', False)]
            
            print(f"SYSTEMD SERVICES: {len(active_services)} active / {len(inactive_services)} inactive")
            
            if inactive_services:
                print("  Inactive services:")
                for svc in inactive_services[:5]:
                    name = svc.get('name', 'unknown')
                    state = svc.get('state', 'unknown')
                    print(f"    \033[91m●\033[0m {name} ({state})")
                
                if len(inactive_services) > 5:
                    print(f"    ... and {len(inactive_services) - 5} more")
            print()
    
    print("=" * width)
    print("Press Ctrl+C to exit | Refreshing every 5 seconds")

def main():
    if len(sys.argv) < 2:
        print("Usage: blinky-monitor.py <collector-url>")
        print()
        print("Examples:")
        print("  blinky-monitor.py http://localhost:8081/api/metrics")
        print("  blinky-monitor.py http://monitoring.example.com:8081/api/metrics")
        sys.exit(1)
    
    url = sys.argv[1]
    
    if not url.startswith('http'):
        url = f"http://{url}"
    
    if not url.endswith('/api/metrics'):
        if not url.endswith('/'):
            url += '/'
        url += 'api/metrics'
    
    print(f"Connecting to: {url}")
    print("Fetching initial data...")
    time.sleep(1)
    
    try:
        while True:
            data = fetch_metrics(url)
            display_metrics(data)
            time.sleep(5)
    except KeyboardInterrupt:
        clear_screen()
        print("Monitoring stopped")
        sys.exit(0)
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
