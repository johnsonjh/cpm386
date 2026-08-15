/*
 * CP/M-386 - vgacon.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 6c93229a-8caf-11f1-911d-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgacon.c - VGA text console */

/*****************************************************************************/

#include "absaddr.h"
#include "io.h"
#include "platform.h"
#include "vgacon.h"

/*****************************************************************************/

/*
 * BIOS data area fields the video BIOS maintains.  Reading these back after
 * a mode set is more reliable than any table we could carry, and it is the
 * only way to learn the geometry of a mode the BIOS chose for us.
 */

#define BDA_VIDEO_MODE 0x449 /* byte: current mode number         */
#define BDA_COLUMNS 0x44A    /* word: text columns                */
#define BDA_CRTC_PORT 0x463  /* word: CRTC index port: 3D4 or 3B4 */
#define BDA_ROWS_M1 0x484    /* byte: text rows - 1               */
#define BDA_CELL_H 0x485     /* word: character cell scan lines   */

/*****************************************************************************/

static struct
{
  volatile unsigned short *mem;
  unsigned cols;
  unsigned rows;
  unsigned cell_h;
  unsigned long base;
  unsigned long map_size;
  unsigned short crtc;
  unsigned char attr;
  unsigned row;
  unsigned col;
  unsigned char cur_start; /* CRTC 0x0A scan line, w/o disable bit */
  unsigned char cur_end;   /* CRTC 0x0B scan line                  */
  unsigned char cur_shown;
  unsigned char cur_blink;
} vc = {
  (volatile unsigned short *)VGACON_DEF_BASE,
  VGACON_DEF_COLS,
  VGACON_DEF_ROWS,
  VGACON_DEF_CELLH,
  VGACON_DEF_BASE,
  (unsigned long)CPM386_VGA_TEXT_SIZE,
  VGACON_DEF_CRTC,
  VGACON_DEF_ATTR,
  0,
  0,
  13,
  14,
  1,
  1
};

/*****************************************************************************/

/* Software cursor. */

static struct
{
  unsigned char active;
  unsigned row;
  unsigned col;
  unsigned short painted;
  unsigned short saved;
} swc;

/*****************************************************************************/

static void vgacon_cursor_adopt (void);

/*****************************************************************************/

static void
swcur_lift (void)
{
  unsigned long off;

  if (!swc.active)
    {
      return;
    }

  swc.active = 0;

  if (swc.row >= vc.rows || swc.col >= vc.cols)
    {
      return;
    }

  off = (unsigned long)swc.row * vc.cols + swc.col;

  /* Only undo what is still ours. */
  if (vc.mem [off] == swc.painted)
    {
      vc.mem [off] = swc.saved;
    }
}

/*****************************************************************************/

static void
swcur_place (unsigned row, unsigned col)
{
  unsigned long off;
  unsigned short cell;

  if (row >= vc.rows || col >= vc.cols)
    {
      return;
    }

  off = (unsigned long)row * vc.cols + col;
  cell = vc.mem [off];

  swc.row = row;
  swc.col = col;
  swc.saved = cell;
  swc.painted = (unsigned short)((cell & 0x00FF)
                                 | (unsigned short)((cell & 0x0F00) << 4)
                                 | (unsigned short)((cell & 0xF000) >> 4));
  vc.mem [off] = swc.painted;
  swc.active = 1;
}

/*****************************************************************************/

unsigned
vgacon_cols (void)
{
  return vc.cols;
}

/*****************************************************************************/

unsigned
vgacon_rows (void)
{
  return vc.rows;
}

/*****************************************************************************/

unsigned
vgacon_row (void)
{
  return vc.row;
}

/*****************************************************************************/

unsigned
vgacon_col (void)
{
  return vc.col;
}

/*****************************************************************************/

unsigned char
vgacon_attr (void)
{
  return vc.attr;
}

/*****************************************************************************/

