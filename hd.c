/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: b805f072-82b4-11f1-82bb-80ee73e9b8e7
 */

/*****************************************************************************/

/* hd.c - hex dump */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))
#define PAGER_LINES 23

/*****************************************************************************/

static int opt_h = 0, opt_i = 0, opt_o = 0, opt_w = 0, opt_n = 0;
static int opt_p = 0, opt_a = 0, opt_d = 0, opt_u = 0;
static ULONG opt_l = 0; /* 0 = no limit */
static ULONG opt_s = 0; /* 0 = no seek  */
static int opt_b_set = 0;
static unsigned opt_b = 0;
static unsigned bytes_per_line = 20, hex_start = 0, ascii_start = 60;
static ULONG file_offset = 0;
static UBYTE out_dma[128];
static int out_col = 0;
static UBYTE out_fcb[36];

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret = 0;

  (void)func;
  (void)info;

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
  if (!opt_w)
    {
      (void)bdos (2, (LONG)(unsigned char)c);

      return;
    }

  out_dma[out_col++] = c;
  if (out_col == 128)
    {
      (void)bdos (26, (LONG)(ULONG)out_dma);
      (void)bdos (21, (LONG)(ULONG)out_fcb);
      out_col = 0;
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
          while (out_col < 128)
            {
              out_dma[out_col++] = 26;
            }

          (void)bdos (26, (LONG)(ULONG)out_dma);
          (void)bdos (21, (LONG)(ULONG)out_fcb);
        }

      (void)bdos (16, (LONG)(ULONG)out_fcb);
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

static char
hexdig (unsigned v)
{
  v &= 0x0f;

  if (opt_u)
    {
      return (char)(v > 9 ? v + 'A' - 10 : v + '0');
    }

  return (char)(v > 9 ? v + 'a' - 10 : v + '0');
}

/*****************************************************************************/

static int pager_line_count = 0;
static int pager_quit = 0;

/*****************************************************************************/

static int
getch_wait (void)
{
  int c;

  while (!(c = (int)bdos (6, 0xFF)))
    {
      ;
    }

  return c & 0xff;
}

/*****************************************************************************/

static int
pager_check (void)
{
  if (!opt_p || opt_w || pager_quit)
    {
      return pager_quit;
    }

  pager_line_count++;

  if (pager_line_count >= PAGER_LINES)
    {
      int d;
      puts ("[More]");

      for (;;)
        {
          d = getch_wait ();

          if (d == 'q' || d == 'Q' || d == 3)
            {
              puts ("\r      \r");
              pager_quit = 1;

              return 1;
            }

          if (d == 13)
            {
              /* advance one line */
              puts ("\r      \r");
              pager_line_count = PAGER_LINES - 1;

              return 0;
            }

          if (d == 32)
            {
              /* advance one page */
              puts ("\r      \r");
              pager_line_count = 0;

              return 0;
            }
        }
    }

  return 0;
}

/*****************************************************************************/

/* Continuous dump state across sequential records. */
static char line[161]; /* wider for -b */
static unsigned col;   /* 0..bytes_per_line-1 bytes filled in line */

/*****************************************************************************/

static int prev_was_star = 0;
static ULONG bytes_dumped = 0;

/*****************************************************************************/

static void
line_clear (void)
{
  unsigned j;
  unsigned width
      = hex_start + bytes_per_line * 3 + (opt_n ? 0 : bytes_per_line) + 2;

  if (width > 160)
    {
      width = 160;
    }

  for (j = 0; j < width; j++)
    {
      line[j] = ' ';
    }

  line[width] = 0;
  col = 0;
}

/*****************************************************************************/

static int
line_all_null (void)
{
  unsigned j;

  for (j = 0; j < bytes_per_line; j++)
    {
      if (line[hex_start + j * 3] != '0' || line[hex_start + j * 3 + 1] != '0')
        {
          return 0;
        }
    }

  return 1;
}

/*****************************************************************************/

