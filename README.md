# 64-bit Operating System Kernel

A production-grade, high-performance 64-bit operating system kernel written in C and x86_64 Assembly.

## Features

- **Architecture**: x86_64
- **Boot**: GRUB2/Multiboot2 compliant
- **Memory Management**: Physical and virtual memory managers with paging support
- **Process Management**: Process and thread support with scheduler
- **Interrupts**: Full IDT, ISR, and IRQ handling with PIC support
- **System Calls**: syscall/sysret interface
- **Drivers**: VGA, keyboard, timer, ATA
- **File System**: VFS layer with extensible filesystem support
- **Modular Design**: Clean architecture for easy extension

## Project Structure

```
kernel/
├── boot/           # Bootloader code
├── kernel/         # Kernel core
│   ├── memory/     # Memory management
│   ├── process/    # Process and scheduler
│   ├── interrupt/  # Interrupt handling
│   ├── drivers/    # Device drivers
│   ├── fs/         # File system
│   └── syscall/    # System call interface
├── lib/            # Library functions
├── include/        # Header files
├── build/          # Build output
└── iso/            # ISO image files
```

## Building

### Prerequisites

- GCC (x86_64 cross-compiler or native)
- NASM
- GNU Make
- GRUB2 tools (grub-mkrescue)

### Build Commands

```bash
# Build kernel and create ISO
make

# Clean build artifacts
make clean

# Run in QEMU
make run

# Run in QEMU with debug output
make run-debug
```

## Running

### QEMU

```bash
make run
```

Or manually:
```bash
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio
```

### VirtualBox

1. Create a new VM (64-bit, Other/Unknown)
2. Set memory to at least 512MB
3. Add the ISO (`build/kernel.iso`) as a CD/DVD drive
4. Start the VM

## Architecture

### Memory Management

- **Physical Memory Manager (PMM)**: Bitmap-based allocator
- **Virtual Memory Manager (VMM)**: 4-level paging (PML4, PDPT, PD, PT)
- **Kernel Heap**: Block-based allocator with coalescing

### Process Management

- Process structure with full CPU context
- Round-robin scheduler (extensible to advanced schedulers)
- Context switching support
- Preemptive multitasking ready

### Interrupts

- 256-entry IDT
- Exception handlers for all x86_64 exceptions
- IRQ handlers for hardware interrupts
- PIC initialization and management

### System Calls

- syscall/sysret instruction-based interface
- System call table with extensible handlers
- User/kernel mode separation

### Drivers

- **VGA**: Text mode driver with cursor support
- **Keyboard**: PS/2 keyboard driver with scancode translation
- **Timer**: PIT-based timer with configurable frequency
- **ATA**: Basic ATA disk driver

## Extending the Kernel

### Adding a Driver

1. Create driver file in `kernel/drivers/`
2. Implement driver interface
3. Register with device manager
4. Initialize in `kernel_main()`

### Adding a System Call

1. Add syscall number to `include/syscall.h`
2. Implement handler in `kernel/syscall/syscall.c`
3. Add to syscall table
4. Update syscall handler

### Adding a Filesystem

1. Implement `vfs_fs_ops_t` interface
2. Register with VFS
3. Mount filesystem

## Code Quality

- Clean, professional code structure
- Comprehensive comments
- Modular design
- Extensible architecture
- Production-ready foundation

## Future Enhancements

The kernel is designed to support:

- Full userland
- Advanced schedulers (CFS, etc.)
- Modern file systems (ext2, ext4, etc.)
- Networking stack
- GUI support
- Multi-core/SMP support
- Advanced memory management
- Security features

## License

This kernel is provided as a foundation for learning and development.

## Notes

- The kernel boots in QEMU and should work on real hardware
- Memory management is simplified but functional
- Some features are stubbed for future implementation
- The design allows for easy extension and enhancement
