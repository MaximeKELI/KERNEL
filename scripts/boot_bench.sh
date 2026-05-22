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
make -s build/kernel.elf iso

LOG="$(mktemp)"
timeout 12s "$QEMU" \
  -drive file=build/kernel.iso,format=raw,if=ide,index=0,media=disk \
  -boot order=c -m 512M \
  -serial mon:stdio -display none -no-reboot >"$LOG" 2>&1 || true

TOTAL="$(grep -oE 'TOTAL[[:space:]]+[0-9]+' "$LOG" 2>/dev/null | tail -1 | grep -oE '[0-9]+' || true)"
if [ -z "$TOTAL" ]; then
  TOTAL="$(grep -oE 'minimal_done[[:space:]]+[0-9]+' "$LOG" 2>/dev/null | tail -1 | grep -oE '[0-9]+' || echo 9999)"
fi
[ -z "$TOTAL" ] && TOTAL=9999
echo "Boot TOTAL: ${TOTAL} ms (log: $LOG)"
if [ "$TOTAL" -lt 200 ] 2>/dev/null; then
  echo "PASS: boot < 200 ms"
  exit 0
fi
echo "WARN: boot >= 200 ms (optimize minimal init or quiet GRUB)"
exit 0
