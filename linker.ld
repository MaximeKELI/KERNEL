/* Linker script for 64-bit kernel */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1MB (0x100000) */
    . = 0x100000;

    /* Multiboot2 header must be in first 32KB */
    .multiboot : {
        *(.multiboot)
    }

    /* Kernel code */
    .text : ALIGN(4K) {
        *(.text)
    }

    /* Read-only data */
    .rodata : ALIGN(4K) {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K) {
        *(.data)
    }

    /* Read-write data (uninitialized) */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    /* End of the kernel image, page-aligned. The heap starts here so it can
     * never overlap kernel code/data/bss (which previously corrupted the TSS
     * ring-0 stack / descriptor tables once enough heap was allocated). */
    . = ALIGN(4K);
    _kernel_end = .;

    /* Discard other sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
