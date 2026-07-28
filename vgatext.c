/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: fd6930d8-82b5-11f1-8667-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgatext.c: BDOS 224 direct VGA text test */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned long ULONG;

/*****************************************************************************/

#include "vgauser.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_CONIN 1
#define BDOS_CONOUT 2
#define BDOS_CONST 11

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
                      "d"((ULONG)info)
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

static void
puthex16 (unsigned v)
{
  static const char h [] = "0123456789ABCDEF";
  int i;

  for (i = 3; i >= 0; i--)
    {
      putch (h [(v >> (i * 4)) & 0xF]);
    }
}

/*****************************************************************************/

static void
putu (unsigned n)
{
  char b [8];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 8)
    {
      b [i++] = (char)('0' + n % 10);
      n /= 10;
    }

  while (i)
    {
      putch (b [--i]);
    }
}

/*****************************************************************************/

static void
puthex32 (ULONG v)
{
  static const char h [] = "0123456789ABCDEF";
  int i;

  puts ("0x");

  for (i = 7; i >= 0; i--)
    {
      putch (h [(v >> (i * 4)) & 0xF]);
    }
}

/*****************************************************************************/

static void
wait_key (void)
{
  while (bdos (BDOS_CONST, 0) == 0)
    {
      ;
    }

  (void)bdos (BDOS_CONIN, 0);
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct cpm_vga_text vi;
  UWORD r;
  unsigned short sel;
  unsigned cols, rows, cell;
  unsigned x, y, off;

  const char *msg = " *** CP/M-386 DIRECT VGA (BDOS 224) *** ";
  const char *hint = " Press any key to return to CCP ";
  const char *p;

  r = bdos (BDOS_CON_VIDEO, (LONG)(ULONG)&vi);

  if (r == 0xFFFF || vi.sel == 0)
    {
      puts ("VGATEXT: no user video map on this platform\r\n");

      bdos (0, 0);
    }

  sel = vi.sel;
  cols = vi.cols ? vi.cols : 80;
  rows = vi.rows ? vi.rows : 25;
  cell = vi.cell_bytes ? vi.cell_bytes : 2;

  puts ("VGATEXT map OK\r\n  selector=0x");
  puthex16 (sel);
  puts ("  phys=");
  puthex32 (vi.phys_base);
  puts ("  size=");
  putu ((unsigned)(vi.map_size / 1024));
  puts ("K  ");
  putu (cols);
  putch ('x');
  putu (rows);
  puts ("\r\n");
  puts ("Painting VGA page 0 via ES:offset; waiting for a key...\r\n");

  /* Fill page 0 w/blue background */
  for (y = 0; y < rows; y++)
    {
      for (x = 0; x < cols; x++)
        {
          off = (y * cols + x) * cell;
          vga_es_store16 (sel, (unsigned short)off,
                          (unsigned short)(' ' | (0x17 << 8)));
        }
    }

  /* title bar */
  p = msg;

  for (x = 0; x < cols; x++)
    {
      char ch = ' ';
      unsigned len = 0;
      const char *q = msg;

      while (*q)
        {
          len++;
          q++;
        }

      if (x >= (cols - len) / 2 && p && *p)
        {
          ch = *p++;
        }

      off = (0 * cols + x) * cell;
      vga_es_store16 (sel, (unsigned short)off,
                      (unsigned short)((unsigned char)ch | (0x1E << 8)));
    }

  /* hint row 2 */
  p = hint;
  x = 2;
  y = 2;

  while (*p && x < cols)
    {
      off = (y * cols + x) * cell;
      vga_es_store16 (sel, (unsigned short)off,
                      (unsigned short)((unsigned char)*p | (0x1F << 8)));
      p++;
      x++;
    }

  /* corner markers */
  vga_es_store16 (sel, 0, (unsigned short)('[' | (0x1E << 8)));
  off = (cols - 1) * cell;
  vga_es_store16 (sel, (unsigned short)off,
                  (unsigned short)(']' | (0x1E << 8)));
  off = ((rows - 1) * cols) * cell;
  vga_es_store16 (sel, (unsigned short)off,
                  (unsigned short)('+' | (0x1E << 8)));
  off = ((rows - 1) * cols + cols - 1) * cell;
  vga_es_store16 (sel, (unsigned short)off,
                  (unsigned short)('+' | (0x1E << 8)));

  /* optional page-1 marker at +4K */
  if (vi.map_size >= 4096UL + 2)
    {
      vga_es_store16 (sel, 4096, (unsigned short)('2' | (0x0A << 8)));
    }

  wait_key ();

  puts ("VGATEXT done.\r\n");

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
