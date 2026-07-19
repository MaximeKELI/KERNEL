# Makefile for 64-bit OS Kernel
# Production-grade build system

# Compiler and tools
CC = gcc
AS = nasm
LD = ld

# Architecture
ARCH = x86_64

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
LIB_DIR = lib
INCLUDE_DIR = include
# Separate object trees for test vs prod so switching RUN_TESTS never reuses a
# stale object compiled with different flags (which caused "undefined reference
# to register_*_tests" link failures when flipping between builds).
ifeq ($(RUN_TESTS),1)
BUILD_DIR = build/test
else
BUILD_DIR = build/prod
endif
# User programs are ABI-identical across variants; build them once in a shared
# location that the *_blob.S files incbin from ("build/user/...").
USER_BUILD_DIR = build/user
ISO_DIR = iso
ISO_BOOT_DIR = $(ISO_DIR)/boot
ISO_GRUB_DIR = $(ISO_DIR)/boot/grub

# Flags
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
         -Wall -Wextra -std=gnu11 -O2 -g \
         -fno-omit-frame-pointer -fstrict-aliasing \
         -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter \
         -Wno-sign-compare -Wno-cast-qual -Wno-pedantic \
         -I$(KERNEL_DIR)/ai
ASFLAGS = -f elf64
LDFLAGS = -T linker.ld -nostdlib -static -z max-page-size=0x1000
LDFLAGS_GCC = -ffreestanding $(LDFLAGS)

