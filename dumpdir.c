/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 929667b8-82b4-11f1-8510-80ee73e9b8e7
 */

/*****************************************************************************/

/* dumpdir.c - pretty-print CP/M directory entry */

/*****************************************************************************/

/*
 * Usage:
 *   DUMPDIR              list all non-empty dirents (user 0..15)
 *   DUMPDIR filespec     search pattern (wildcards * ? ok)
 *   DUMPDIR -a filespec  include all users (still skips 0xE5 empty)
 *   DUMPDIR -h           help
 */

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
  puts ("Usage: DUMPDIR [-a] [filespec]\r\n");
  puts ("  no args    all files, current user\r\n");
  puts ("  filespec   name pattern (* and ? wildcards)\r\n");
  puts ("  -a         show all users 0..15 (not only current)\r\n");
}

/*****************************************************************************/

/* Build FCB name/type from pattern string; * -> ? */
static void
pattern_to_fcb (UBYTE *fcb, const char *pat)
{
  int i, ni = 0, ti = 0, in_type = 0;
  char c;

  for (i = 0; i < 36; i++)
    {
      fcb [i] = 0;
    }

  for (i = 1; i <= 11; i++)
    {
      fcb [i] = ' ';
    }

  fcb [0] = 0; /* default drive */

  if (pat [0] && pat [1] == ':')
    {
      c = toupper_ch ((unsigned char)pat [0]);

      if (c >= 'A' && c <= 'P')
        {
          fcb [0] = (UBYTE)(c - 'A' + 1);
        }

      pat += 2;
    }

  while (*pat && *pat != ' ' && *pat != '\t' && *pat != '\r')
    {
      c = *pat++;

      if (c == '.')
        {
          in_type = 1;

          continue;
        }

      if (c == '*')
        {
          if (!in_type)
            {
              while (ni < 8)
                {
                  fcb [1 + ni++] = '?';
                }
            }
          else
            {
              while (ti < 3)
                {
                  fcb [9 + ti++] = '?';
                }
            }

          continue;
        }

      if (c == '?')
        {
          if (!in_type && ni < 8)
            {
              fcb [1 + ni++] = '?';
            }
          else if (in_type && ti < 3)
            {
              fcb [9 + ti++] = '?';
            }

          continue;
        }

      c = toupper_ch ((unsigned char)c);

      if (!in_type && ni < 8)
        {
          fcb [1 + ni++] = (UBYTE)c;
        }
      else if (in_type && ti < 3)
        {
          fcb [9 + ti++] = (UBYTE)c;
        }
    }

  /* bare name, no type -> any type */
  if (!in_type && ti == 0)
    {
      fcb [9] = fcb [10] = fcb [11] = '?';
    }
}

/*****************************************************************************/

static void
put_name11 (const UBYTE *p)
{
  int i;
  unsigned char c;

  for (i = 0; i < 8; i++)
    {
      c = (unsigned char)(p [i] & 0x7f);
      putch ((c >= 32 && c < 127) ? (char)c : '.');
    }

  putch ('.');

  for (i = 8; i < 11; i++)
    {
      c = (unsigned char)(p [i] & 0x7f);
      putch ((c >= 32 && c < 127) ? (char)c : '.');
    }
}

/*****************************************************************************/