void
vgacon_set_attr (unsigned char attr)
{
  vc.attr = attr;
}

/*****************************************************************************/

unsigned
vgacon_cell_h (void)
{
  return vc.cell_h;
}

/*****************************************************************************/

unsigned
vgacon_cell_bytes (void)
{
  return VGACON_CELL_BYTES;
}

/*****************************************************************************/

unsigned long
vgacon_base (void)
{
  return vc.base;
}

/*****************************************************************************/

unsigned long
vgacon_map_size (void)
{
  return vc.map_size;
}

/*****************************************************************************/

unsigned short
vgacon_crtc (void)
{
  return vc.crtc;
}

/*****************************************************************************/

int
vgacon_probe (void)
{
#if !CPM386_HAS_VGA_TEXT
  return 0;
#else
  volatile unsigned short *p = vc.mem;
  unsigned short save0 = p [0];
  unsigned short save1 = p [1];
  int ok;

  /* Two different patterns, so memory stuck will not pass test */
  p [0] = 0xA55A;
  p [1] = 0x5AA5;
  ok = (p [0] == 0xA55A && p [1] == 0x5AA5);

  if (ok)
    {
      p [0] = 0x1234;
      ok = (p [0] == 0x1234 && p [1] == 0x5AA5);
    }

  p [0] = save0;
  p [1] = save1;

  return ok;
#endif
}

/*****************************************************************************/

void
vgacon_geometry (unsigned cols, unsigned rows, unsigned cell_h,
                 unsigned long base, unsigned short crtc)
{
  /* Anything wrong here means we were handed junk and to keep the defaults */
  if (cols < 20 || cols > 255)
    {
      cols = VGACON_DEF_COLS;
    }

  if (rows < 10 || rows > 255)
    {
      rows = VGACON_DEF_ROWS;
    }

  if (cell_h < 2 || cell_h > 32)
    {
      cell_h = VGACON_DEF_CELLH;
    }

  if (base == 0)
    {
      base = VGACON_DEF_BASE;
    }

  if (crtc != 0x3D4 && crtc != 0x3B4)
    {
      crtc = VGACON_DEF_CRTC;
    }

  vc.cols = cols;
  vc.rows = rows;
  vc.cell_h = cell_h;
  vc.base = base;
  vc.mem = (volatile unsigned short *)base;
  vc.crtc = crtc;
  vc.row = 0;
  vc.col = 0;

  /* How much of the plane a program may touch through the ring-3 selector */
  vc.map_size = (unsigned long)cols * rows * VGACON_CELL_BYTES;

  if (vc.map_size > (unsigned long)CPM386_VGA_TEXT_SIZE)
    {
      vc.map_size = (unsigned long)CPM386_VGA_TEXT_SIZE;
    }

  /*
   * A new mode means a new cell height, so the cached cursor scan lines are
   * stale: an underline at 13..14 is off the bottom of an eight line cell.
   * Take the shape the video BIOS just programmed instead.  The visibility
   * and blink settings are policy and are left alone.
   */

  vgacon_cursor_adopt ();
}

/*****************************************************************************/

void
vgacon_adopt_bda (void)
{
  unsigned cols = *ABS_U16 (BDA_COLUMNS);
  unsigned rows = (unsigned)*ABS_U8 (BDA_ROWS_M1) + 1u;
  unsigned cell_h = *ABS_U16 (BDA_CELL_H);
  unsigned short crtc = *ABS_U16 (BDA_CRTC_PORT);

  /*
   * A monochrome adapter puts the plane at B0000.  Everything else this
   * driver supports is at B8000.
   */

  unsigned long base = (crtc == 0x3B4) ? 0xB0000UL : 0xB8000UL;

  vgacon_geometry (cols, rows, cell_h, base, crtc);
}

/*****************************************************************************/

