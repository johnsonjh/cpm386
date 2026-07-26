/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 0db77da2-82b4-11f1-adbf-80ee73e9b8e7
 */

/*****************************************************************************/

/* aclockdv.c - direct VGA aclock for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "aclock.h"
#include "vgauser.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_CONOUT 2
#define BDOS_CONIN 1
#define BDOS_CONST 11
#define BDOS_GET_TOD 105

/*****************************************************************************/

struct cpm_datetime
{
  UWORD days; /*cppcheck-suppress unusedStructMember*/
  UBYTE hour;
  UBYTE min;
  UBYTE sec;
};

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func),
                      "i"(BDOS_INT),
                      "d"((unsigned long)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

static void
putch (char c)
{
  bdos (BDOS_CONOUT, (LONG)(unsigned char)c);
}

/*****************************************************************************/

static void
puts (const char *s)
{
  while (*s)
    {
      putch (*s++);
    }
}

/*****************************************************************************/

static int
key_ready (void)
{
  return bdos (BDOS_CONST, 0) != 0;
}

/*****************************************************************************/

static void
flush_key (void)
{
  while (key_ready ())
    {
      (void)bdos (BDOS_CONIN, 0);
    }
}

/*****************************************************************************/

static int
wait_next_second (UBYTE last_sec)
{
  struct cpm_datetime dt;

  for (;;)
    {
      if (key_ready ())
        {
          (void)bdos (BDOS_CONIN, 0);
          return 1;
        }

      if (bdos (BDOS_GET_TOD, (LONG)(unsigned long)&dt) == 0
          && dt.sec != last_sec)
        {
          return 0;
        }
    }
}

/*****************************************************************************/

static unsigned short g_sel;
static unsigned g_cols = 80, g_rows = 25, g_cell = 2;

/*****************************************************************************/

static void
vga_put (int x, int y, char ch, unsigned char attr)
{
  unsigned off;

  if (x < 1 || y < 1)
    {
      return;
    }

  if ((unsigned)x > g_cols || (unsigned)y > g_rows)
    {
      return;
    }

  off = ((unsigned)(y - 1) * g_cols + (unsigned)(x - 1)) * g_cell;

  if (off + 1 >= 0x8000u) /* stay inside 32K map */
    {
      return;
    }

  vga_es_store16 (g_sel, (unsigned short)off,
                  (unsigned short)((unsigned char)ch | (attr << 8)));
}

/*****************************************************************************/

static void
cls_vga (void)
{
  unsigned x, y, off;

  for (y = 0; y < g_rows; y++)
    {
      for (x = 0; x < g_cols; x++)
        {
          off = (y * g_cols + x) * g_cell;
          vga_es_store16 (g_sel, (unsigned short)off,
                          (unsigned short)(' ' | (0x07 << 8)));
        }
    }
}

/*****************************************************************************/

static void
draw_point (int x, int y, char c)
{
  vga_put (x, y, c, 0x0F);
}

/*****************************************************************************/

static void
draw_text (int x, int y, const char *s)
{
  while (*s)
    {
      vga_put (x, y, *s, 0x0E); /* yellow */
      x++;
      s++;
    }
}

/*****************************************************************************/

static void
draw_circle (void)
{
  int n;

  for (n = 0; n < 60; n++)
    {
      draw_point (circle[n][0], circle[n][1], (char)circle[n][2]);
    }
}

/*****************************************************************************/

static void
draw_hour (int n)
{
  int m;

  for (m = 0; m < 6; m++)
    {
      draw_point (hour[n][m][0], hour[n][m][1], 'h');
    }
}

/*****************************************************************************/

static void
draw_minute (int n)
{
  int m;

  for (m = 0; m < 8; m++)
    {
      draw_point (minute[n][m][0], minute[n][m][1], 'm');
    }
}

/*****************************************************************************/

static void
draw_seconds (int n)
{
  int m;

  for (m = 0; m < 8; m++)
    {
      draw_point (minute[n][m][0], minute[n][m][1], '.');
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct cpm_vga_text vi;
  struct cpm_datetime dt;
  int hidx;
  char dig[16];
  UWORD r;

  r = bdos (BDOS_CON_VIDEO, (LONG)(unsigned long)&vi);

  if (r == 0xFFFF || vi.sel == 0)
    {
      puts ("ACLOCKDV: no user VGA text map\r\n");

      bdos (0, 0);
    }

  g_sel = vi.sel;

  if (vi.cols)
    {
      g_cols = vi.cols;
    }

  if (vi.rows)
    {
      g_rows = vi.rows;
    }

  if (vi.cell_bytes)
    {
      g_cell = vi.cell_bytes;
    }

  flush_key ();

  for (;;)
    {
      if (bdos (BDOS_GET_TOD, (LONG)(unsigned long)&dt) != 0)
        {
          dt.hour = dt.min = dt.sec = 0;
        }

      cls_vga ();

      draw_circle ();

      hidx = ((dt.hour >= 12 ? dt.hour - 12 : dt.hour) * 5) + (dt.min / 10);

      if (hidx < 0)
        {
          hidx = 0;
        }

      if (hidx > 59)
        {
          hidx = 59;
        }

      draw_hour (hidx);

      draw_minute (dt.min % 60);

      draw_seconds (dt.sec % 60);

      draw_text (35, 6, ".:ACLOCK:.");

      dig[0] = '[';
      dig[1] = (char)('0' + (dt.hour / 10) % 10);
      dig[2] = (char)('0' + dt.hour % 10);
      dig[3] = ':';
      dig[4] = (char)('0' + (dt.min / 10) % 10);
      dig[5] = (char)('0' + dt.min % 10);
      dig[6] = ':';
      dig[7] = (char)('0' + (dt.sec / 10) % 10);
      dig[8] = (char)('0' + dt.sec % 10);
      dig[9] = ']';
      dig[10] = 0;

      draw_text (35, 19, dig);

      if (wait_next_second (dt.sec))
        {
          break;
        }
    }

  puts ("\r\n");

  bdos (0, 0);
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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
