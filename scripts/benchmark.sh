#!/bin/bash
# Benchmark script for kernel performance

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

BENCHMARK_TYPE="all"
ITERATIONS=10

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BENCHMARK_TYPE="$2"
            shift 2
            ;;
        -i|--iterations)
            ITERATIONS="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -t, --type TYPE      Benchmark type (memory|scheduler|all)"
            echo "  -i, --iterations N   Number of iterations"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}Running kernel benchmarks...${NC}"

# Build kernel
make clean
make CFLAGS="-O3 -flto -DBENCHMARK"

# Run benchmarks
echo -e "${YELLOW}Benchmark type: $BENCHMARK_TYPE${NC}"
echo -e "${YELLOW}Iterations: $ITERATIONS${NC}"

# QEMU will run benchmarks
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio -nographic 2>&1 | \
    grep -E "(BENCHMARK|TIME|CYCLES)" || true

echo -e "${GREEN}Benchmarks completed!${NC}"
