/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 8a9504ec-82b5-11f1-9a9d-80ee73e9b8e7
 */

/*****************************************************************************/

/* printenv.c - display environment */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
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
                      "d"((unsigned long)info)
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
putu (unsigned n)
{
  char b[8];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 8)
    {
      b[i++] = (char)('0' + n % 10);
      n /= 10;
    }
  while (i)
    {
      putch (b[--i]);
    }
}

/*****************************************************************************/

static void
puthex4 (UWORD v)
{
  static const char hx[] = "0123456789ABCDEF";

  putch (hx[(v >> 12) & 0xf]);
  putch (hx[(v >> 8) & 0xf]);
  putch (hx[(v >> 4) & 0xf]);
  putch (hx[v & 0xf]);
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
isalnum_ (int c)
{
  return (c >= '0' && c <= '9')
      || (c >= 'A' && c <= 'Z')
      || (c >= 'a' && c <= 'z');
}

/*****************************************************************************/

static int
isspace_ (int c)
{
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: PRINTENV [-h]\r\n");
  puts ("  Display ENV.DAT variables and system info\r\n");
}

/*****************************************************************************/

/* Parse command tail for -h */
static int
want_help (void)
{
  unsigned tlen = CMD_TAIL[0], i = 0;
  char t[128];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      t[i] = (char)CMD_TAIL[1 + i];
    }

  t[tlen] = 0;
  i = 0;

  while (t[i] == ' ' || t[i] == '\t')
    {
      i++;
    }

  if (t[i] == '-' || t[i] == '/')
    {
      i++;

      while (t[i] && t[i] != ' ')
        {
          if (toupper_ch ((unsigned char)t[i]) == 'H')
            {
              return 1;
            }

          i++;
        }
    }

  return 0;
}

/*****************************************************************************/

static void
fill_env_fcb (UBYTE *fcb)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  /* ENV.DAT */
  fcb[1] = 'E'; fcb[2] = 'N'; fcb[3] = 'V';

  for (i = 4; i <= 8; i++)
    {
      fcb[i] = ' ';
    }

  fcb[9] = 'D'; fcb[10] = 'A'; fcb[11] = 'T';
}

/*****************************************************************************/

/*
 * Read ENV.DAT; emit each completed VAR=val line
 * (skip comments/errors)
 */

static void
dump_env_file (void)
{
  UBYTE fcb[36];
  UBYTE dma[128]; /*cppcheck-suppress unassignedVariable*/
  UBYTE lrbc = 0;
  UBYTE prev[128];
  int have = 0;
  UWORD r;
  char var[17], val[129];
  int vi = 0, ai = 0;
  /* 0=skip, 1=var, 2=pre=, 3=val, 4=comment */
  int st = 0;
  unsigned pos, rec_len;
  int final = 0;

  fill_env_fcb (fcb);
  fcb[32] = 0xFF;
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts ("(no ENV.DAT)\r\n");
      return;
    }

  lrbc = fcb[32];
  fcb[32] = 0;
  (void)bdos (26, (LONG)(unsigned long)dma);

  for (;;)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);
      if (r != 0)
        {
          final = 1;

          if (!have)
            {
              break;
            }

          rec_len = (lrbc != 0) ? lrbc : 128;
        }
      else if (!have)
        {
          int j;

          for (j = 0; j < 128; j++)
            {
              prev[j] = dma[j];
            }

          have = 1;

          continue;
        }
      else
        {
          rec_len = 128;
        }

      for (pos = 0; pos < rec_len; pos++)
        {
          int c = prev[pos] & 0xff;

          if (c == 26)
            {
              goto done_file;
            }

          if (st == 4)
            { /* comment to EOL */
              if (c == '\n' || c == '\r')
                {
                  st = 0;
                }

              continue;
            }

          if (c == '\n' || c == '\r')
            {
              if (st == 3 || st == 1 || st == 2)
                {
                  var[vi] = 0;
                  val[ai] = 0;

                  /* trim trailing spaces in val */
                  while (ai > 0 && isspace_ ((unsigned char)val[ai - 1]))
                    {
                      val[--ai] = 0;
                    }

                  if (vi > 0 && st == 3)
                    {
                      puts (var);
                      putch ('=');
                      puts (val);
                      puts ("\r\n");
                    }
                }

              vi = ai = 0;
              st = 0;

              continue;
            }

          if (st == 0)
            {
              if (c == ';')
                {
                  st = 4;

                  continue;
                }

              if (isspace_ (c))
                {
                  continue;
                }

              if (isalnum_ (c) || c == '_')
                {
                  vi = 0;
                  var[vi++] = (char)toupper_ch (c);
                  st = 1;
                }

              continue;
            }

          if (st == 1)
            {
              if (c == '=')
                {
                  var[vi] = 0;
                  ai = 0;
                  st = 3;

                  continue;
                }

              if (isspace_ (c))
                {
                  var[vi] = 0;
                  st = 2;

                  continue;
                }

              if ((isalnum_ (c) || c == '_') && vi < 16)
                {
                  var[vi++] = (char)toupper_ch (c);
                }

              continue;
            }

          if (st == 2)
            {
              if (isspace_ (c))
                {
                  continue;
                }

              if (c == '=')
                {
                  ai = 0;
                  st = 3;

                  continue;
                }

              st = 0; /* error line */

              continue;
            }

          if (st == 3)
            {
              if (ai == 0 && isspace_ (c))
                {
                  continue; /* post-= spaces */
                }

              if (ai < 128)
                {
                  val[ai++] = (char)c;
                }
            }
        }

      if (final)
        {
          break;
        }

      {
        int j;

        for (j = 0; j < 128; j++)
          {
            prev[j] = dma[j];
          }
      }
    }

done_file:
  /* trailing line without CR */
  if (st == 3 && vi > 0)
    {
      var[vi] = 0;
      val[ai] = 0;

      while (ai > 0 && isspace_ ((unsigned char)val[ai - 1]))
        {
          val[--ai] = 0;
        }

      puts (var);
      putch ('=');
      puts (val);
      puts ("\r\n");
    }

  (void)bdos (16, (LONG)(unsigned long)fcb);
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UWORD ver;
  int drive, user;

  if (want_help ())
    {
      help ();

      (void)bdos (0, 0);
    }

  /* System "environment" (always available) */
  ver = bdos (12, 0);
  drive = (int)bdos (25, 0);
  user = (int)bdos (32, 0xFF);

  puts ("OS=CP/M-386\r\n");

  puts ("BDOS=");
  puthex4 (ver);
  puts ("\r\n");

  puts ("DRIVE=");
  putch ((char)('A' + drive));
  puts (":\r\n");

  puts ("USER=");
  putu ((unsigned)user);
  puts ("\r\n");

  /* File-based env (ENV.DAT) */
  dump_env_file ();

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