static void
dump_dirent (const UBYTE *de, unsigned dir_index)
{
  int i;

  puts ("---- dirent @ search index ");
  putu (dir_index);
  puts (" ----\r\n");

  puts ("user/ex  ");
  puthex2 (de [0]);
  puts ("  ");

  if (de [0] == 0xE5)
    {
      puts ("(empty/deleted)");
    }
  else if (de [0] < 16)
    {
      puts ("user ");
      putu (de [0]);
    }
  else
    {
      puts ("(label/special)");
    }

  puts ("\r\n");

  puts ("name     ");
  put_name11 (de + 1);

  if (de [9] & 0x80)
    {
      puts ("  R/O");
    }

  if (de [10] & 0x80)
    {
      puts (" SYS");
    }

  if (de [11] & 0x80)
    {
      puts (" ARC");
    }

  puts ("\r\n");

  puts ("extent   ");
  puthex2 (de [12]);
  puts ("  (");
  putu (de [12] & 0x1f);
  puts (")\r\n");

  puts ("s1/LRBC  ");
  puthex2 (de [13]);
  puts ("  (");
  putu (de [13]);
  puts (")\r\n");

  puts ("s2       ");
  puthex2 (de [14]);
  puts ("  module=");
  putu (de [14] & 0x3f);

  if (de [14] & 0x80)
    {
      puts (" write");
    }

  puts ("\r\n");

  puts ("rc       ");
  puthex2 (de [15]);
  puts ("  (");
  putu (de [15]);
  puts (" records)\r\n");

  puts ("map bytes");

  for (i = 0; i < 16; i++)
    {
      if (i == 8)
        {
          puts ("\r\n         ");
        }

      putch (' ');
      puthex2 (de [16 + i]);
    }

  puts ("\r\n");

  puts ("map words");

  for (i = 0; i < 8; i++)
    {
      UWORD w = (UWORD)de [16 + i * 2] | ((UWORD)de [17 + i * 2] << 8);

      putch (' ');
      puthex2 ((w >> 8) & 0xFF);
      puthex2 (w & 0xFF);
    }

  puts (" (LE)\r\n\r\n");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb [36];
  UBYTE dma [128];
  char tail [128];
  unsigned tlen, i;
  int flag_all = 0;
  const char *pat = 0;
  UWORD r;
  int user, count = 0;

  tlen = CMD_TAIL [0];

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
  while (tail [i])
    {
      if (tail [i] == '-' || tail [i] == '/')
        {
          i++;

          while (tail [i] && tail [i] != ' ' && tail [i] != '\t')
            {
              char c = toupper_ch ((unsigned char)tail [i++]);

              if (c == 'H')
                {
                  help ();

                  (void)bdos (0, 0);
                }

              if (c == 'A')
                {
                  flag_all = 1;
                }
            }
          while (tail [i] == ' ' || tail [i] == '\t')
            {
              i++;
            }

          continue;
        }

      pat = &tail [i];

      break;
    }

  if (pat && *pat && *pat != '\r')
    {
      pattern_to_fcb (fcb, pat);
    }
  else if (DEF_FCB [1] != ' ' && DEF_FCB [1] != '?' && DEF_FCB [1] != 0)
    {
      for (i = 0; i < 36; i++)
        {
          fcb [i] = DEF_FCB [i];
        }

      if (fcb [9] == ' ' && fcb [10] == ' ' && fcb [11] == ' ')
        {
          fcb [9] = fcb [10] = fcb [11] = '?';
        }
    }
  else
    {
      pattern_to_fcb (fcb, "*.*");
    }

  user = (int)bdos (32, 0xFF);
  (void)bdos (26, (LONG)(ULONG)dma);

  if (fcb [0])
    {
      (void)bdos (14, (LONG)(fcb [0] - 1));
    }

  fcb [12] = '?';
  fcb [14] = '?';

  puts ("DUMPDIR");

  if (flag_all)
    {
      puts (" (all users)");
    }
  else
    {
      puts (" (user ");
      putu ((unsigned)user);
      puts (")");
    }

  puts ("\r\n\r\n");

  r = bdos (17, (LONG)(ULONG)fcb);

  while (r != 255)
    {
      const UBYTE *de = dma + (r * 32);

      if (de [0] != 0xE5 && (flag_all || de [0] == (UBYTE)user))
        {
          dump_dirent (de, (unsigned)r);
          count++;
        }

      r = bdos (18, 0);
    }

  puts ("Total entries shown: ");
  putu ((unsigned)count);
  puts ("\r\n");

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
