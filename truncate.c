/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 8e3ba22a-8889-11f1-a6c5-80ee73e9b8e7
 */

/*****************************************************************************/

/* truncate.c - truncate file to specified byte count */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30

/*****************************************************************************/

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

static void
putu (unsigned long n)
{
  char buf[12];
  int i = 0;

  if (n == 0)
    {
      putch ('0');

      return;
    }

  while (n && i < 12)
    {
      buf[i++] = (char)('0' + (n % 10));
      n /= 10;
    }

  while (i > 0)
    {
      putch (buf[--i]);
    }
}

/*****************************************************************************/

static void
puthex (unsigned long n)
{
  char buf[12];
  int i = 0;
  static const char hex[] = "0123456789ABCDEF";

  if (n == 0)
    {
      puts ("0x0");

      return;
    }

  while (n && i < 8)
    {
      buf[i++] = hex[n & 0xf];
      n >>= 4;
    }

  puts ("0x");

  while (i > 0)
    {
      putch (buf[--i]);
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

static unsigned long
exact_size (unsigned long records, UBYTE lrbc)
{
  if (records == 0)
    {
      return 0;
    }

  if (lrbc == 0)
    {
      return records * 128UL;
    }

  return (records - 1) * 128UL + (unsigned long)lrbc;
}

/*****************************************************************************/

static void
set_ran (UBYTE *fcb, unsigned long rec)
{
  fcb[33] = (UBYTE)((rec >> 16) & 0xff);
  fcb[34] = (UBYTE)((rec >> 8) & 0xff);
  fcb[35] = (UBYTE)(rec & 0xff);
}

/*****************************************************************************/

static unsigned long
get_ran (const UBYTE *fcb)
{
  return ((unsigned long)fcb[33] << 16) | ((unsigned long)fcb[34] << 8)
         | (unsigned long)fcb[35];
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
str_eq_ci (const char *a, const char *b)
{
  while (*a && *b)
    {
      if (toupper_ch ((unsigned char)*a) != toupper_ch ((unsigned char)*b))
        {
          return 0;
        }

      a++;
      b++;
    }

  return *a == *b;
}

/*****************************************************************************/

static unsigned long
parse_num (const char *s, int *len)
{
  unsigned long v = 0;
  int n = 0;

  while (s[n] >= '0' && s[n] <= '9')
    {
      v = v * 10 + (unsigned long)(s[n] - '0');
      n++;
    }

  *len = n;

  return v;
}

/*****************************************************************************/

static int
parse_hex_byte (const char *s, UBYTE *out)
{
  int hi, lo;

  if (s[0] != '0' || (s[1] != 'x' && s[1] != 'X'))
    {
      return 0;
    }

  if (s[2] >= '0' && s[2] <= '9')
    {
      hi = s[2] - '0';
    }
  else if (s[2] >= 'a' && s[2] <= 'f')
    {
      hi = s[2] - 'a' + 10;
    }
  else if (s[2] >= 'A' && s[2] <= 'F')
    {
      hi = s[2] - 'A' + 10;
    }
  else
    {
      return 0;
    }

  if (s[3] >= '0' && s[3] <= '9')
    {
      lo = s[3] - '0';
    }
  else if (s[3] >= 'a' && s[3] <= 'f')
    {
      lo = s[3] - 'a' + 10;
    }
  else if (s[3] >= 'A' && s[3] <= 'F')
    {
      lo = s[3] - 'A' + 10;
    }
  else
    {
      return 0;
    }

  if (s[4] && s[4] != ' ' && s[4] != '\t' && s[4] != '\r')
    {
      return 0;
    }

  *out = (UBYTE)((hi << 4) | lo);

  return 1;
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: TRUNCATE [-h] [-f 0xNN] filename bytes\r\n");
  puts ("  -h       show this help text\r\n");
  puts ("  -f 0xNN  fill byte for data past LRBC");
  puts (" (default: 0x1A)\r\n");
  puts ("  bytes    byte count to truncate to, or the string \"LRBC\"\r\n");
  puts ("\r\n");
  puts ("If bytes is \"LRBC\", truncates to the");
  puts (" current Last Record Byte Count\r\n");
  puts ("with data after the LRBC in the final");
  puts (" record filled with the fill byte.\r\n");
}

/*****************************************************************************/

static void
print_fcb_name (const UBYTE *fcb)
{
  int i;

  for (i = 1; i <= 8 && fcb[i] != ' '; i++)
    {
      putch ((char)(fcb[i] & 0x7f));
    }

  if (fcb[9] != ' ')
    {
      putch ('.');

      for (i = 9; i <= 11 && fcb[i] != ' '; i++)
        {
          putch ((char)(fcb[i] & 0x7f));
        }
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb[36];
  UWORD r;
  UBYTE lrbc;
  unsigned long records, file_bytes;
  unsigned long trunc_bytes;
  unsigned long new_records;
  UBYTE new_lrbc;
  int opt_h = 0;
  int opt_f_set = 0;
  UBYTE fill_byte = 0x1A; /* default fill byte */
  int use_lrbc = 0; /* 1 = truncate at existing LRBC */
  char *filename = 0;
  char *length_str = 0;
  int i;
  char tail[128];
  int tlen;
  int nargs = 0; /* count of non-option arguments */

  tlen = CMD_TAIL[0];

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
              else if (c == 'f')
                {
                  /* Skip to fill byte value */
                  while (tail[i] == ' ' || tail[i] == '\t')
                    {
                      i++;
                    }

                  if (!tail[i])
                    {
                      puts ("ERROR: -f requires a");
                      puts (" value (e.g. -f 0x1A)\r\n");

                      bdos (0, 0);
                    }

                  {
                    const char *fval = &tail[i];
                    UBYTE parsed_byte;

                    if (!parse_hex_byte (fval, &parsed_byte))
                      {
                        puts ("ERROR: Invalid fill byte format\r\n");
                        puts ("Expected: -f 0xNN (e.g., \"-f 0x1A\")\r\n");

                        bdos (0, 0);
                      }

                    fill_byte = parsed_byte;
                    opt_f_set = 1;

                    while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
                      {
                        i++;
                      }
                  }

                  break;
                }
              else
                {
                  puts ("ERROR: Unknown option: -");
                  putch (c);
                  puts ("\r\n");
                  help ();

                  bdos (0, 0);
                }
            }
        }
      else
        {
          if (nargs == 0)
            {
              filename = &tail[i];
            }
          else if (nargs == 1)
            {
              length_str = &tail[i];
            }

          nargs++;

          while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
            {
              i++;
            }

          if (tail[i])
            {
              tail[i++] = 0;
            }
        }

      while (tail[i] == ' ' || tail[i] == '\t')
        {
          i++;
        }
    }

  if (opt_h || !filename || !length_str)
    {
      help ();

      bdos (0, 0);
    }

  if (str_eq_ci (length_str, "LRBC"))
    {
      use_lrbc = 1;
    }

  set_fcb (fcb, filename);
  fcb[12] = 0;    /* extent */
  fcb[14] = 0;    /* s2 */
  fcb[32] = 0xFF; /* request LRBC */

  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts ("ERROR: File not found: ");
      puts (filename);
      puts ("\r\n");

      bdos (0, 0);
    }

  lrbc = fcb[32];
  fcb[32] = 0; /* reset before I/O */

  bdos (35, (LONG)(unsigned long)fcb);
  records = get_ran (fcb);
  file_bytes = exact_size (records, lrbc);

  bdos (16, (LONG)(unsigned long)fcb);

  if (use_lrbc)
    {
      if (records == 0)
        {
          puts ("File is empty, nothing to truncate!\r\n");

          bdos (0, 0);
        }

      trunc_bytes = file_bytes;
    }
  else
    {
      int nlen = 0;
      trunc_bytes = parse_num (length_str, &nlen);

      if (nlen == 0)
        {
          puts ("ERROR: Invalid byte count: ");
          puts (length_str);
          puts ("\r\n");

          bdos (0, 0);
        }

      if (length_str[nlen] && length_str[nlen] != ' '
          && length_str[nlen] != '\t' && length_str[nlen] != '\r')
        {
          puts ("ERROR: Invalid byte count: ");
          puts (length_str);
          puts ("\r\n");

          bdos (0, 0);
        }

      if (trunc_bytes > file_bytes)
        {
          puts ("ERROR: Byte count ");
          putu (trunc_bytes);
          puts (" exceeds file size ");
          putu (file_bytes);
          puts ("\r\n");

          bdos (0, 0);
        }
    }

  if (trunc_bytes == 0)
    {
      new_records = 0;
      new_lrbc = 0;
    }
  else
    {
      new_lrbc = (UBYTE)(trunc_bytes % 128);

      if (new_lrbc == 0)
        {
          new_records = trunc_bytes / 128;
          new_lrbc = 0;
        }
      else
        {
          new_records = (trunc_bytes / 128) + 1;
        }
    }

  if (new_records < records)
    {
      set_fcb (fcb, filename);
      set_ran (fcb, new_records);
      r = bdos (99, (LONG)(unsigned long)fcb);

      if (r > 3 && r != 0)
        {
          puts ("ERROR: Truncation failed (BDOS 99");
          puts (" returned ");
          putu ((unsigned long)r);
          puts (")\r\n");

          bdos (0, 0);
        }
    }

  if (new_records > 0 && new_lrbc != 0)
    {
      UBYTE dma[128];

      set_fcb (fcb, filename);
      fcb[12] = 0;
      fcb[14] = 0;
      fcb[32] = 0;

      r = bdos (15, (LONG)(unsigned long)fcb);

      if (r > 3)
        {
          puts ("ERROR: Cannot reopen file\r\n");

          bdos (0, 0);
        }

      bdos (26, (LONG)(unsigned long)dma);

      set_ran (fcb, new_records - 1);
      r = bdos (33, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          for (i = 0; i < 128; i++)
            {
              dma[i] = fill_byte;
            }
        }

      for (i = (int)new_lrbc; i < 128; i++)
        {
          dma[i] = fill_byte;
        }

      set_ran (fcb, new_records - 1);
      r = bdos (34, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          puts ("ERROR: Write failed (BDOS 34 returned ");
          putu ((unsigned long)r);
          puts (")\r\n");
          bdos (16, (LONG)(unsigned long)fcb);

          bdos (0, 0);
        }

      bdos (16, (LONG)(unsigned long)fcb);
    }
  else if (new_records > 0 && new_lrbc == 0)
    {
      ;
    }

  set_fcb (fcb, filename);
  fcb[6] |= 0x80; /* signal: set LRBC */
  fcb[32] = (UBYTE)new_lrbc; /* new LRBC value */
  r = bdos (30, (LONG)(unsigned long)fcb);

  if (r == 255)
    {
      puts ("ERROR: Failed to set LRBC\r\n");

      bdos (0, 0);
    }

  puts ("Truncated ");
  print_fcb_name (fcb);
  puts (" to ");
  putu (trunc_bytes);
  puts (" bytes (");
  putu (new_records);
  puts (" records, LRBC=");
  putu ((unsigned long)new_lrbc);
  puts (")");

  if (opt_f_set)
    {
      puts (" fill=");
      puthex ((unsigned long)fill_byte);
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
