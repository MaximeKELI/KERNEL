#!/usr/bin/env bash
# Build and run kernel phase A–D smoke checks (host compile + optional QEMU)
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== Phase build ==="
make build/kernel.elf
make iso

echo "=== Unit tests (RUN_TESTS=1) ==="
make RUN_TESTS=1 build/kernel.elf 2>&1 | tail -5

echo "=== Done ==="
echo "QEMU: make run  then  init-full  →  phases | fork-test | exec nettest"
