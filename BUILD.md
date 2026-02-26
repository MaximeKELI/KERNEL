# Build Instructions

## Prerequisites

Ensure you have the following tools installed:

- **GCC** (GNU Compiler Collection) - x86_64 target
- **NASM** (Netwide Assembler) - version 2.10 or later
- **GNU Make** - version 3.80 or later
- **GRUB2** tools - specifically `grub-mkrescue`
- **QEMU** (optional, for testing)

### Installing Prerequisites

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential nasm grub-pc-bin grub-common xorriso qemu-system-x86
```

#### Fedora/RHEL:
```bash
sudo dnf install gcc nasm grub2-tools xorriso qemu-system-x86_64
```

#### Arch Linux:
```bash
sudo pacman -S base-devel nasm grub qemu
```

#### macOS (with Homebrew):
```bash
brew install nasm qemu
# GRUB2 may need to be built from source or use alternative bootloader
```

## Building the Kernel

### Basic Build

```bash
# Build kernel and create ISO
make

# This will:
# 1. Compile all C source files
# 2. Assemble all assembly files
# 3. Link everything into kernel.elf
# 4. Create bootable ISO image
```

### Clean Build

```bash
# Remove all build artifacts
make clean

# Then rebuild
make
```

## Output Files

After building, you'll find:

- `build/kernel.elf` - The compiled kernel ELF binary
- `build/kernel.iso` - Bootable ISO image
- `iso/` - ISO directory structure

## Running the Kernel

### QEMU (Recommended for Development)

```bash
# Simple run
make run

# Or manually:
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio

# With debug output
make run-debug

# Or manually:
qemu-system-x86_64 -cdrom build/kernel.iso -m 512M -serial stdio -d int -no-reboot
```

### VirtualBox

1. Create a new virtual machine:
   - Type: Other
   - Version: Other/Unknown (64-bit)
   - Memory: 512 MB or more

2. Add the ISO:
   - Settings → Storage
   - Add optical drive
   - Select `build/kernel.iso`

3. Start the VM

### Real Hardware

**WARNING**: Running on real hardware may cause data loss or hardware damage. Use at your own risk.

1. Create a bootable USB from the ISO:
   ```bash
   sudo dd if=build/kernel.iso of=/dev/sdX bs=4M status=progress
   ```

2. Boot from USB on target machine

## Troubleshooting

### Build Errors

**"nasm: command not found"**
- Install NASM: `sudo apt-get install nasm` (or equivalent)

**"grub-mkrescue: command not found"**
- Install GRUB2 tools: `sudo apt-get install grub-pc-bin`

**"undefined reference" errors**
- Check that all source files are included in Makefile
- Run `make clean` and rebuild

### Runtime Issues

**Kernel doesn't boot**
- Check QEMU version (should be recent)
- Verify ISO was created correctly: `file build/kernel.iso`
- Try with debug: `make run-debug`

**Screen is blank**
- Check serial output: `-serial stdio` flag
- Kernel may be panicking - check for error messages

**"Multiboot2 magic not found"**
- Ensure GRUB2 is being used (not GRUB Legacy)
- Check `grub.cfg` is correct

## Development Tips

1. **Incremental Builds**: Make automatically rebuilds only changed files
2. **Debug Symbols**: Built with `-g` flag for debugging
3. **Serial Output**: All kernel output goes to serial (stdio in QEMU)
4. **Clean Often**: Run `make clean` if you encounter strange build issues

## Next Steps

After successful build and boot:

1. Check kernel initialization messages
2. Test keyboard input (if implemented)
3. Verify memory management
4. Test system calls (if userland exists)

For development, consider:
- Adding GDB support for debugging
- Implementing more drivers
- Adding userland programs
- Extending filesystem support
