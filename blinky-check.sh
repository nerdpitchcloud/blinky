#!/bin/bash

COLLECTOR_URL="${1:-http://localhost:8081/api/metrics}"

if [[ ! "$COLLECTOR_URL" =~ ^http ]]; then
    COLLECTOR_URL="http://$COLLECTOR_URL"
fi

if [[ ! "$COLLECTOR_URL" =~ /api/metrics$ ]]; then
    COLLECTOR_URL="${COLLECTOR_URL%/}/api/metrics"
fi

echo "Checking Blinky Collector: $COLLECTOR_URL"
echo ""

response=$(curl -s "$COLLECTOR_URL")

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to connect to collector"
    echo "Make sure the collector is running at $COLLECTOR_URL"
    exit 1
fi

if [ -z "$response" ]; then
    echo "ERROR: Empty response from collector"
    exit 1
fi

echo "$response" | python3 -c "
import sys
import json

try:
    data = json.load(sys.stdin)
    hosts = data.get('hosts', [])
    
    if not hosts:
        print('No hosts connected to collector')
        sys.exit(0)
    
    print(f'Total Hosts: {len(hosts)}')
    print('')
    
    for host in hosts:
        hostname = host.get('hostname', 'unknown')
        online = host.get('online', False)
        agent_version = host.get('agent_version', 'unknown')
        version_mismatch = host.get('version_mismatch', False)
        metrics = host.get('metrics', {})
        
        status = 'ONLINE' if online else 'OFFLINE'
        status_color = '\033[92m' if online else '\033[91m'
        
        print(f'{status_color}●\033[0m {hostname} [{status}]')
        print(f'  Agent Version: {agent_version}', end='')
        
        if version_mismatch:
            print(' \033[93m[VERSION MISMATCH]\033[0m')
        else:
            print()
        
        if not online or not metrics:
            print('  No metrics available')
            print()
            continue
        
        cpu = metrics.get('cpu', {})
        memory = metrics.get('memory', {})
        disks = metrics.get('disks', [])
        containers = metrics.get('containers', [])
        k8s = metrics.get('kubernetes', {})
        
        if cpu:
            usage = cpu.get('usage', 0)
            load_1 = cpu.get('load_1', 0)
            cores = cpu.get('cores', 0)
            
            cpu_color = '\033[91m' if usage >= 80 else '\033[93m' if usage >= 60 else '\033[92m'
            print(f'  CPU: {cpu_color}{usage:.1f}%\033[0m (Load: {load_1:.2f}, Cores: {cores})')
        
        if memory:
            usage = memory.get('usage', 0)
            total = memory.get('total', 0)
            used = memory.get('used', 0)
            
            mem_color = '\033[91m' if usage >= 80 else '\033[93m' if usage >= 60 else '\033[92m'
            total_gb = total / (1024**3)
            used_gb = used / (1024**3)
            print(f'  Memory: {mem_color}{usage:.1f}%\033[0m ({used_gb:.1f}GB / {total_gb:.1f}GB)')
        
        if disks:
            print(f'  Disks: {len(disks)} mounted')
            for disk in disks:
                mount = disk.get('mount', '/')
                usage = disk.get('usage', 0)
                total = disk.get('total', 0)
                available = disk.get('available', 0)
                
                disk_color = '\033[91m' if usage >= 80 else '\033[93m' if usage >= 60 else '\033[92m'
                total_gb = total / (1024**3)
                avail_gb = available / (1024**3)
                print(f'    {mount}: {disk_color}{usage:.1f}%\033[0m ({avail_gb:.1f}GB / {total_gb:.1f}GB free)')
        
        if containers:
            running = [c for c in containers if c.get('state') == 'running']
            print(f'  Containers: {len(running)} running / {len(containers)} total')
        
        if k8s and k8s.get('detected', False):
            cluster_type = k8s.get('type', 'unknown')
            pods = k8s.get('pods', 0)
            nodes = k8s.get('nodes', 0)
            print(f'  Kubernetes: {cluster_type} ({pods} pods, {nodes} nodes)')
        
        print()

except json.JSONDecodeError as e:
    print(f'ERROR: Invalid JSON response: {e}')
    sys.exit(1)
except Exception as e:
    print(f'ERROR: {e}')
    sys.exit(1)
"

if [ $? -eq 0 ]; then
    echo "Collector is working correctly"
    exit 0
else
    echo "ERROR: Failed to parse metrics"
    exit 1
fi