static void
line_flush (void)
{
  int k;
  unsigned width
      = hex_start + bytes_per_line * 3 + (opt_n ? 0 : bytes_per_line) + 2;

  if (width > 160)
    {
      width = 160;
    }

  if (pager_quit)
    {
      return;
    }

  if (opt_a && col == bytes_per_line && line_all_null ())
    {
      if (!prev_was_star)
        {
          puts ("*");
          putch ('\r');
          putch ('\n');

          if (pager_check ())
            {
              return;
            }

          prev_was_star = 1;
        }

      line_clear ();

      return;
    }

  prev_was_star = 0;

  k = (int)width - 1;

  while (k >= 0 && line[k] == ' ')
    {
      k--;
    }

  line[k + 1] = 0;
  puts (line);
  putch ('\r');
  putch ('\n');
  (void)pager_check ();
  line_clear ();
}

/*****************************************************************************/

static void
put_dec_offset (ULONG off)
{
  char buf[12];
  int i = 0;
  int k;

  if (off == 0)
    {
      buf[i++] = '0';
    }
  else
    {
      while (off)
        {
          buf[i++] = (char)('0' + off % 10);
          off /= 10;
        }
    }

  /* right-justify in 10 chars */
  for (k = 0; k < 10 - i; k++)
    {
      line[k] = ' ';
    }

  while (i > 0)
    {
      line[k++] = buf[--i];
    }

  line[10] = ':';
  line[11] = ' ';
}

/*****************************************************************************/

/* append raw file bytes to ongoing dump */
static void
hexdump_feed (const UBYTE *buf, unsigned size)
{
  unsigned i;
  unsigned char ch;

  for (i = 0; i < size; i++)
    {
      /* -l / limit: stop after N bytes */
      if (opt_l && bytes_dumped >= opt_l)
        {
          return;
        }

      if (pager_quit)
        {
          return;
        }

      if (col == 0)
        {
          line_clear ();
          if (opt_o)
            {
              if (opt_d)
                {
                  put_dec_offset (file_offset);
                }
              else
                {
                  ULONG tmp = file_offset;
                  int k;
                  line[8] = ':';
                  line[9] = ' ';
                  for (k = 7; k >= 0; k--)
                    {
                      line[k] = hexdig (tmp);
                      tmp >>= 4;
                    }
                }
            }
        }

      ch = buf[i];
      line[hex_start + col * 3] = hexdig (ch >> 4);
      line[hex_start + col * 3 + 1] = hexdig (ch);

      if (!opt_n)
        {
          if (ch < 0x7f && ch > (unsigned char)' ')
            {
              line[ascii_start + col] = (char)ch;
            }
          else
            {
              line[ascii_start + col] = '.';
            }
        }

      col++;
      file_offset++;
      bytes_dumped++;

      if (col == bytes_per_line)
        {
          line_flush ();
        }

      /* -l/limit, check again after incrementing */
      if (opt_l && bytes_dumped >= opt_l)
        {
          if (col != 0)
            {
              line_flush ();
            }

          return;
        }
    }
}

/*****************************************************************************/

static void
hexdump_end (void)
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

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  for (i = 1; i <= 11; i++)
    {
      fcb[i] = ' ';
    }

  if (name[0] && name[1] == ':')
    {
      char c = name[0];
      if (c >= 'a' && c <= 'z')
        {
          c -= 32;
        }

      fcb[0] = (UBYTE)(c - 'A' + 1);
      name += 2;
    }

  while (*name && *name != '.' && *name != ' ' && *name != '\t'
         && *name != '\r')
    {
      char c = *name++;
      if (c >= 'a' && c <= 'z')
        {
          c -= 32;
        }

      if (j <= 8)
        {
          fcb[j++] = (UBYTE)c;
        }
    }

  if (*name == '.')
    {
      name++;
      j = 9;
      while (*name && *name != ' ' && *name != '\t' && *name != '\r')
        {
          char c = *name++;
          if (c >= 'a' && c <= 'z')
            {
              c -= 32;
            }

          if (j <= 11)
            {
              fcb[j++] = (UBYTE)c;
            }
        }
    }
}

/*****************************************************************************/

