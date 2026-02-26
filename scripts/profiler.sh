#!/bin/bash
# Kernel profiler script

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PROFILE_TYPE="time"
DURATION=10

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            PROFILE_TYPE="$2"
            shift 2
            ;;
        -d|--duration)
            DURATION="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -t, --type TYPE      Profile type (time|memory|cache)"
            echo "  -d, --duration SEC   Profile duration in seconds"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}Profiling kernel ($PROFILE_TYPE)...${NC}"

# Build with profiling
make clean
case $PROFILE_TYPE in
    time)
        make CFLAGS="-g -O2 -pg -DPROFILE_TIME"
        ;;
    memory)
        make CFLAGS="-g -O2 -DPROFILE_MEMORY"
        ;;
    cache)
        make CFLAGS="-g -O2 -DPROFILE_CACHE"
        ;;
esac

# Run with profiling
echo -e "${YELLOW}Running kernel for ${DURATION}s...${NC}"
timeout $DURATION qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio -nographic 2>&1 | \
    grep -E "(PROFILE|TIME|MEMORY|CACHE)" > profile.log || true

# Analyze results
if [ -f profile.log ]; then
    echo -e "${GREEN}Profile results:${NC}"
    cat profile.log
else
    echo -e "${YELLOW}No profile data collected${NC}"
fi
