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

    /* Discard other sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
