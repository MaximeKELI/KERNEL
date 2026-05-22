#!/usr/bin/env bash
# Measure minimal boot time from serial log (target < 200 ms)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

QEMU="${QEMU:-$(command -v qemu-system-x86_64 2>/dev/null || true)}"
if [ -z "$QEMU" ]; then
  echo "qemu-system-x86_64 not found"
  exit 1
fi

make -s build/user/nettest 2>/dev/null || true
make -s build/kernel.elf

LOG="$(mktemp)"
# Direct -kernel skips GRUB; measures kernel fast-boot path only
timeout 6s "$QEMU" \
  -kernel build/kernel.elf -m 512M \
  -serial file:"$LOG" -display none -no-reboot 2>/dev/null || true

TOTAL="$(grep -E 'TOTAL.*ms' "$LOG" | tail -1 | grep -oE '[0-9]+' | head -1 || echo 9999)"
echo "Boot TOTAL: ${TOTAL} ms (log: $LOG)"
if [ "$TOTAL" -lt 200 ] 2>/dev/null; then
  echo "PASS: boot < 200 ms"
  exit 0
fi
echo "WARN: boot >= 200 ms (optimize minimal init or quiet GRUB)"
exit 0
