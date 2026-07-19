#!/bin/bash
# Kernel test runner / CI gate.
#
# Builds the test kernel (RUN_TESTS=1, isolated object tree build/test), boots it
# in QEMU, and exits non-zero unless every in-kernel test passes. Parses the
# "Total: N passed, M failed" summary the test harness prints on the serial port.

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TIMEOUT="${TEST_TIMEOUT:-120}"
ISO="build/test/kernel.iso"
LOG="$(mktemp /tmp/kernel_test.XXXXXX.log)"

echo -e "${GREEN}Building test kernel (RUN_TESTS=1)...${NC}"
make RUN_TESTS=1 iso

echo -e "${GREEN}Booting tests in QEMU (timeout ${TIMEOUT}s)...${NC}"
# Default NIC (e1000) is left in place on purpose: some tests probe it.
qemu-system-x86_64 \
    -drive file="${ISO}",format=raw,if=ide,index=0,media=disk \
    -boot order=c -m 512M \
    -serial file:"${LOG}" -display none -no-reboot \
    >/dev/null 2>&1 &
QEMU_PID=$!

# The kernel keeps running (idle shell) after printing the summary, so poll the
# serial log and stop QEMU as soon as the run finishes (or panics).
for ((i = 0; i < TIMEOUT; i++)); do
    if grep -qE "Total: [0-9]+ passed|!!! KERNEL PANIC !!!" "${LOG}" 2>/dev/null; then
        break
    fi
    if ! kill -0 "${QEMU_PID}" 2>/dev/null; then
        break
    fi
    sleep 1
done
kill -9 "${QEMU_PID}" 2>/dev/null || true
wait "${QEMU_PID}" 2>/dev/null || true

echo "----------------------------------------"

if grep -qE "!!! KERNEL PANIC !!!" "${LOG}"; then
    echo -e "${RED}Kernel PANIC during tests:${NC}"
    grep -A2 -E "!!! KERNEL PANIC !!!" "${LOG}" | head -n 6
    echo "Full serial log kept at: ${LOG}"
    exit 1
fi

SUMMARY="$(grep -E "Total: [0-9]+ passed" "${LOG}" | tail -n1 || true)"
if [ -z "${SUMMARY}" ]; then
    echo -e "${RED}No test summary found — the kernel did not finish the test run.${NC}"
    echo "Last serial output:"
    tail -n 20 "${LOG}"
    echo "Full serial log kept at: ${LOG}"
    exit 1
fi

echo -e "${YELLOW}${SUMMARY}${NC}"

PASSED="$(echo "${SUMMARY}" | sed -nE 's/.*Total: ([0-9]+) passed.*/\1/p')"
FAILED="$(echo "${SUMMARY}" | sed -nE 's/.*passed, ([0-9]+) failed.*/\1/p')"

if [ -z "${PASSED}" ] || [ -z "${FAILED}" ]; then
    echo -e "${RED}Could not parse the test summary line.${NC}"
    exit 1
fi

if [ "${FAILED}" -ne 0 ] || [ "${PASSED}" -eq 0 ]; then
    echo -e "${RED}Tests FAILED (${PASSED} passed, ${FAILED} failed). Log: ${LOG}${NC}"
    exit 1
fi

echo -e "${GREEN}All ${PASSED} tests passed.${NC}"
rm -f "${LOG}"
exit 0
