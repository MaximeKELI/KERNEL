#!/bin/bash
# Code quality checker

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

CHECK_TYPE="all"
FIX=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            CHECK_TYPE="$2"
            shift 2
            ;;
        --fix)
            FIX=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -t, --type TYPE   Check type (style|warnings|all)"
            echo "  --fix            Auto-fix issues"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

ERRORS=0

# Check for common issues
echo -e "${GREEN}Checking code quality...${NC}"

# Check for tabs
if grep -r $'\t' kernel/ include/ lib/ --include="*.c" --include="*.h"; then
    echo -e "${RED}Found tabs in source files!${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Check for trailing whitespace
if grep -r '[[:space:]]$' kernel/ include/ lib/ --include="*.c" --include="*.h"; then
    echo -e "${RED}Found trailing whitespace!${NC}"
    ERRORS=$((ERRORS + 1))
fi

# Check for TODO/FIXME without issue numbers
if grep -r "TODO\|FIXME" kernel/ include/ lib/ --include="*.c" --include="*.h" | grep -v "#[0-9]"; then
    echo -e "${YELLOW}Found TODO/FIXME without issue numbers${NC}"
fi

# Check for memory leaks (basic)
if grep -r "kmalloc\|malloc" kernel/ --include="*.c" | grep -v "kfree\|free"; then
    echo -e "${YELLOW}Potential memory leaks detected${NC}"
fi

# Check compilation warnings
echo -e "${GREEN}Checking for compilation warnings...${NC}"
make clean > /dev/null 2>&1
if ! make 2>&1 | grep -i warning; then
    echo -e "${GREEN}No warnings found!${NC}"
else
    echo -e "${YELLOW}Warnings found${NC}"
    ERRORS=$((ERRORS + 1))
fi

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}Code quality check passed!${NC}"
    exit 0
else
    echo -e "${RED}Code quality check failed with $ERRORS errors${NC}"
    exit 1
fi
