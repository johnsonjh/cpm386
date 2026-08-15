/*
 * CP/M-386 - capslock.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 5da7a342-918b-11f1-9764-246e96298730
 */

/*****************************************************************************/

/* capslock.c - choose what the Caps-Lock key does (BDOS 235) */

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
#define BDOS_KBD_CAPS 235

/*****************************************************************************/

/* Must match the KBD_CAPS_* values in kbd.h. */

#define CAPS_ON 0
#define CAPS_OFF 1
#define CAPS_CTRL 2
#define CAPS_SWAP 3

#define CAPS_QUERY 0xFFFF
#define CAPS_BAD 0xFFFF

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
  puts ("Usage: CAPSLOCK [on | off | control | swap]\r\n");
  puts ("  (no args)  query the current Caps-Lock key configuration\r\n");
  puts ("  on         Caps-Lock locks the shift; left-Ctrl is left-Ctrl\r\n");
  puts ("  off        Caps-Lock is disabled\r\n");
  puts ("  control    Caps-Lock acts as a second left-Ctrl\r\n");
  puts ("  swap       Caps-Lock and left-Ctrl exchange functions\r\n");
}

/*****************************************************************************/

static void
report (UWORD mode)
{
  switch (mode)
    {
    case CAPS_ON:
      puts ("Caps-Lock is enabled; left-Ctrl is left-Ctrl.\r\n");

      break;

    case CAPS_OFF:
      puts ("Caps-Lock is disabled.\r\n");

      break;

    case CAPS_CTRL:
      puts ("Caps-Lock acts as a second left-Ctrl.\r\n");

      break;

    case CAPS_SWAP:
      puts ("Caps-Lock and left-Ctrl are swapped.\r\n");

      break;

    default:
      puts ("Caps-Lock control is not available.\r\n");

      break;
    }
}

/*****************************************************************************/

/*
 * The word the user typed.  "enable" and "disable" are accepted alongside
 * on and off, as are 1/0 and true/false.  Returns a CAPS_* value or -1.
 */

static int
parse_mode (const char *s)
{
  if (streq_ci (s, "on") || streq_ci (s, "enable") || streq_ci (s, "enabled")
      || streq_ci (s, "1") || streq_ci (s, "true") || streq_ci (s, "yes")
      || streq_ci (s, "normal"))
    {
      return CAPS_ON;
    }

  if (streq_ci (s, "off") || streq_ci (s, "disable")
      || streq_ci (s, "disabled") || streq_ci (s, "0")
      || streq_ci (s, "false") || streq_ci (s, "no")
      || streq_ci (s, "none"))
    {
      return CAPS_OFF;
    }

  if (streq_ci (s, "control") || streq_ci (s, "ctrl") || streq_ci (s, "ctl"))
    {
      return CAPS_CTRL;
    }

  if (streq_ci (s, "swap") || streq_ci (s, "swapped")
      || streq_ci (s, "exchange"))
    {
      return CAPS_SWAP;
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

  /* First word of the tail; anything after it is a usage error. */
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
      report (bdos (BDOS_KBD_CAPS, (LONG)(ULONG)CAPS_QUERY));
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

  r = bdos (BDOS_KBD_CAPS, (LONG)(ULONG)(unsigned)mode);

  if (r == CAPS_BAD)
    {
      puts ("Caps-Lock control is not available.\r\n");
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
