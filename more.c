/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/*****************************************************************************/

/* more.c: file pager for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

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
                      "d"((unsigned long)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

static void
putch (char c)
{
  bdos (2, (LONG)(unsigned char)c);
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
toupper_ch (int c)
{
  if (c >= 'a' && c <= 'z')
    {
      return c - 32;
    }

  return c;
}

/*****************************************************************************/

static int
getch_wait (void)
{
  int c;

  while (!(c = (int)bdos (6, 0xFF)))
    {
      ;
    }
  while (bdos (11, 0))
    {
      int d = (int)bdos (6, 0xFF) & 0xff;
      if (d != '\r' && d != '\n' && d != 0)
        {
          break;
        }
    }
  return c & 0xff;
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: MORE [-h] filename\r\n");
  puts ("  Space  next page\r\n");
  puts ("  Enter  next line\r\n");
  puts ("  Q/^C   quit\r\n");
}

/*****************************************************************************/

static void
fill_from_def_fcb (UBYTE *fcb)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = DEF_FCB[i];
    }
}

/*****************************************************************************/

static int
parse_tail_help (void)
{
  unsigned tlen = CMD_TAIL[0];
  unsigned i;
  char tail[128];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      tail[i] = (char)CMD_TAIL[1 + i];
    }

  tail[tlen] = 0;

  i = 0;

  while (tail[i] == ' ' || tail[i] == '\t')
    {
      i++;
    }

  while (tail[i])
    {
      if (tail[i] == '-' || tail[i] == '/')
        {
          i++;

          while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
            {
              if (toupper_ch ((unsigned char)tail[i]) == 'H')
                {
                  return 1;
                }

              i++;
            }
          while (tail[i] == ' ' || tail[i] == '\t')
            {
              i++;
            }

          continue;
        }

      break;
    }

  return 0;
}

/*****************************************************************************/

/* Clear "[More]" prompt */
static void
clear_more (void)
{
  puts ("\r      \r");
}

/*****************************************************************************/

void
_start (void)
{
  UBYTE fcb[36];
  UBYTE dma[128];
  UBYTE prev[128];
  UBYTE lrbc = 0;
  UWORD r;
  int process = 1;
  int col = 0, ctr = 0, space = 0;
  int have = 0;
  int i;
  unsigned rec_len;
  unsigned pos;
  int final = 0;

  if (parse_tail_help ())
    {
      help ();
      bdos (0, 0);
    }

  if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
    {
      help ();
      puts ("ERR: No file specified\r\n");
      bdos (0, 0);
    }

  fill_from_def_fcb (fcb);
  fcb[12] = 0;
  fcb[14] = 0;
  fcb[32] = 0xFF;
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts ("ERR: File not found\r\n");
      bdos (0, 0);
    }

  lrbc = fcb[32];
  fcb[32] = 0;

  bdos (26, (LONG)(unsigned long)dma);

  /* Read ahead one record so LRBC can trim the last */
  for (;;)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          final = 1;

          if (!have)
            {
              break; /* empty file */
            }

          rec_len = 128;

          if (lrbc != 0)
            {
              rec_len = lrbc;
            }

          /* fall through to process prev as last record */
        }
      else
        {
          if (have)
            {
              rec_len = 128;
            }
          else
            {
              /* first record: stash and read again */
              for (i = 0; i < 128; i++)
                {
                  prev[i] = dma[i];
                }

              have = 1;

              continue;
            }
        }

      /* Emit prev[0..rec_len) */
      pos = 0;

      while (process && pos < rec_len)
        {
          int c;

          if (space)
            {
              c = ' ';
              space--;
            }
          else
            {
              c = prev[pos++] & 0xff;
              if (c == 9)
                {
                  c = ' ';
                  space += 3;
                }
            }

          if (c == 26)
            {
              goto done;
            }

          if (c == '\r')
            {
              putch ('\r');
              col = 0;
            }
          else if (c == '\n')
            {
              putch ('\n');
              col = 0;
              ctr++;
            }
          else
            {
              putch ((char)c);
              col++;

              if (col >= 80)
                {
                  col = 0;
                  ctr++;
                }
            }

          if (ctr > 22)
            {
              int d, wt = 1;
              puts ("[More]");

              while (wt)
                {
                  d = getch_wait ();

                  switch (d)
                    {
                    case 'q':
                    case 'Q':
                    case 3:
                      clear_more ();
                      wt = 0;
                      process = 0;

                      break;

                    case 13:
                      clear_more ();
                      wt = 0;
                      ctr = 22;

                      break;

                    case 32:
                      clear_more ();
                      wt = 0;
                      ctr = 0;

                      break;

                    default:
                      break;
                    }
                }
            }
          else
            {
              while (bdos (11, 0))
                {
                  int d = (int)bdos (1, 0);

                  if (d == 3)
                    {
                      process = 0;

                      break;
                    }
                }
            }
        }

      if (!process || final)
        {
          break;
        }

      /* shift dma -> prev for next iteration */
      for (i = 0; i < 128; i++)
        {
          prev[i] = dma[i];
        }

      have = 1;
    }

done:
  bdos (16, (LONG)(unsigned long)fcb);

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