# Source files
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/*.c) \
                 $(wildcard $(KERNEL_DIR)/memory/*.c) \
                 $(wildcard $(KERNEL_DIR)/process/*.c) \
                 $(wildcard $(KERNEL_DIR)/interrupt/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/*.c) \
                 $(wildcard $(KERNEL_DIR)/fs/*.c) \
                 $(wildcard $(KERNEL_DIR)/syscall/*.c) \
                 $(wildcard $(KERNEL_DIR)/signal/*.c) \
                 $(wildcard $(KERNEL_DIR)/ipc/*.c) \
                 $(wildcard $(KERNEL_DIR)/device/*.c) \
                 $(wildcard $(KERNEL_DIR)/module/*.c) \
                 $(wildcard $(KERNEL_DIR)/cache/*.c) \
                 $(wildcard $(KERNEL_DIR)/log/*.c) \
                 $(wildcard $(KERNEL_DIR)/kthread/*.c) \
                 $(wildcard $(KERNEL_DIR)/workqueue/*.c) \
                 $(wildcard $(KERNEL_DIR)/elf/*.c) \
                 $(wildcard $(KERNEL_DIR)/timer/*.c) \
                 $(wildcard $(KERNEL_DIR)/net/*.c) \
                 $(wildcard $(KERNEL_DIR)/smp/*.c) \
                 $(wildcard $(KERNEL_DIR)/acpi/*.c) \
                 $(wildcard $(KERNEL_DIR)/io/*.c) \
                 $(wildcard $(KERNEL_DIR)/rcu/*.c) \
                 $(wildcard $(KERNEL_DIR)/security/*.c) \
                 $(wildcard $(KERNEL_DIR)/namespace/*.c) \
                 $(wildcard $(KERNEL_DIR)/cgroup/*.c) \
                 $(wildcard $(KERNEL_DIR)/virt/*.c) \
                 $(wildcard $(KERNEL_DIR)/trace/*.c) \
                 $(wildcard $(KERNEL_DIR)/block/*.c) \
                 $(wildcard $(KERNEL_DIR)/watchdog/*.c) \
                 $(wildcard $(KERNEL_DIR)/ai/*.c) \
                 $(wildcard $(KERNEL_DIR)/bpf/*.c) \
                 $(wildcard $(KERNEL_DIR)/livepatch/*.c) \
                 $(wildcard $(KERNEL_DIR)/numa/*.c) \
                 $(wildcard $(KERNEL_DIR)/hotplug/*.c) \
                 $(wildcard $(KERNEL_DIR)/perf/*.c) \
                 $(wildcard $(KERNEL_DIR)/kexec/*.c) \
                 $(wildcard $(KERNEL_DIR)/efi/*.c) \
                 $(wildcard $(KERNEL_DIR)/iommu/*.c) \
                 $(wildcard $(KERNEL_DIR)/power/*.c) \
                 $(wildcard $(KERNEL_DIR)/devicetree/*.c) \
                 $(wildcard $(KERNEL_DIR)/container/*.c) \
                 $(wildcard $(KERNEL_DIR)/checkpoint/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/usb/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/net/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/gpu/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/audio/*.c) \
                 $(wildcard $(KERNEL_DIR)/drivers/input/*.c) \
                 $(wildcard $(KERNEL_DIR)/media/*.c) \
                 $(wildcard $(KERNEL_DIR)/vdso/*.c) \
                 $(wildcard $(LIB_DIR)/*.c)

KERNEL_SOURCES += $(KERNEL_DIR)/test/test.c
ifeq ($(RUN_TESTS),1)
CFLAGS += -DRUN_TESTS
KERNEL_SOURCES += $(wildcard $(KERNEL_DIR)/test/tests_*.c)
endif

USER_NETTEST = $(USER_BUILD_DIR)/nettest
USER_SH = $(USER_BUILD_DIR)/sh
NETTEST_BLOB = $(BUILD_DIR)/kernel/nettest_blob.o
SH_BLOB = $(BUILD_DIR)/kernel/sh_blob.o

KERNEL_ASM_SOURCES = $(wildcard $(KERNEL_DIR)/interrupt/*.S) \
                     $(wildcard $(KERNEL_DIR)/syscall/*.S) \
                     $(filter-out $(KERNEL_DIR)/asm/numa.S,$(wildcard $(KERNEL_DIR)/asm/*.S))

BOOT_SOURCES = $(BOOT_DIR)/boot.asm

# Object files
KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(BUILD_DIR)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
BOOT_OBJECTS = $(BOOT_SOURCES:%.asm=$(BUILD_DIR)/%.o)
ALL_OBJECTS = $(BOOT_OBJECTS) $(KERNEL_OBJECTS) $(KERNEL_ASM_OBJECTS) $(NETTEST_BLOB) $(SH_BLOB)

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = $(BUILD_DIR)/kernel.iso

.PHONY: all clean run run-qemu iso boot-bench nettest test docs benchmark deploy check release profile install-scripts

all: $(KERNEL_ELF) iso

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/memory
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/process
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/interrupt
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/syscall
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/signal
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/ipc
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/device
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/module
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/cache
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/log
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/kthread
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/workqueue
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/elf
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/timer
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/net
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/smp
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/acpi
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/io
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/rcu
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/security
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/namespace
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/cgroup
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/virt
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/trace
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/block
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/watchdog
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/ai
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/bpf
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/livepatch
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/numa
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/hotplug
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/perf
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/kexec
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/efi
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/iommu
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/power
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/devicetree
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/container
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/checkpoint
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/test
	mkdir -p $(BUILD_DIR)/$(LIB_DIR)
	mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	mkdir -p $(ISO_BOOT_DIR) $(ISO_GRUB_DIR)

# Compile C sources
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Assemble ASM sources
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Assemble kernel ASM sources (nettest blob built separately — needs user ELF first)
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel/nettest_blob.o: $(USER_NETTEST) $(KERNEL_DIR)/nettest_blob.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(KERNEL_DIR)/nettest_blob.S -o $@

$(BUILD_DIR)/kernel/sh_blob.o: $(USER_SH) $(KERNEL_DIR)/sh_blob.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(KERNEL_DIR)/sh_blob.S -o $@

# Userland (embedded + copied to ISO)
$(USER_NETTEST) $(USER_SH): user/Makefile
	$(MAKE) -C user

# Link kernel
$(KERNEL_ELF): $(ALL_OBJECTS) $(USER_NETTEST) linker.ld
	$(CC) $(LDFLAGS_GCC) $(ALL_OBJECTS) -o $@

# Create ISO
iso: $(KERNEL_ELF) $(USER_NETTEST) grub.cfg
	mkdir -p $(ISO_BOOT_DIR) $(ISO_GRUB_DIR)
	cp $(KERNEL_ELF) $(ISO_BOOT_DIR)/
	cp $(USER_NETTEST) $(ISO_BOOT_DIR)/nettest
	cp $(USER_SH) $(ISO_BOOT_DIR)/sh
	cp grub.cfg $(ISO_GRUB_DIR)/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

boot-bench: iso
	@bash scripts/boot_bench.sh

# QEMU: prefer system package (/usr/bin), then AppImage in ~/.local/bin
QEMU ?= $(firstword $(wildcard /usr/bin/qemu-system-x86_64) $(HOME)/.local/bin/qemu-system-x86_64)

# Run kernel directly in QEMU (no ISO)
run-kernel: $(KERNEL_ELF)
	@test -n "$(QEMU)" || (echo "QEMU not found. Install: apt install qemu-system-x86 OR see ~/.local/bin/QEMU-AppImage"; exit 1)
	$(QEMU) -kernel $(KERNEL_ELF) -m 512M -serial mon:stdio -display none -no-reboot

# QEMU drive: grub-mkrescue produces a hybrid ISO — boot as disk, not -cdrom
QEMU_ISO_DRIVE = -drive file=$(ISO_IMAGE),format=raw,if=ide,index=0,media=disk

# QEMU user networking + virtio-net (fallback rtl8139 if virtio missing)
QEMU_NETDEV ?= -netdev user,id=net0 -device virtio-net-pci,netdev=net0

# Run in QEMU from ISO (Multiboot2 via GRUB)
run: iso
	@test -n "$(QEMU)" || (echo "QEMU not found"; exit 1)
	$(QEMU) $(QEMU_ISO_DRIVE) -boot order=c -m 512M $(QEMU_NETDEV) -serial mon:stdio -display none -no-reboot

# QEMU with VGA display (video blit visible)
run-video: iso
	@test -n "$(QEMU)" || (echo "QEMU not found"; exit 1)
	$(QEMU) $(QEMU_ISO_DRIVE) -boot order=c -m 512M $(QEMU_NETDEV) -serial mon:stdio -vga std -no-reboot

# Run in QEMU with debug (GRUB/kernel messages on serial if configured)
run-debug: iso
	@test -n "$(QEMU)" || (echo "QEMU not found"; exit 1)
	$(QEMU) $(QEMU_ISO_DRIVE) -boot order=c -m 512M -serial stdio -display none -no-reboot -d guest_errors

# Generate documentation
docs:
	@echo "Generating API documentation..."
	doxygen Doxyfile

# Clean build artifacts
clean:
	rm -rf build $(ISO_DIR) docs

# Run tests
test:
	@bash scripts/test.sh

# Run benchmarks
benchmark:
	@bash scripts/benchmark.sh

# Deploy kernel
deploy:
	@bash scripts/deploy.sh

# Check code quality
check:
	@bash scripts/check_code.sh

# Generate release
release:
	@bash scripts/gen_release.sh

# Profile kernel
profile:
	@bash scripts/profiler.sh

# Install scripts
install-scripts:
	chmod +x scripts/*.sh
	chmod +x tools/*.py