static ULONG
parse_num (const char *s, int *len)
{
  ULONG v = 0;
  int n = 0;

  while (s[n] >= '0' && s[n] <= '9')
    {
      v = v * 10 + (ULONG)(s[n] - '0');
      n++;
    }

  *len = n;

  return v;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb[36];

  opt_h = opt_i = opt_o = opt_w = opt_n = 0;
  opt_p = opt_a = opt_d = opt_u = 0;
  opt_l = opt_s = 0;
  opt_b_set = 0;
  opt_b = 0;
  file_offset = out_col = col = 0;
  bytes_dumped = 0;
  pager_line_count = 0;
  pager_quit = 0;
  prev_was_star = 0;
  UBYTE dma[128];
  UBYTE prev[128];
  UWORD r;
  UBYTE lrbc = 0;
  int have = 0;
  int i;
  char tail[128];
  int tlen = CMD_TAIL[0];
  const char *filename = 0;

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
              char c = tail[i++];

              if (c >= 'A' && c <= 'Z')
                {
                  c += 32;
                }

              if (c == 'h')
                {
                  opt_h = 1;
                }
              else if (c == 'i')
                {
                  opt_i = 1;
                }
              else if (c == 'o')
                {
                  opt_o = 1;
                }
              else if (c == 'w')
                {
                  opt_w = 1;
                }
              else if (c == 'n')
                {
                  opt_n = 1;
                }
              else if (c == 'p')
                {
                  opt_p = 1;
                }
              else if (c == 'a')
                {
                  opt_a = 1;
                }
              else if (c == 'd')
                {
                  opt_d = 1;
                  opt_o = 1;
                }
              else if (c == 'u')
                {
                  opt_u = 1;
                }
              else if (c == 'b')
                {
                  int nlen = 0;
                  ULONG v;

                  if (tail[i] && tail[i] != ' ' && tail[i] != '\t')
                    {
                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }
                  else
                    {
                      while (tail[i] == ' ' || tail[i] == '\t')
                        {
                          i++;
                        }

                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }

                  if (nlen > 0 && v > 0 && v <= 64)
                    {
                      opt_b_set = 1;
                      opt_b = (unsigned)v;
                    }
                  else
                    {
                      puts ("Invalid -b value (1-64)\r\n");

                      (void)bdos (0, 0);
                    }

                  break;
                }
              else if (c == 'l')
                {
                  int nlen = 0;
                  ULONG v;

                  if (tail[i] && tail[i] != ' ' && tail[i] != '\t')
                    {
                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }
                  else
                    {
                      while (tail[i] == ' ' || tail[i] == '\t')
                        {
                          i++;
                        }

                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }

                  if (nlen > 0 && v > 0)
                    {
                      opt_l = v;
                    }
                  else
                    {
                      puts ("Invalid -l value\r\n");

                      (void)bdos (0, 0);
                    }

                  break;
                }
              else if (c == 's')
                {
                  int nlen = 0;
                  ULONG v;

                  if (tail[i] && tail[i] != ' ' && tail[i] != '\t')
                    {
                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }
                  else
                    {
                      while (tail[i] == ' ' || tail[i] == '\t')
                        {
                          i++;
                        }

                      v = parse_num (&tail[i], &nlen);
                      i += nlen;
                    }

                  if (nlen > 0)
                    {
                      opt_s = v;
                    }
                  else
                    {
                      puts ("Invalid -s value\r\n");

                      (void)bdos (0, 0);
                    }

                  break;
                }
              else
                {
                  opt_h = 1;
                }
            }
        }
      else
        {
          filename = &tail[i];

          break;
        }

      while (tail[i] == ' ' || tail[i] == '\t')
        {
          i++;
        }
    }

  if (opt_d)
    {
      opt_o = 1;
    }

  if (opt_h || !filename || !*filename)
    {
      puts ("Usage: HD [options] filename\r\n");
      puts ("  -a      collapse null lines with \"*\"\r\n");
      puts ("  -b N    bytes per line (1-64)\r\n");
      puts ("  -d      use decimal offsets (implies -o)\r\n");
      puts ("  -h      show this help text\r\n");
      puts ("  -i      ignore Last Record Byte Count\r\n");
      puts ("  -l N    stop dump after N bytes\r\n");
      puts ("  -n      disable ASCII display\r\n");
      puts ("  -o      prefix line with offset\r\n");
      puts ("  -p      pause after each screen\r\n");
      puts ("  -s N    seek N bytes before dumping\r\n");
      puts ("  -u      upper case hex characters\r\n");
      puts ("  -w      write output to FILENAME.HEX\r\n");

      (void)bdos (0, 0);
    }

  set_fcb (fcb, filename);

  fcb[12] = 0;
  fcb[14] = 0;
  fcb[32] = 0xFF; /* request LRBC on open */

  r = bdos (15, (LONG)(ULONG)fcb);

  if (r > 3)
    {
      puts ("File not found\r\n");

      (void)bdos (0, 0);
    }

  lrbc = fcb[32]; /* DOS-PLUS bytes used in last record (0 = full) */
  fcb[32] = 0;    /* sequential from record 0 */

  if (opt_i)
    {
      lrbc = 0;
    }

  if (opt_w)
    {
      for (i = 0; i < 36; i++)
        {
          out_fcb[i] = fcb[i];
        }

      out_fcb[9]  = 'H';
      out_fcb[10] = 'E';
      out_fcb[11] = 'X';
      out_fcb[12] = out_fcb[13] = out_fcb[14] = out_fcb[15] = 0;
      out_fcb[32] = 0;

      (void)bdos (19, (LONG)(ULONG)out_fcb);

      r = bdos (22, (LONG)(ULONG)out_fcb);

      if (r > 3)
        {
          opt_w = 0;
          puts ("Cannot create output file\r\n");
        }
    }

  /* Compute layout */
  if (opt_d)
    {
      hex_start = 12; /* 10-digit decimal + ": " */
    }
  else
    {
      hex_start = opt_o ? 10 : 0;
    }

  if (opt_b_set)
    {
      bytes_per_line = opt_b;
    }
  else
    {
      bytes_per_line = (80 - hex_start) / (opt_n ? 3 : 4);
    }

  ascii_start = hex_start + bytes_per_line * 3;

  col = 0;
  (void)bdos (26, (LONG)(ULONG)dma);

  /* -s, seek past N bytes */
  if (opt_s > 0)
    {
      ULONG to_skip = opt_s;

      /* skip full records */
      while (to_skip >= 128)
        {
          r = bdos (20, (LONG)(ULONG)fcb);

          if (r != 0)
            {
              puts ("Seek past end of file\r\n");
              (void)bdos (16, (LONG)(ULONG)fcb);

              (void)bdos (0, 0);
            }

          to_skip -= 128;
          file_offset += 128;
        }

      if (to_skip > 0)
        {
          r = bdos (20, (LONG)(ULONG)fcb);

          if (r != 0)
            {
              puts ("Seek past end of file\r\n");
              (void)bdos (16, (LONG)(ULONG)fcb);

              (void)bdos (0, 0);
            }

          file_offset += to_skip;

          {
            unsigned tail_len = 128 - (unsigned)to_skip;
            UBYTE partial[128];
            unsigned pi;

            for (pi = 0; pi < tail_len; pi++)
              {
                partial[pi] = dma[(unsigned)to_skip + pi];
              }

            for (pi = 0; pi < tail_len; pi++)
              {
                prev[pi] = partial[pi];
              }

            for (;;)
              {
                r = bdos (20, (LONG)(ULONG)fcb);

                if (r != 0)
                  {
                    break;
                  }

                if (have)
                  {
                    hexdump_feed (prev, 128);

                    if (pager_quit)
                      {
                        break;
                      }

                    if (opt_l && bytes_dumped >= opt_l)
                      {
                        break;
                      }
                  }
                else
                  {
                    hexdump_feed (partial, tail_len);

                    if (pager_quit)
                      {
                        break;
                      }

                    if (opt_l && bytes_dumped >= opt_l)
                      {
                        break;
                      }
                  }

                for (pi = 0; pi < 128; pi++)
                  {
                    prev[pi] = dma[pi];
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

                hexdump_feed (prev, n);
              }
            else
              {
                unsigned n = tail_len;

                if (lrbc != 0 && lrbc < (UBYTE)tail_len)
                  {
                    if (lrbc > (UBYTE)to_skip)
                      {
                        n = lrbc - (unsigned)to_skip;
                      }
                    else
                      {
                        n = 0;
                      }
                  }

                if (n > 0)
                  {
                    hexdump_feed (partial, n);
                  }
              }

            goto finish;
          }
        }
    }

  /* Normal read loop */
  for (;;)
    {
      r = bdos (20, (LONG)(ULONG)fcb);

      if (r != 0)
        {
          break;
        }

      if (have)
        {
          hexdump_feed (prev, 128);

          if (pager_quit)
            {
              break;
            }

          if (opt_l && bytes_dumped >= opt_l)
            {
              break;
            }
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

      hexdump_feed (prev, n);
    }

finish:
  hexdump_end ();
  out_flush_close ();

  (void)bdos (16, (LONG)(ULONG)fcb);

  (void)bdos (0, 0);
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
