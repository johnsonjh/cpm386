/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 356a13d6-970f-11f1-96a9-80ee73e9b8e7
 */

/*****************************************************************************/

/* esctilde.c - control ESC / tilde key mapping (BDOS 237) */

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_KBD_ESC 237

/*****************************************************************************/

/* Must match the KBD_ESC_* values in kbd.h. */

#define ESC_NORMAL 0
#define ESC_SWAP 1

#define ESC_QUERY 0xFFFF
#define ESC_BAD 0xFFFF

/*****************************************************************************/

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

static int
streq_ci (const char *a, const char *b)
{
  while (*a && *b)
    {
      char x = *a++;
      char y = *b++;

      if (x >= 'A' && x <= 'Z')
        {
          x = (char)(x + 32);
        }

      if (y >= 'A' && y <= 'Z')
        {
          y = (char)(y + 32);
        }

      if (x != y)
        {
          return 0;
        }
    }

  return *a == 0 && *b == 0;
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: ESCTILDE [-h] [default | swap]\r\n");
  puts ("  (no args)  query the current ESC and Tilde key configuration\r\n");
  puts ("  default    ESC key and Tidle key function normally\r\n");
  puts ("  swap       ESC key and Tilde key exchange functions\r\n");
}

/*****************************************************************************/

static void
report (UWORD mode)
{
  switch (mode)
    {
    case ESC_NORMAL:
      puts ("ESC and Tilde keys function normally.\r\n");

      break;

    case ESC_SWAP:
      puts ("ESC and Tilde keys are swapped.\r\n");

      break;

    default:
      puts ("ESC and Tilde key control is not available.\r\n");

      break;
    }
}

/*****************************************************************************/

/*
 * The word the user typed.  Returns a ESC_* value or -1.
 */

static int
parse_mode (const char *s)
{
  if (streq_ci (s, "on") || streq_ci (s, "off")
   || streq_ci (s, "default") || streq_ci (s, "normal")
   || streq_ci (s, "enable") || streq_ci (s, "disable"))
    {
      return ESC_NORMAL;
    }

  if (streq_ci (s, "swap") || streq_ci (s, "swapped")
   || streq_ci (s, "exchange"))
    {
      return ESC_SWAP;
    }

  return -1;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char tail [128];
  char word [32];
  unsigned tlen, i, n;
  int mode;
  UWORD r;

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

  n = 0;

  while (tail [i] && tail [i] != ' ' && tail [i] != '\t'
         && n < sizeof word - 1)
    {
      word [n++] = tail [i++];
    }

  word [n] = 0;

  while (tail [i] == ' ' || tail [i] == '\t')
    {
      i++;
    }

  puts ("\r\n");

  if (tail [i])
    {
      usage ();
      (void)bdos (0, 0);

      return;
    }

  if (n == 0)
    {
      report (bdos (BDOS_KBD_ESC, (LONG)(ULONG)ESC_QUERY));
      (void)bdos (0, 0);

      return;
    }

  if (word [0] == '-' || word [0] == '/' || word [0] == '?')
    {
      usage ();
      (void)bdos (0, 0);

      return;
    }

  mode = parse_mode (word);

  if (mode < 0)
    {
      puts ("Unknown setting: ");
      puts (word);
      puts ("\r\n\r\n");
      usage ();
      (void)bdos (0, 0);

      return;
    }

  r = bdos (BDOS_KBD_ESC, (LONG)(ULONG)(unsigned)mode);

  if (r == ESC_BAD)
    {
      puts ("ESC and Tilde key control is not available.\r\n");
    }
  else
    {
      report (r);
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