void
vgacon_cell (unsigned row, unsigned col, unsigned char ch, unsigned char attr)
{
  if (row >= vc.rows || col >= vc.cols)
    {
      return;
    }

  swcur_lift ();

  vc.mem [row * vc.cols + col]
      = (unsigned short)((unsigned short)ch
                         | ((unsigned short)attr << 8));
}

/*****************************************************************************/

void
vgacon_read_cell (unsigned row, unsigned col, unsigned char *ch,
                  unsigned char *attr)
{
  unsigned short cell;

  if (row >= vc.rows || col >= vc.cols)
    {
      if (ch)
        {
          *ch = ' ';
        }

      if (attr)
        {
          *attr = vc.attr;
        }

      return;
    }

  swcur_lift ();

  cell = vc.mem [row * vc.cols + col];

  if (ch)
    {
      *ch = (unsigned char)(cell & 0xFF);
    }

  if (attr)
    {
      *attr = (unsigned char)((cell >> 8) & 0xFF);
    }
}

/*****************************************************************************/

void
vgacon_fill (unsigned r0, unsigned c0, unsigned r1, unsigned c1,
             unsigned char ch, unsigned char attr)
{
  unsigned short cell
      = (unsigned short)((unsigned short)ch | ((unsigned short)attr << 8));
  unsigned r, c;

  swcur_lift ();

  if (r1 >= vc.rows)
    {
      r1 = vc.rows - 1;
    }

  if (c1 >= vc.cols)
    {
      c1 = vc.cols - 1;
    }

  for (r = r0; r <= r1 && r < vc.rows; r++)
    {
      for (c = c0; c <= c1 && c < vc.cols; c++)
        {
          vc.mem [r * vc.cols + c] = cell;
        }
    }
}

/*****************************************************************************/

void
vgacon_scroll_region (unsigned top, unsigned bot, int n, unsigned char attr)
{
  unsigned short blank
      = (unsigned short)(' ' | ((unsigned short)attr << 8));
  unsigned lines;
  unsigned span;
  unsigned r, c;

  if (n == 0 || vc.rows == 0)
    {
      return;
    }

  swcur_lift ();

  if (bot >= vc.rows)
    {
      bot = vc.rows - 1;
    }

  if (top > bot)
    {
      return;
    }

  span = bot - top + 1;
  lines = (unsigned)((n < 0) ? -n : n);

  if (lines >= span)
    {
      vgacon_fill (top, 0, bot, vc.cols - 1, ' ', attr);

      return;
    }

  if (n > 0)
    {
      /* Content moves toward `top`; blank rows appear at `bot`. */
      for (r = top; r + lines <= bot; r++)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c]
                  = vc.mem [(r + lines) * vc.cols + c];
            }
        }

      for (r = bot + 1 - lines; r <= bot; r++)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c] = blank;
            }
        }
    }
  else
    {
      /* Content moves toward `bot`; blank rows appear at `top`. */
      for (r = bot + 1; r-- > top + lines;)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c]
                  = vc.mem [(r - lines) * vc.cols + c];
            }
        }

      for (r = top; r < top + lines; r++)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c] = blank;
            }
        }
    }
}

/*****************************************************************************/

void
vgacon_scroll (int n)
{
  if (vc.rows == 0)
    {
      return;
    }

  vgacon_scroll_region (0, vc.rows - 1, n, vc.attr);
}

/*****************************************************************************/

void
vgacon_ins_chars (unsigned row, unsigned col, unsigned n, unsigned char attr)
{
  unsigned short blank
      = (unsigned short)(' ' | ((unsigned short)attr << 8));
  volatile unsigned short *line;
  unsigned c;

  if (row >= vc.rows || col >= vc.cols || n == 0)
    {
      return;
    }

  swcur_lift ();

  if (n > vc.cols - col)
    {
      n = vc.cols - col;
    }

  line = vc.mem + (unsigned long)row * vc.cols;

  for (c = vc.cols; c-- > col + n;)
    {
      line [c] = line [c - n];
    }

  for (c = col; c < col + n; c++)
    {
      line [c] = blank;
    }
}

