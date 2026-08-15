/*
 * CP/M-386 - vgacon.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 7e78c91a-8caf-11f1-937a-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgacon.h - VGA text console: geometry, cell primitives, byte stream */

/*****************************************************************************/

#ifndef VGACON_H
# define VGACON_H

/*****************************************************************************/

/*
 * The console done into two layers:
 *
 *   - The primitives below (cell / fill / scroll / cursor) know about the
 *     text plane and nothing else.  They are done so that an escape
 *     sequence interpreter needs: fill() serves ESC[J and ESC[K, a signed
 *     scroll() serves ESC[L and ESC[M, cursor() serves ESC[H.
 *
 *   - vgacon_putc() is the byte stream handler.  It does what the old
 *     vga_putc() did (CR, LF, BS, TAB, printable) and nothing more; the
 *     VT102/VT52/DRI interpreter in vgaterm.c sits on top of the
 *     primitives instead, so this remains the "no emulation" path used by
 *     boot diagnostics and by anything that wants raw glyph output.
 *
 * Geometry is runtime state, not compile-time constants, so a mode change
 * is just a call to vgacon_geometry().
 */

/*****************************************************************************/

/* Defaults used when there is no BIOS data area worth believing. */

# define VGACON_DEF_BASE 0xB8000UL
# define VGACON_DEF_COLS 80
# define VGACON_DEF_ROWS 25
# define VGACON_DEF_CELLH 16
# define VGACON_DEF_CRTC 0x3D4

/* One text cell is a character byte plus an attribute byte. */
# define VGACON_CELL_BYTES 2

/* Default attribute: light grey on black. */
# define VGACON_DEF_ATTR 0x07

/*****************************************************************************/

/*
 * Presence test.  Writes and restores two cells of the text plane; the
 * video hole is never populated by the memory controller, so with no
 * adapter fitted the writes are dropped and the reads float.
 */

int vgacon_probe (void);

/*
 * Adopt the geometry left by the BIOS (or by a mode set) and clear.  Safe
 * to call repeatedly.
 */

void vgacon_init (void);

/*
 * Re-describe the plane after a mode change.  cell_h is the character cell
 * height in scan lines (8, 14 or 16); it is kept for the font code and for
 * the cursor shape.  Values that fail a sanity check are replaced by the
 * VGACON_DEF_* defaults.  Resets the cursor to the origin.
 */

void vgacon_geometry (unsigned cols, unsigned rows, unsigned cell_h,
                      unsigned long base, unsigned short crtc);

/*
 * Re-read the BIOS data area (0x449/0x44A/0x484/0x485/0x463) and adopt what
 * it reports.  This is how geometry is learned after an int 10h mode set:
 * the video BIOS updates the BDA, so there is no need for per-mode tables
 * and it doubles as a check that the mode really took effect.
 */

void vgacon_adopt_bda (void);

/*****************************************************************************/

/* Primitives */

void vgacon_cell (unsigned row, unsigned col, unsigned char ch,
                  unsigned char attr);
void vgacon_read_cell (unsigned row, unsigned col, unsigned char *ch,
                       unsigned char *attr);
void vgacon_fill (unsigned r0, unsigned c0, unsigned r1, unsigned c1,
                  unsigned char ch, unsigned char attr);

/* n > 0 scrolls up (content moves toward row 0), n < 0 scrolls down. */
void vgacon_scroll (int n);

/*
 * The same, confined to rows top..bot inclusive and filling the vacated
 * rows with attr.  This is what a DECSTBM scrolling region, ESC[L (IL) and
 * ESC[M (DL) all reduce to; vgacon_scroll() is this over the whole screen
 * with the current attribute.
 */

void vgacon_scroll_region (unsigned top, unsigned bot, int n,
                           unsigned char attr);

/*
 * Character insert / delete within one row, from col to the right margin.
 * Vacated cells are filled with a blank in attr.  ESC[@ (ICH) and ESC[P
 * (DCH).
 */

void vgacon_ins_chars (unsigned row, unsigned col, unsigned n,
                       unsigned char attr);
void vgacon_del_chars (unsigned row, unsigned col, unsigned n,
                       unsigned char attr);

void vgacon_cursor (unsigned row, unsigned col);
void vgacon_clear (void);

/*
 * Cursor visibility, blink and shape.  The shape is the CRTC scan line pair
 * (0x0A/0x0B); vgacon_init() latches whatever the BIOS left there so that
 * hiding and re-showing the cursor restores the original block/underline
 * rather than guessing at one.
 *
 * VGA cannot stop the hardware cursor blinking, so a steady cursor is drawn
 * in the plane instead: the hardware cursor is hidden and the cell under the
 * logical cursor is rendered with its attribute nibbles swapped.
 */

void vgacon_cursor_visible (int on);
int vgacon_cursor_shown (void);
void vgacon_cursor_blink (int on);
int vgacon_cursor_blinks (void);
void vgacon_cursor_shape (unsigned start, unsigned end);
unsigned vgacon_cursor_start (void);
unsigned vgacon_cursor_end (void);

/*****************************************************************************/

/* Byte stream - CR / LF / BS / TAB / printable, no escape interpretation. */

void vgacon_putc (unsigned char c);

/*****************************************************************************/

/* Live state, for BDOS 224, the mode code and the terminal emulator. */

unsigned vgacon_cols (void);
unsigned vgacon_rows (void);
unsigned vgacon_row (void);
unsigned vgacon_col (void);
unsigned char vgacon_attr (void);
void vgacon_set_attr (unsigned char attr);
unsigned vgacon_cell_h (void);
unsigned vgacon_cell_bytes (void);
unsigned long vgacon_base (void);
unsigned long vgacon_map_size (void);
unsigned short vgacon_crtc (void);

/*****************************************************************************/

#endif /* ifndef VGACON_H */

/*****************************************************************************/

/*
 * Local Variables:
 * mode: c
 * indent-tabs-mode: nil
 * tab-width: 2
 * c-basic-offset: 2
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
