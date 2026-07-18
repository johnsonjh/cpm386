/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 6ecd1e9c-82b6-11f1-9a75-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * user.ld - link transient programs at classic CP/M load address 0x100
 * (TPA-relative). Base page 0x00-0xFF is reserved for FCBs / command tail.
 * ENTRY(_start) must be the first byte of the image (load_off == entry_off).
 *
 * Separate R-X and R-W PT_LOAD segments so ld does not warn about RWX.
 * After objcopy -O binary the image is still a flat blob for the loader.
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
    . = 0x100;

    .text : {
        /* Keep program entry at the front of the binary image. */
        *(.text._start)
        *(.text.startup)
        *(.text*)
        *(.rodata*)
    } :text

    .data : {
        *(.data*)
    } :data

    .bss : {
        *(.bss*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}

/*****************************************************************************/
