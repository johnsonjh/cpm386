/*
 * CP/M-386 - dumpfcb.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 979dae60-82b4-11f1-a625-80ee73e9b8e7
 */

/*****************************************************************************/

/* dumpfcb.c: pretty-print a File Control Block */

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
  (void)bdos (2, (LONG)(unsigned char)c);
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
puthex2 (unsigned v)
{
  static const char h [] = "0123456789ABCDEF";

  putch (h [(v >> 4) & 0xF]);
  putch (h [v & 0xF]);
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

static char
toupper_ch (unsigned char c)
{
  if (c >= 'a' && c <= 'z')
    {
      return (char)(c - 32);
    }

  return (char)c;
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: DUMPFCB [filename]\r\n");
  puts ("  no args   dump default FCB at TPA+0x5C\r\n");
  puts ("  filename  open file (FCB+32=0xFF for LRBC), dump FCB\r\n");
}

/*****************************************************************************/

/*
 * full != 0: 36 bytes
 * full == 0: 16 bytes only
 */

static void
dump_fcb (const UBYTE *f, const char *title, int full)
{
  int i;
  unsigned char c;

  puts ("\r\n");

  if (title)
    {
      puts (title);
      puts ("\r\n");
    }

  puts ("Offset  Field     Hex              Decoded\r\n");
  puts ("------  --------  ---------------  ----------------------\r\n");

  /* +0 drive */
  puts ("+00     drvcode   ");
  puthex2 (f [0]);
  puts ("               ");

  if (f [0] == 0)
    {
      puts ("default drive");
    }
  else if (f [0] >= 1 && f [0] <= 16)
    {
      puts ("drive ");
      putch ((char)('A' + f [0] - 1));
      puts (":");
    }
  else
    {
      puts ("(invalid)");
    }

  puts ("\r\n");

  /* +1..8 name */
  puts ("+01     fname     ");

  for (i = 1; i <= 8; i++)
    {
      puthex2 (f [i]);
      putch (' ');
    }

  puts (" ");

  for (i = 1; i <= 8; i++)
    {
      c = (unsigned char)(f [i] & 0x7f);
      putch ((c >= 32 && c < 127) ? (char)c : '.');
    }

  if (f [1] & 0x80)
    {
      puts ("  f1'");
    }

  if (f [2] & 0x80)
    {
      puts (" f2'");
    }

  if (f [3] & 0x80)
    {
      puts (" f3'");
    }

  if (f [4] & 0x80)
    {
      puts (" f4'");
    }

  puts ("\r\n");

  /* +9..11 type + attributes */
  puts ("+09     ftype     ");

  for (i = 9; i <= 11; i++)
    {
      puthex2 (f [i]);
      putch (' ');
    }

  puts ("          ");

  for (i = 9; i <= 11; i++)
    {
      c = (unsigned char)(f [i] & 0x7f);
      putch ((c >= 32 && c < 127) ? (char)c : '.');
    }

  if (f [9] & 0x80)
    {
      puts ("  R/O");
    }

  if (f [10] & 0x80)
    {
      puts (" SYS");
    }

  if (f [11] & 0x80)
    {
      puts (" ARC");
    }

  puts ("\r\n");

  puts ("+0C     extent    ");
  puthex2 (f [12]);
  puts ("               ");
  putu (f [12] & 0x1f);
  puts (" (low 5 bits)\r\n");

  puts ("+0D     s1/LRBC   ");
  puthex2 (f [13]);
  puts ("               ");
  putu (f [13]);

  if (f [13] == 0)
    {
      puts (" (full last rec / empty)");
    }
  else
    {
      puts (" bytes used in last rec");
    }

  puts ("\r\n");

  puts ("+0E     s2        ");
  puthex2 (f [14]);
  puts ("               module=");
  putu (f [14] & 0x3f);

  if (f [14] & 0x80)
    {
      puts ("  write-flag");
    }

  puts ("\r\n");

  puts ("+0F     rcdcnt    ");
  puthex2 (f [15]);
  puts ("               ");
  putu (f [15]);
  puts (" records in extent\r\n");

  if (!full)
    {
      puts ("(Base-page FCB1 ends here; +10.. at 0x6C is FCB2.)\r\n");
    }
  else
    {
      ULONG ran;

      /* +16..31 allocation map */
      puts ("+10     dskmap    ");

      for (i = 0; i < 16; i++)
        {
          puthex2 (f [16 + i]);

          if (i == 7)
            {
              puts ("\r\n                    ");
            }
          else if (i < 15)
            {
              putch (' ');
            }
        }

      puts ("\r\n");
      puts ("        as words  ");

      for (i = 0; i < 8; i++)
        {
          UWORD w = (UWORD)f [16 + i * 2] | ((UWORD)f [17 + i * 2] << 8);
          puthex2 ((w >> 8) & 0xFF);
          puthex2 (w & 0xFF);

          if (i < 7)
            {
              putch (' ');
            }
        }

      puts (" (LE)\r\n");

      puts ("+20     cur_rec   ");
      puthex2 (f [32]);
      puts ("               ");

      if (f [32] == 0xFF)
        {
          puts ("0xFF (LRBC request/result)");
        }
      else
        {
          putu (f [32]);
          puts (" (current record 0..127)");
        }

      puts ("\r\n");

      ran = (ULONG)f [33] | ((ULONG)f [34] << 8) | ((ULONG)f [35] << 16);
      puts ("+21     ran0..2   ");
      puthex2 (f [33]);
      putch (' ');
      puthex2 (f [34]);
      putch (' ');
      puthex2 (f [35]);
      puts ("          random=");
      putu ((unsigned)ran);
      puts ("\r\n");
    }

  /* human name line */
  puts ("\r\nAs filespec: ");

  if (f [0] >= 1 && f [0] <= 16)
    {
      putch ((char)('A' + f [0] - 1));
      putch (':');
    }

  for (i = 1; i <= 8; i++)
    {
      c = (unsigned char)(f [i] & 0x7f);

      if (c != ' ')
        {
          putch ((c >= 32 && c < 127) ? (char)c : '?');
        }
    }

  if ((f  [9] & 0x7f) != ' '
   || (f [10] & 0x7f) != ' '
   || (f [11] & 0x7f) != ' ')
    {
      putch ('.');

      for (i = 9; i <= 11; i++)
        {
          c = (unsigned char)(f [i] & 0x7f);

          if (c != ' ')
            {
              putch ((c >= 32 && c < 127) ? (char)c : '?');
            }
        }
    }

  puts ("\r\n");
}

/*****************************************************************************/

static int
parse_help (void)
{
  unsigned tlen = CMD_TAIL [0], i;
  char tail [128];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      tail [i] = (char)CMD_TAIL [1 + i];
    }

  tail [tlen] = 0;
  i = 0;

  while (tail [i] == ' ' || tail [i] == '\t')
    {
      i++;
    }
  if (tail [i] == '-' || tail [i] == '/')
    {
      i++;

      if (toupper_ch ((unsigned char)tail [i]) == 'H')
        {
          return 1;
        }
    }

  return 0;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb [36];
  int i;
  UWORD r;

  if (parse_help ())
    {
      help ();

      (void)bdos (0, 0);
    }

  /*
   * No name in default FCB
   * dump base-page FCB1 (and FCB2 prefix)
   */

  if (DEF_FCB [1] == ' ' || DEF_FCB [1] == 0)
    {
      dump_fcb (DEF_FCB, "FCB1 at TPA+0x5C (first 16 bytes)", 0);
      dump_fcb (DEF_FCB + 0x10, "FCB2 at TPA+0x6C (first 16 bytes)", 0);

      (void)bdos (0, 0);
    }

  for (i = 0; i < 36; i++)
    {
      fcb [i] = DEF_FCB [i];
    }

  fcb [12] = 0;
  fcb [14] = 0;
  fcb [32] = 0xFF; /* request LRBC on open */

  r = bdos (15, (LONG)(ULONG)fcb);

  if (r > 3)
    {
      puts ("File not found (dumping FCB as passed)\r\n");
      dump_fcb (fcb, "FCB before open", 1);

      (void)bdos (0, 0);
    }

  puts ("Open OK (dir index ");
  putu (r);
  puts (")\r\n");
  dump_fcb (fcb, "FCB after open (cur_rec may hold LRBC if was 0xFF)", 1);

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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
