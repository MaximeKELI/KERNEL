#!/bin/bash
# Test runner script

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TEST_SUITE=""
VERBOSE=false
COVERAGE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--suite)
            TEST_SUITE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --coverage)
            COVERAGE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -s, --suite SUITE   Run specific test suite"
            echo "  -v, --verbose       Verbose output"
            echo "  --coverage          Generate coverage report"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Build with tests
echo -e "${GREEN}Building kernel with tests enabled...${NC}"
make clean
make CFLAGS="-DRUN_TESTS -g -O0"

# Run tests in QEMU
echo -e "${GREEN}Running tests...${NC}"
if [ -n "$TEST_SUITE" ]; then
    echo -e "${YELLOW}Running test suite: $TEST_SUITE${NC}"
fi

# QEMU will run tests and output results
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio -nographic 2>&1 | \
    grep -E "(TEST|PASS|FAIL)" || true

echo -e "${GREEN}Tests completed!${NC}"
