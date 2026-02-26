#!/bin/bash
# Build script with advanced options

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default options
BUILD_TYPE="release"
ENABLE_TESTS=false
ENABLE_DOCS=false
CLEAN_BUILD=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --tests)
            ENABLE_TESTS=true
            shift
            ;;
        --docs)
            ENABLE_DOCS=true
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -t, --type TYPE     Build type (release|debug|profile)"
            echo "  --tests             Enable unit tests"
            echo "  --docs              Generate documentation"
            echo "  -c, --clean         Clean before build"
            echo "  -v, --verbose       Verbose output"
            echo "  -h, --help          Show this help"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build artifacts...${NC}"
    make clean
fi

# Set build flags
export CFLAGS=""
if [ "$BUILD_TYPE" = "debug" ]; then
    export CFLAGS="-g -O0 -DDEBUG"
elif [ "$BUILD_TYPE" = "profile" ]; then
    export CFLAGS="-g -O2 -pg"
else
    export CFLAGS="-O3 -flto"
fi

if [ "$ENABLE_TESTS" = true ]; then
    export CFLAGS="$CFLAGS -DRUN_TESTS"
fi

if [ "$VERBOSE" = true ]; then
    export CFLAGS="$CFLAGS -v"
fi

# Build
echo -e "${GREEN}Building kernel (type: $BUILD_TYPE)...${NC}"
make

# Generate docs if requested
if [ "$ENABLE_DOCS" = true ]; then
    echo -e "${GREEN}Generating documentation...${NC}"
    make docs
fi

echo -e "${GREEN}Build completed successfully!${NC}"
