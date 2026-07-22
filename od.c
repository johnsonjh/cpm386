/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 24fae2dc-82b5-11f1-9c88-80ee73e9b8e7
 */

/*****************************************************************************/

/* od.c - octal dump for CP/M-386 */

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

static int opt_h = 0, opt_i = 0, opt_o = 0, opt_w = 0, opt_n = 0;
static unsigned bytes_per_line = 16, hex_start = 0, ascii_start = 64;
static unsigned long file_offset = 0;
static UBYTE out_dma[128];
static int out_col = 0;
static UBYTE out_fcb[36];

static char line[81];
static unsigned col;

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
  if (!opt_w)
    {
      bdos (2, (LONG)(unsigned char)c);

      return;
    }

  out_dma[out_col++] = c;
  if (out_col == 128)
    {
      bdos (26, (LONG)(unsigned long)out_dma);
      bdos (21, (LONG)(unsigned long)out_fcb);
      out_col = 0;
    }
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
out_flush_close (void)
{
  if (opt_w)
    {
      if (out_col > 0)
        {
          while (out_col < 128) out_dma[out_col++] = 26;
          bdos (26, (LONG)(unsigned long)out_dma);
          bdos (21, (LONG)(unsigned long)out_fcb);
        }
      bdos (16, (LONG)(unsigned long)out_fcb);
    }
}

/*****************************************************************************/

static void
line_clear (void)
{
  unsigned j;
  for (j = 0; j < 80; j++) line[j] = ' ';
  line[80] = 0;
  col = 0;
}

/*****************************************************************************/

static void
line_flush (void)
{
  int k = 79;
  while (k >= 0 && line[k] == ' ') k--;
  line[k + 1] = 0;
  puts (line);
  putch ('\r');
  putch ('\n');
  line_clear ();
}

/*****************************************************************************/

static void
octaldump_feed (const UBYTE *buf, unsigned size)
{
  unsigned i;
  unsigned char byte;
  int d1, d2, d3, pos;

  for (i = 0; i < size; i++)
    {
      if (col == 0)
        {
          line_clear ();
          if (opt_o)
            {
              unsigned long tmp = file_offset;
              int k;
              line[11] = ':';
              line[12] = ' ';
              for (k = 10; k >= 0; k--)
                {
                  line[k] = (char)('0' + (tmp & 7));
                  tmp >>= 3;
                }
            }
        }

      byte = buf[i];
      d1 = byte / 64;
      d2 = (byte % 64) / 8;
      d3 = byte % 8;
      pos = (int)(hex_start + col * 4);
      line[pos] = (char)('0' + d1);
      line[pos + 1] = (char)('0' + d2);
      line[pos + 2] = (char)('0' + d3);

      if (!opt_n)
        {
          if (byte >= 32 && byte < 127)
            {
              line[ascii_start + col] = (char)byte;
            }
          else
            {
              line[ascii_start + col] = '.';
            }
        }

      col++;
      file_offset++;

      if (col == bytes_per_line)
        {
          line_flush ();
        }
    }
}

/*****************************************************************************/

static void
octaldump_end (void)
{
  if (col != 0)
    {
      line_flush ();
    }
}

/*****************************************************************************/

static void
set_fcb (UBYTE *fcb, const char *name)
{
  int i, j = 1;

  for (i = 0; i < 36; i++) fcb[i] = 0;
  for (i = 1; i <= 11; i++) fcb[i] = ' ';

  if (name[0] && name[1] == ':')
    {
      char c = name[0];
      if (c >= 'a' && c <= 'z') c -= 32;
      fcb[0] = (UBYTE)(c - 'A' + 1);
      name += 2;
    }

  while (*name && *name != '.' && *name != ' ' && *name != '\t' && *name != '\r')
    {
      char c = *name++;
      if (c >= 'a' && c <= 'z') c -= 32;
      if (j <= 8) fcb[j++] = (UBYTE)c;
    }

  if (*name == '.')
    {
      name++;
      j = 9;
      while (*name && *name != ' ' && *name != '\t' && *name != '\r')
        {
          char c = *name++;
          if (c >= 'a' && c <= 'z') c -= 32;
          if (j <= 11) fcb[j++] = (UBYTE)c;
        }
    }
}

/*****************************************************************************/

void
_start (void)
{
  UBYTE fcb[36];
  opt_h = opt_i = opt_o = opt_w = opt_n = 0;
  file_offset = out_col = col = 0;
  UBYTE dma[128];
  UBYTE prev[128];
  UWORD r;
  UBYTE lrbc = 0;
  int have = 0;
  int i;
  char tail[128];
  int tlen = CMD_TAIL[0];
  char *filename = 0;

  if (tlen > 126) tlen = 126;
  for (i = 0; i < tlen; i++) tail[i] = (char)CMD_TAIL[1 + i];
  tail[tlen] = 0;

  i = 0;
  while (tail[i] == ' ' || tail[i] == '\t') i++;
  while (tail[i])
    {
      if (tail[i] == '-' || tail[i] == '/')
        {
          i++;
          while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
            {
              char c = tail[i++];
              if (c >= 'A' && c <= 'Z') c += 32;
              if (c == 'h') opt_h = 1;
              else if (c == 'i') opt_i = 1;
              else if (c == 'o') opt_o = 1;
              else if (c == 'w') opt_w = 1;
              else if (c == 'n') opt_n = 1;
              else opt_h = 1;
            }
        }
      else
        {
          filename = &tail[i];

          break;
        }
      while (tail[i] == ' ' || tail[i] == '\t') i++;
    }

  if (opt_h || !filename || !*filename)
    {
      puts ("Usage: OD [-h] [-i] [-o] [-w] [-n] filename\r\n");
      puts ("  -h  help\r\n");
      puts ("  -i  ignore LRBC\r\n");
      puts ("  -o  prefix line with offset\r\n");
      puts ("  -w  write output to FILENAME.OCT\r\n");
      puts ("  -n  disable ASCII display\r\n");

      bdos (0, 0);
    }

  set_fcb (fcb, filename);
  fcb[12] = 0;
  fcb[14] = 0;
  fcb[32] = 0xFF;
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts ("File not found\r\n");

      bdos (0, 0);
    }

  lrbc = fcb[32];
  fcb[32] = 0;

  if (opt_i) lrbc = 0;

  if (opt_w)
    {
      for (i = 0; i < 36; i++) out_fcb[i] = fcb[i];
      out_fcb[9] = 'O';
      out_fcb[10] = 'C';
      out_fcb[11] = 'T';
      out_fcb[12] = out_fcb[13] = out_fcb[14] = out_fcb[15] = 0;
      out_fcb[32] = 0;
      bdos (19, (LONG)(unsigned long)out_fcb);
      r = bdos (22, (LONG)(unsigned long)out_fcb);
      if (r > 3)
        {
          opt_w = 0;
          puts ("Cannot create output file\r\n");
        }
    }

  hex_start = opt_o ? 13 : 0;
  bytes_per_line = (80 - hex_start) / (opt_n ? 4 : 5);
  ascii_start = hex_start + bytes_per_line * 4;

  col = 0;
  bdos (26, (LONG)(unsigned long)dma);

  for (;;)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          break;
        }

      if (have)
        {
          octaldump_feed (prev, 128);
        }

      for (i = 0; i < 128; i++)
        {
          prev[i] = dma[i];
        }

      have = 1;
    }

  if (have)
    {
      unsigned n = 128;

      if (lrbc != 0)
        {
          n = lrbc;
        }

      octaldump_feed (prev, n);
    }

  octaldump_end ();
  out_flush_close ();
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
