#!/usr/bin/env python3
"""
Kernel Statistics Tool
Analyze kernel performance and resource usage
"""

import sys
import argparse
import re
from collections import defaultdict

def parse_kernel_log(log_file):
    """Parse kernel log for statistics"""
    stats = {
        'memory': defaultdict(int),
        'processes': defaultdict(int),
        'interrupts': 0,
        'errors': [],
        'warnings': []
    }
    
    with open(log_file, 'r') as f:
        for line in f:
            # Memory stats
            if 'Memory allocated' in line or 'kmalloc' in line:
                match = re.search(r'(\d+)\s*(bytes|KB|MB)', line)
                if match:
                    size = int(match.group(1))
                    unit = match.group(2)
                    if unit == 'MB':
                        size *= 1024 * 1024
                    elif unit == 'KB':
                        size *= 1024
                    stats['memory']['allocated'] += size
            
            # Process stats
            if 'Process created' in line:
                stats['processes']['created'] += 1
            if 'Process destroyed' in line:
                stats['processes']['destroyed'] += 1
            
            # Interrupt stats
            if 'Interrupt' in line:
                stats['interrupts'] += 1
            
            # Errors and warnings
            if '[ERROR]' in line:
                stats['errors'].append(line.strip())
            if '[WARN]' in line:
                stats['warnings'].append(line.strip())
    
    return stats

def print_stats(stats):
    """Print statistics"""
    print("=== Kernel Statistics ===\n")
    
    print("Memory:")
    print(f"  Allocated: {stats['memory']['allocated'] / 1024 / 1024:.2f} MB")
    
    print("\nProcesses:")
    print(f"  Created: {stats['processes']['created']}")
    print(f"  Destroyed: {stats['processes']['destroyed']}")
    print(f"  Active: {stats['processes']['created'] - stats['processes']['destroyed']}")
    
    print(f"\nInterrupts: {stats['interrupts']}")
    
    print(f"\nErrors: {len(stats['errors'])}")
    if stats['errors']:
        print("  Recent errors:")
        for error in stats['errors'][-5:]:
            print(f"    {error}")
    
    print(f"\nWarnings: {len(stats['warnings'])}")
    if stats['warnings']:
        print("  Recent warnings:")
        for warning in stats['warnings'][-5:]:
            print(f"    {warning}")

def main():
    parser = argparse.ArgumentParser(description='Kernel Statistics Tool')
    parser.add_argument('log_file', help='Kernel log file')
    parser.add_argument('--summary', action='store_true', help='Show summary only')
    
    args = parser.parse_args()
    
    stats = parse_kernel_log(args.log_file)
    print_stats(stats)

if __name__ == '__main__':
    main()