/*****************************************************************************/

void
vgacon_del_chars (unsigned row, unsigned col, unsigned n, unsigned char attr)
{
  unsigned short blank
      = (unsigned short)(' ' | ((unsigned short)attr << 8));
  volatile unsigned short *line;
  unsigned c;

  if (row >= vc.rows || col >= vc.cols || n == 0)
    {
      return;
    }

  swcur_lift ();

  if (n > vc.cols - col)
    {
      n = vc.cols - col;
    }

  line = vc.mem + (unsigned long)row * vc.cols;

  for (c = col; c + n < vc.cols; c++)
    {
      line [c] = line [c + n];
    }

  for (c = vc.cols - n; c < vc.cols; c++)
    {
      line [c] = blank;
    }
}

/*****************************************************************************/

void
vgacon_cursor (unsigned row, unsigned col)
{
  unsigned long pos;

  if (row >= vc.rows)
    {
      row = vc.rows - 1;
    }

  if (col > vc.cols)
    {
      col = vc.cols;
    }

  swcur_lift ();

  vc.row = row;
  vc.col = col;

  pos = (unsigned long)row * vc.cols + col;

  outb (vc.crtc, 0x0E); /* cursor location high */
  outb ((unsigned short)(vc.crtc + 1), (unsigned char)((pos >> 8) & 0xFF));
  outb (vc.crtc, 0x0F); /* cursor location low */
  outb ((unsigned short)(vc.crtc + 1), (unsigned char)(pos & 0xFF));

  if (vc.cur_shown && !vc.cur_blink)
    {
      swcur_place (row, col);
    }
}

/*****************************************************************************/

void
vgacon_clear (void)
{
  vgacon_fill (0, 0, vc.rows - 1, vc.cols - 1, ' ', vc.attr);
  vgacon_cursor (0, 0);
}

/*****************************************************************************/

/* Push the cached shape and visibility back out to CRTC 0x0A / 0x0B. */

static void
vgacon_cursor_program (void)
{
  unsigned char start = (unsigned char)(vc.cur_start & 0x1F);

  /*
   * The hardware cursor is only used when it is both wanted and allowed to
   * blink; a steady cursor is painted into the plane by swcur_place().
   */

  if (!vc.cur_shown || !vc.cur_blink)
    {
      start |= 0x20; /* CRTC 0x0A bit 5 blanks the cursor */
    }

  outb (vc.crtc, 0x0A);
  outb ((unsigned short)(vc.crtc + 1), start);
  outb (vc.crtc, 0x0B);
  outb ((unsigned short)(vc.crtc + 1), (unsigned char)(vc.cur_end & 0x1F));

  if (vc.cur_shown && !vc.cur_blink)
    {
      swcur_place (vc.row, vc.col);
    }
  else
    {
      swcur_lift ();
    }
}

/*****************************************************************************/

void
vgacon_cursor_visible (int on)
{
  vc.cur_shown = (unsigned char)(on ? 1 : 0);
  vgacon_cursor_program ();
}

/*****************************************************************************/

int
vgacon_cursor_shown (void)
{
  return vc.cur_shown ? 1 : 0;
}

/*****************************************************************************/

void
vgacon_cursor_blink (int on)
{
  vc.cur_blink = (unsigned char)(on ? 1 : 0);
  vgacon_cursor_program ();
}

/*****************************************************************************/

int
vgacon_cursor_blinks (void)
{
  return vc.cur_blink ? 1 : 0;
}

/*****************************************************************************/

void
vgacon_cursor_shape (unsigned start, unsigned end)
{
  if (end >= vc.cell_h)
    {
      end = vc.cell_h ? vc.cell_h - 1 : 0;
    }

  if (start > end)
    {
      start = end;
    }

  vc.cur_start = (unsigned char)(start & 0x1F);
  vc.cur_end = (unsigned char)(end & 0x1F);
  vgacon_cursor_program ();
}

