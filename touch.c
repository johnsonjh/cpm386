/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: e48c929e-82b5-11f1-a4ac-80ee73e9b8e7
 */

/*****************************************************************************/

/* touch.c: create empty file if missing */

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
set_fcb (UBYTE *fcb, const char *name)
{
  int i, j = 1;

  for (i = 0; i < 36; i++)
    {
      fcb [i] = 0;
    }

  for (i = 1; i <= 11; i++)
    {
      fcb [i] = ' ';
    }

  if (name [0] && name [1] == ':')
    {
      char c = name [0];

      if (c >= 'a' && c <= 'z')
        {
          c -= 32;
        }

      fcb [0] = (UBYTE)(c - 'A' + 1);
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
          fcb [j++] = (UBYTE)c;
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
              fcb [j++] = (UBYTE)c;
            }
        }
    }
}

/*****************************************************************************/

static void
touch_one (const char *name)
{
  UBYTE fcb [36];
  UWORD r;

  set_fcb (fcb, name);

  fcb [12] = 0; /* extent */
  fcb [14] = 0; /* s2 */
  fcb [32] = 0;

  r = bdos (15, (LONG)(ULONG)fcb);

  if (r <= 3)
    {
      (void)bdos (16, (LONG)(ULONG)fcb); /* close */

      return;
    }

  set_fcb (fcb, name);

  fcb [12] = 0;
  fcb [13] = 0; /* s1 */
  fcb [14] = 0;
  fcb [15] = 0; /* rc */
  fcb [32] = 0;

  r = bdos (22, (LONG)(ULONG)fcb);

  if (r <= 3)
    {
      (void)bdos (16, (LONG)(ULONG)fcb);
    }
  else
    {
      puts ("Cannot create ");
      puts (name);
      puts ("\r\n");
    }
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: TOUCH [-h] filename [filename ...]\r\n");
  puts ("  -h  help\r\n");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char tail [128];
  int tlen, i;
  int opt_h = 0;
  int nfiles = 0;

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
              char c = tail [i++];

              if (c >= 'A' && c <= 'Z')
                {
                  c += 32;
                }

              if (c == 'h')
                {
                  opt_h = 1;
                }
              else
                {
                  puts ("ERROR: Invalid option '");
                  putch (c);
                  puts ("'\r\n");
                  usage ();

                  (void)bdos (0, 0);
                }
            }
        }
      else
        {
          const char *fn = &tail [i];

          while (tail [i] && tail [i] != ' ' && tail [i] != '\t')
            {
              i++;
            }

          if (tail [i])
            {
              tail [i++] = 0;
            }

          if (!opt_h)
            {
              touch_one (fn);
              nfiles++;
            }

          while (tail [i] == ' ' || tail [i] == '\t')
            {
              i++;
            }

          continue;
        }

      while (tail [i] == ' ' || tail [i] == '\t')
        {
          i++;
        }
    }

  if (opt_h || nfiles == 0)
    {
      if (nfiles == 0)
        {
          if (DEF_FCB [1] != ' ' && DEF_FCB [1] != 0)
            {
              UBYTE fcb [36];
              UWORD r;

              for (i = 0; i < 36; i++)
                {
                  fcb [i] = DEF_FCB [i];
                }

              fcb [12] = 0;
              fcb [14] = 0;
              fcb [32] = 0;

              r = bdos (15, (LONG)(ULONG)fcb);

              if (r <= 3)
                {
                  (void)bdos (16, (LONG)(ULONG)fcb);

                  (void)bdos (0, 0);
                }

              for (i = 0; i < 36; i++)
                {
                  fcb [i] = DEF_FCB [i];
                }

              fcb [12] = 0;
              fcb [13] = 0;
              fcb [14] = 0;
              fcb [15] = 0;
              fcb [32] = 0;

              r = bdos (22, (LONG)(ULONG)fcb);

              if (r <= 3)
                {
                  (void)bdos (16, (LONG)(ULONG)fcb);
                }

              (void)bdos (0, 0);
            }
        }

      usage ();
    }

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
