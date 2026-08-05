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

    /*
     * The loader reads the flat image and nothing more, so .bss arrives as
     * whatever the previous transient left in the TPA.  Programs that rely
     * on zero-initialised statics clear it themselves between these two
     * symbols; the rest cost nothing by their being defined.
     */

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        _end = .;
    } :data

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
    }
}

/*****************************************************************************/

/*
 * Local Variables:
 * mode: LD-Script
 * indent-tabs-mode: nil
 * tab-width: 4
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/*****************************************************************************/
/* vim: set ft=ld ts=4 sw=4 tw=4 ai expandtab cc=80 : */
/*****************************************************************************/
