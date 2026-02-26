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
BUILD_DIR = build
ISO_DIR = iso
ISO_BOOT_DIR = $(ISO_DIR)/boot
ISO_GRUB_DIR = $(ISO_DIR)/boot/grub

# Flags
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
         -Wall -Wextra -Wpedantic -Werror -std=c11 -O3 -flto -g \
         -fno-omit-frame-pointer -fstack-usage -fstrict-aliasing \
         -Wstrict-prototypes -Wmissing-prototypes -Wuninitialized \
         -Wmaybe-uninitialized -Wshadow -Wcast-qual -Wcast-align
ASFLAGS = -f elf64
LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000 -flto \
          -Wl,--gc-sections -Wl,--as-needed

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
                 $(wildcard $(KERNEL_DIR)/test/*.c) \
                 $(wildcard $(LIB_DIR)/*.c) \
                 $(wildcard $(KERNEL_DIR)/test/tests_*.c)

KERNEL_ASM_SOURCES = $(wildcard $(KERNEL_DIR)/interrupt/*.S) \
                     $(wildcard $(KERNEL_DIR)/syscall/*.S) \
                     $(wildcard $(KERNEL_DIR)/asm/*.S)

BOOT_SOURCES = $(BOOT_DIR)/boot.asm

# Object files
KERNEL_OBJECTS = $(KERNEL_SOURCES:%.c=$(BUILD_DIR)/%.o)
KERNEL_ASM_OBJECTS = $(KERNEL_ASM_SOURCES:%.S=$(BUILD_DIR)/%.o)
BOOT_OBJECTS = $(BOOT_SOURCES:%.asm=$(BUILD_DIR)/%.o)
ALL_OBJECTS = $(BOOT_OBJECTS) $(KERNEL_OBJECTS) $(KERNEL_ASM_OBJECTS)

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = $(BUILD_DIR)/kernel.iso

.PHONY: all clean run run-qemu iso test docs benchmark deploy check release profile install-scripts

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

# Assemble kernel ASM sources
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_ELF): $(ALL_OBJECTS) linker.ld
	$(LD) $(LDFLAGS) $(ALL_OBJECTS) -o $@

# Create ISO
iso: $(KERNEL_ELF) grub.cfg
	mkdir -p $(ISO_BOOT_DIR) $(ISO_GRUB_DIR)
	cp $(KERNEL_ELF) $(ISO_BOOT_DIR)/
	cp grub.cfg $(ISO_GRUB_DIR)/
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

# Run in QEMU
run: iso
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M -serial stdio

# Run in QEMU with debug
run-debug: iso
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -m 512M -serial stdio -d int -no-reboot

# Generate documentation
docs:
	@echo "Generating API documentation..."
	doxygen Doxyfile

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) docs

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
