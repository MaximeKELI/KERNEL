#!/bin/bash
# Release generation script

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

VERSION=""
RELEASE_TYPE="stable"

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -t|--type)
            RELEASE_TYPE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -v, --version VERSION   Version number (e.g., 1.0.0)"
            echo "  -t, --type TYPE         Release type (stable|beta|alpha)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ -z "$VERSION" ]; then
    echo -e "${YELLOW}Version not specified, using default${NC}"
    VERSION="1.0.0"
fi

echo -e "${GREEN}Generating release v$VERSION ($RELEASE_TYPE)...${NC}"

# Clean and build
make clean
make CFLAGS="-O3 -flto -DRELEASE_VERSION=\\\"$VERSION\\\""

# Generate documentation
make docs

# Create release directory
RELEASE_DIR="release/v$VERSION"
mkdir -p "$RELEASE_DIR"

# Copy files
cp build/kernel.elf "$RELEASE_DIR/"
cp build/kernel.iso "$RELEASE_DIR/"
cp -r docs "$RELEASE_DIR/" 2>/dev/null || true

# Create release notes
cat > "$RELEASE_DIR/RELEASE_NOTES.md" << EOF
# Kernel Release v$VERSION

**Release Type**: $RELEASE_TYPE  
**Date**: $(date)

## Changes

- See CHANGELOG.md for detailed changes

## Installation

1. Build ISO: \`make\`
2. Boot from ISO or USB

## Documentation

See docs/ directory for API documentation.

EOF

# Create checksums
cd "$RELEASE_DIR"
sha256sum kernel.elf kernel.iso > SHA256SUMS
cd ../..

echo -e "${GREEN}Release v$VERSION created in $RELEASE_DIR${NC}"
