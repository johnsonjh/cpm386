/*
 * CP/M-386 - numlock.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: c5bab3a0-91a6-11f1-986c-80ee73e9b8e7
 */

/*****************************************************************************/

/* numlock.c - Num-Lock state and LED control (BDOS 236) */

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
#define BDOS_KBD_LOCK 236

/*****************************************************************************/

/* Must match struct cpm_kbdlock and the KBDL_* values in bdosdef.h */

#define KBDL_SET_LOCKS 0x0001
#define KBDL_SET_LEDS 0x0002

#define KBDL_SCROLL 0x0001
#define KBDL_NUM 0x0002
#define KBDL_CAPS 0x0004

struct kbdlock
{
  UWORD flags;
  UWORD locks;
  UWORD leds;
  UWORD pad;
};

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

static void
zero (void *p, int n)
{
  int i;

  for (i = 0; i < n; i++)
    {
      ((UBYTE *)p) [i] = 0;
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

/* on / 1 / enable / true and off / 0 / disable / false.  -1 if neither. */

static int
parse_bool (const char *s)
{
  if (streq_ci (s, "on") || streq_ci (s, "1") || streq_ci (s, "enable")
      || streq_ci (s, "enabled") || streq_ci (s, "true")
      || streq_ci (s, "yes"))
    {
      return 1;
    }

  if (streq_ci (s, "off") || streq_ci (s, "0") || streq_ci (s, "disable")
      || streq_ci (s, "disabled") || streq_ci (s, "false")
      || streq_ci (s, "no"))
    {
      return 0;
    }

  return -1;
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: NUMLOCK [-h] [-q] [-s on|off] [-l on|off]\r\n");
  puts ("  (no args)  same as -q\r\n");
  puts ("  -h         show this help text\r\n");
  puts ("  -l arg     Num-Lock LED reporting on or off\r\n");
  puts ("  -q         query the Num-Lock state and LED setting\r\n");
  puts ("  -s arg     turn Num-Lock on or off\r\n");
  puts ("\r\n");
}

/*****************************************************************************/

static void
report (const struct kbdlock *k)
{
  puts ("Num-Lock status: ");
  puts ((k->locks & KBDL_NUM) ? "enabled\r\n" : "disabled\r\n");

  puts ("Num-Lock light: ");
  puts ((k->leds & KBDL_NUM) ? "enabled\r\n" : "disabled\r\n");
}

/*****************************************************************************/

/* Split the command tail into NUL-terminated words. */

#define MAX_ARGS 12

static char *argv [MAX_ARGS];
static int argc;

static void
split_tail (char *tail)
{
  int i = 0;

  argc = 0;

  for (;;)
    {
      while (tail [i] == ' ' || tail [i] == '\t')
        {
          tail [i++] = 0;
        }

      if (!tail [i])
        {
          return;
        }

      if (argc < MAX_ARGS)
        {
          argv [argc++] = &tail [i];
        }

      while (tail [i] && tail [i] != ' ' && tail [i] != '\t')
        {
          i++;
        }
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char tail [128];
  struct kbdlock k;
  unsigned tlen, i;
  int a;
  int set_lock = -1;
  int set_lamp = -1;

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
  split_tail (tail);

  for (a = 0; a < argc; a++)
    {
      char *w = argv [a];
      int j = 1;

      if (w [0] != '-' && w [0] != '/')
        {
          puts ("\r\n");
          usage ();
          (void)bdos (0, 0);

          return;
        }

      while (w [j])
        {
          char c = w [j++];

          if (c >= 'A' && c <= 'Z')
            {
              c = (char)(c + 32);
            }

          if (c == 's' || c == 'l')
            {
              const char *arg;
              int v;

              if (w [j] == '=' || w [j] == ':')
                {
                  j++;
                }

              if (w [j])
                {
                  arg = &w [j];

                  while (w [j])
                    {
                      j++;
                    }
                }
              else if (a + 1 < argc)
                {
                  arg = argv [++a];
                }
              else
                {
                  puts ("\r\nMissing on or off after -");
                  putch (c);
                  puts ("\r\n\r\n");
                  usage ();
                  (void)bdos (0, 0);

                  return;
                }

              v = parse_bool (arg);

              if (v < 0)
                {
                  puts ("\r\nExpected on or off, got: ");
                  puts (arg);
                  puts ("\r\n\r\n");
                  usage ();
                  (void)bdos (0, 0);

                  return;
                }

              if (c == 's')
                {
                  set_lock = v;
                }
              else
                {
                  set_lamp = v;
                }

              continue;
            }

          if (c == 'q')
            {
              continue; /* reporting happens either way */
            }

          if (c == 'h' || c == '?')
            {
              puts ("\r\n");
              usage ();
              (void)bdos (0, 0);

              return;
            }

          puts ("\r\n");
          usage ();
          (void)bdos (0, 0);

          return;
        }
    }

  puts ("\r\n");

  /* Read the current state, apply whatever was asked for, then report. */
  zero (&k, sizeof (k));

  if (bdos (BDOS_KBD_LOCK, (LONG)(ULONG)&k) != 0)
    {
      puts ("Keyboard lock control is not available.\r\n");
      (void)bdos (0, 0);

      return;
    }

  if (set_lock >= 0 || set_lamp >= 0)
    {
      UWORD locks = k.locks;
      UWORD leds = k.leds;

      if (set_lock >= 0)
        {
          locks = (UWORD)(set_lock ? (locks | KBDL_NUM)
                                   : (locks & ~KBDL_NUM));
        }

      if (set_lamp >= 0)
        {
          leds = (UWORD)(set_lamp ? (leds | KBDL_NUM)
                                  : (leds & ~KBDL_NUM));
        }

      zero (&k, sizeof (k));
      k.flags = KBDL_SET_LOCKS | KBDL_SET_LEDS;
      k.locks = locks;
      k.leds = leds;

      if (bdos (BDOS_KBD_LOCK, (LONG)(ULONG)&k) != 0)
        {
          puts ("Keyboard lock control is not available.\r\n");
          (void)bdos (0, 0);

          return;
        }
    }

  report (&k);
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