/*****************************************************************************/

unsigned
vgacon_cursor_start (void)
{
  return vc.cur_start;
}

/*****************************************************************************/

unsigned
vgacon_cursor_end (void)
{
  return vc.cur_end;
}

/*****************************************************************************/

/*
 * Latch the cursor shape the BIOS programmed for the current mode.  Doing
 * this rather than assuming an underline means ESC f / ESC[?25l can hide
 * the cursor and ESC e / ESC[?25h can put back exactly what was there,
 * whatever the cell height turns out to be.
 */

static void
vgacon_cursor_adopt (void)
{
  unsigned char a, b;

  outb (vc.crtc, 0x0A);
  a = inb ((unsigned short)(vc.crtc + 1));
  outb (vc.crtc, 0x0B);
  b = inb ((unsigned short)(vc.crtc + 1));

  vc.cur_start = (unsigned char)(a & 0x1F);
  vc.cur_end = (unsigned char)(b & 0x1F);

  /*
   * A hidden or nonsensical cursor is replaced by a two scan line underline
   * at the bottom of the cell, which is what every text mode BIOS uses.
   */

  if ((a & 0x20) != 0 || vc.cur_end == 0 || vc.cur_start > vc.cur_end
      || vc.cur_end >= vc.cell_h)
    {
      unsigned h = (vc.cell_h >= 2 && vc.cell_h <= 32) ? vc.cell_h : 16;

      vc.cur_end = (unsigned char)(h - 1);
      vc.cur_start = (unsigned char)(h - 2);
    }

  swc.active = 0; /* the mode set wiped whatever we had painted */
  vgacon_cursor_program ();
}

/*****************************************************************************/

void
vgacon_init (void)
{
  vgacon_adopt_bda ();
  vgacon_clear ();

  /* Display starts at the beginning of the plane (CRTC 0x0C/0x0D). */
  outb (vc.crtc, 0x0C);
  outb ((unsigned short)(vc.crtc + 1), 0);
  outb (vc.crtc, 0x0D);
  outb ((unsigned short)(vc.crtc + 1), 0);

  vc.cur_shown = 1;
  vc.cur_blink = 1;
  vgacon_cursor_adopt ();
  vgacon_cursor (0, 0);
}

/*****************************************************************************/

/* Advance one line, scrolling when the cursor falls off the bottom. */

static void
vgacon_newline (void)
{
  vc.col = 0;

  if (++vc.row >= vc.rows)
    {
      vc.row = vc.rows - 1;
      vgacon_scroll (1);
    }
}

/*****************************************************************************/

/*
 * Byte stream handler.  Behaviour is deliberately unchanged from the
 * original vga_putc(); the VT102/VT52 interpreter replaces this body and
 * nothing beneath it.
 */

void
vgacon_putc (unsigned char c)
{
  if (c == '\r')
    {
      vc.col = 0;
      vgacon_cursor (vc.row, vc.col);

      return;
    }

  if (c == '\n')
    {
      vgacon_newline ();
      vgacon_cursor (vc.row, vc.col);

      return;
    }

  if (c == '\b')
    {
      if (vc.col > 0)
        {
          vc.col--;
        }

      vgacon_cursor (vc.row, vc.col);

      return;
    }

  if (c == '\t')
    {
      vc.col = (vc.col + 8u) & ~7u;

      if (vc.col >= vc.cols)
        {
          vgacon_newline ();
        }

      vgacon_cursor (vc.row, vc.col);

      return;
    }

  /* printable, or a control character we render as-is */
  if (vc.col >= vc.cols)
    {
      vgacon_newline ();
    }

  vgacon_cell (vc.row, vc.col, c, vc.attr);
  vc.col++;
  vgacon_cursor (vc.row, vc.col);
}

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
