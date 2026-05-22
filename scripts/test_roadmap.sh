#!/bin/bash
set -e
cd "$(dirname "$0")/.."
make RUN_TESTS=1 build/kernel.elf 2>&1 | tail -20
echo "Roadmap build OK"
