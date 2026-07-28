/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 3cb46d0c-82b6-11f1-a3b5-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * linker.ld - flat 32-bit protected mode layout for CP/M-386
 * Load at 64K (for real-mode floppy loader compat)
 * kernel low, TPA after kernel+stack to end of RAM
 *
 * Separate R-X and R-W load segments so ld does not warn about RWX.
 * Flat identity map still maps everything; segment flags are ELF only.
 */

/*****************************************************************************/

ENTRY(_start)

/*****************************************************************************/

PHDRS
{
    text PT_LOAD FLAGS(5); /* PF_R | PF_X */
    data PT_LOAD FLAGS(6); /* PF_R | PF_W */
}

/*****************************************************************************/

SECTIONS
{
    . = 0x10000;  /* 64k for easy 16-bit real mode
                   * load + multiboot low load addr */

    /*
     * multiboot header must appear within the first 8KB of the loaded image.
     * placed first so it sits at the start of the first PT_LOAD
     */

    .multiboot ALIGN(4) :
    {
        KEEP(*(.multiboot))
    } :text

    /*
     * real _start code follows the header. The boot loader must call the
     * actual kernel_entry symbol (generated via nm), not hardcoded 0x10000.
     */

    .text.start ALIGN(4) :
    {
        *(.text.start)
    } :text

    .text ALIGN(4) :
    {
        *(.text)
        *(.text.*)
    } :text

    .rodata ALIGN(4) :
    {
        *(.rodata)
        *(.rodata.*)
    } :text

    .data ALIGN(4) :
    {
        *(.data)
        *(.data.*)
    } :data

    .bss ALIGN(4) :
    {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    } :data

    /*
     * Writable RAM disk AFTER all kernel data. TPA starts past this.
     * Block N is at physical offset N*blocksize into this array; if the
     * array were placed inside .rodata, writes would clobber kernel data.
     */

    .ramdisk ALIGN(16) :
    {
        *(.ramdisk)
    } :data

    /* provide end symbol for potential TPA or heap */
    end = .;
    __kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}

/*****************************************************************************/
