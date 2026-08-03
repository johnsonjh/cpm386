/*
 * CP/M-386
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
  0
};

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

  vc.mem [row * vc.cols + col]
      = (unsigned short)((unsigned short)ch
                         | ((unsigned short)attr << 8));
}

/*****************************************************************************/

void
vgacon_fill (unsigned r0, unsigned c0, unsigned r1, unsigned c1,
             unsigned char ch, unsigned char attr)
{
  unsigned short cell
      = (unsigned short)((unsigned short)ch | ((unsigned short)attr << 8));
  unsigned r, c;

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
vgacon_scroll (int n)
{
  unsigned short blank
      = (unsigned short)(' ' | ((unsigned short)vc.attr << 8));
  unsigned lines;
  unsigned r, c;

  if (n == 0)
    {
      return;
    }

  lines = (unsigned)((n < 0) ? -n : n);

  if (lines >= vc.rows)
    {
      vgacon_fill (0, 0, vc.rows - 1, vc.cols - 1, ' ', vc.attr);

      return;
    }

  if (n > 0)
    {
      /* Content moves toward row 0; blank rows appear at the bottom. */
      for (r = 0; r + lines < vc.rows; r++)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c]
                  = vc.mem [(r + lines) * vc.cols + c];
            }
        }

      for (r = vc.rows - lines; r < vc.rows; r++)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c] = blank;
            }
        }
    }
  else
    {
      /* Content moves toward the last row; blank rows appear at the top. */
      for (r = vc.rows; r-- > lines;)
        {
          for (c = 0; c < vc.cols; c++)
            {
              vc.mem [r * vc.cols + c]
                  = vc.mem [(r - lines) * vc.cols + c];
            }
        }

      for (r = 0; r < lines; r++)
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

  vc.row = row;
  vc.col = col;

  pos = (unsigned long)row * vc.cols + col;

  outb (vc.crtc, 0x0E); /* cursor location high */
  outb ((unsigned short)(vc.crtc + 1), (unsigned char)((pos >> 8) & 0xFF));
  outb (vc.crtc, 0x0F); /* cursor location low */
  outb ((unsigned short)(vc.crtc + 1), (unsigned char)(pos & 0xFF));
}

/*****************************************************************************/

void
vgacon_clear (void)
{
  vgacon_fill (0, 0, vc.rows - 1, vc.cols - 1, ' ', vc.attr);
  vgacon_cursor (0, 0);
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
