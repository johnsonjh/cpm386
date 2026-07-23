/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: e29e562e-8662-11f1-ad61-80ee73e9b8e7
 */

/*****************************************************************************/

/* julia.c */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30

/*****************************************************************************/

#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func), "i"(BDOS_INT),
                      "d"((ULONG)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

static void
putch (char c)
{
  bdos (2, (LONG)(UBYTE)c);
}

/*****************************************************************************/

static void
putnl (void)
{
  putch ('\n');
}

/*****************************************************************************/

static char
iter_to_char (int iter, int max_iter)
{
  if (iter >= max_iter)
    {
      return '0';
    }

  {
    static const char hex[16] =
      {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
      };

    if (iter < 0)
      {
        iter = 0;
      }

    if (iter > 15)
      {
        iter = 15;
      }

    return hex[iter];
  }
}

/*****************************************************************************/

void
_start (void)
{
  const LONG SCALE = 1L << 14;   /* reduced to avoid 32-bit overflow */

  const int WIDTH = 80;
  const int HEIGHT = 25;

  const LONG REAL_MIN = -2L * SCALE;
  const LONG REAL_SPAN = 3L * SCALE;
  const LONG IMAG_MIN = -1L * SCALE;
  const LONG IMAG_SPAN = 2L * SCALE;

  const int MAX_ITER = 16;

  /* Julia set parameter c = CR + i*CI (fixed-point) */
  const LONG CR = -11469L;  /* ≈ -0.7 * SCALE */
  const LONG CI =  4427L;   /* ≈  0.27015 * SCALE */

  int y, x;

  for (y = 0; y < HEIGHT; ++y)
    {
      LONG zi0 = IMAG_MIN + (IMAG_SPAN * (LONG)y) / (LONG)(HEIGHT - 1);

      for (x = 0; x < WIDTH; ++x)
        {
          LONG zr0 = REAL_MIN + (REAL_SPAN * (LONG)x) / (LONG)(WIDTH - 1);

          LONG zr = zr0;
          LONG zi = zi0;
          int iter = 0;

          while (iter < MAX_ITER)
            {
              LONG zr2 = (zr * zr - zi * zi) / SCALE;
              LONG zi2 = (2L * zr * zi) / SCALE;

              zr = zr2 + CR;
              zi = zi2 + CI;

              if ((zr * zr + zi * zi) > (4L * SCALE * SCALE))
                {
                  break;
                }

              ++iter;
            }

          if ('0' == iter_to_char (iter, MAX_ITER))
            putch (' ');
          else
            putch (iter_to_char (iter, MAX_ITER));
        }

      putnl ();
    }

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
