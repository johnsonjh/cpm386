/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 67c71640-8caf-11f1-992b-80ee73e9b8e7
 */

/*****************************************************************************/

/* textmode.c - query and set the console text mode (BDOS 229/230/231) */

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

#define BDOS_CONIN 1
#define BDOS_CONST 11
#define BDOS_GET_TICKS 225
#define BDOS_VID_QUERY 229
#define BDOS_VID_ENUM 230
#define BDOS_VID_SET 231

/*****************************************************************************/

/* Must match vidmode.h */

#define VIDM_TEXT 0x0001
#define VIDM_GRAPHICS 0x0002
#define VIDM_CURRENT 0x0004
#define VIDM_CONSOLE 0x0008
#define VIDM_DEFAULT 0x0010
#define VIDM_LFB 0x0020
#define VIDM_VESA 0x0040

#define VIDA_TRANSIENT 0
#define VIDA_CONSOLE 1
#define VIDA_COMMIT 2
#define VIDA_REVERT 3

#define VIDR_OK 0x0000
#define VIDR_FAILED 0xFFFB
#define VIDR_NOMORE 0xFFFC
#define VIDR_BADMODE 0xFFFD
#define VIDR_NOHW 0xFFFE
#define VIDR_BADPTR 0xFFFF

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/* Seconds the mode change is held before it is undone. */
#define CONFIRM_SECS 5

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

/* Must match struct cpm_vidmode in bdosdef.h */
struct vidmode
{
  UWORD index;
  UWORD mode;
  UWORD flags;
  UWORD cols;
  UWORD rows;
  UWORD cell_bytes;
  UWORD width;
  UWORD height;
  UWORD bpp;
  UWORD sel;
  ULONG pitch;
  ULONG fb_size;
  ULONG fb_phys;
};

/* Must match struct cpm_vidset in bdosdef.h */
struct vidset
{
  UWORD mode;
  UWORD action;
  UWORD flags;
  UWORD pad;
};

/* Must match struct cpm_ticks in bdosdef.h */
struct cpm_ticks
{
  ULONG lo;
  ULONG hi;
  ULONG hz;
};

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
putu (ULONG n)
{
  char b [12];
  int i = 0;

  if (!n)
    {
      putch ('0');

      return;
    }

  while (n && i < 12)
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

/* Right-align a small number in a field of the given width. */
static int
digits_in (ULONG n)
{
  int d = 1;

  while (n >= 10)
    {
      n /= 10;
      d++;
    }

  return d;
}

/*****************************************************************************/

static void
putu_pad (ULONG n, int width)
{
  int digits = digits_in (n);

  while (digits++ < width)
    {
      putch (' ');
    }

  putu (n);
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

static void
report (UWORD r)
{
  switch (r)
    {
    case VIDR_NOHW:
      puts ("No video adapter present\r\n");

      break;

    case VIDR_BADMODE:
      puts ("Mode not supported by this adapter\r\n");

      break;

    case VIDR_FAILED:
      puts ("The video BIOS refused the mode\r\n");

      break;

    case VIDR_BADPTR:
      puts ("BDOS rejected the request\r\n");

      break;

    default:
      puts ("Unexpected status\r\n");

      break;
    }
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: TEXTMODE [-h] [-a] [-y] [-t] [mode]\r\n");
  puts ("  (no args)  print the current mode and the available modes\r\n");
  puts ("  mode       set the mode (asks for confirmation)\r\n");
  puts ("  -y         set without asking; the mode is kept immediately\r\n");
  puts ("  -a         also list graphics modes\r\n");
  puts ("  -t         set as a transient mode, undone when this exits\r\n");
  puts ("  -h         show this help\r\n");
}

/*****************************************************************************/

static void
show_modes (int all)
{
  struct vidmode v;
  UWORD r;
  unsigned i;

  zero (&v, sizeof (v));
  r = bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v);

  if (r != VIDR_OK)
    {
      report (r);

      return;
    }

  puts ("Current mode: ");
  putu (v.mode);
  puts (" (");
  putu (v.cols);
  puts ("x");
  putu (v.rows);
  puts (")\r\n\r\n");

  puts ("Mode  Size          Flags\r\n");
  puts ("----  ------------  -----------------------\r\n");

  for (i = 0;; i++)
    {
      int width = 0;

      zero (&v, sizeof (v));
      v.index = (UWORD)i;
      r = bdos (BDOS_VID_ENUM, (LONG)(ULONG)&v);

      if (r == VIDR_NOMORE)
        {
          break;
        }

      if (r != VIDR_OK)
        {
          report (r);

          return;
        }

      /* Graphics modes are only listed on request; this is a text tool. */
      if (!(v.flags & VIDM_TEXT) && !all)
        {
          continue;
        }

      putu_pad (v.mode, 4);
      puts ("  ");

      if (v.flags & VIDM_TEXT)
        {
          putu_pad (v.cols, 4);
          putch ('x');
          putu (v.rows);
          width = 5 + digits_in (v.rows);
        }
      else
        {
          putu_pad (v.width, 4);
          putch ('x');
          putu (v.height);
          putch ('x');
          putu (v.bpp);
          width = 6 + digits_in (v.height) + digits_in (v.bpp);
        }

      while (width++ < 12)
        {
          putch (' ');
        }

      puts ("  ");

      if (v.flags & VIDM_GRAPHICS)
        {
          puts ((v.flags & VIDM_VESA) ? "vesa " : "vga ");
        }

      if (v.flags & VIDM_CURRENT)
        {
          puts ("current ");
        }

      if (v.flags & VIDM_CONSOLE)
        {
          puts ("console ");
        }

      if (v.flags & VIDM_DEFAULT)
        {
          puts ("default ");
        }

      puts ("\r\n");
    }
}

/*****************************************************************************/

static ULONG
now_lo (ULONG *hz)
{
  struct cpm_ticks t;

  zero (&t, sizeof (t));

  if (bdos (BDOS_GET_TICKS, (LONG)(ULONG)&t) != 0)
    {
      *hz = 0;

      return 0;
    }

  *hz = t.hz;

  return t.lo;
}

/*****************************************************************************/

/* Overwrite the prompt line; ANSI cannot be assumed on the VGA path. */
static void
clear_line (void)
{
  int i;

  putch ('\r');

  for (i = 0; i < 52; i++)
    {
      putch (' ');
    }

  putch ('\r');
}

/*****************************************************************************/

static int
confirm (void)
{
  ULONG hz;
  int secs;
  int drain = 0;

  /* Throw away anything already typed ahead! */
  while (bdos (BDOS_CONST, 0) != 0 && drain++ < 64)
    {
      (void)bdos (6, 0xFF);
    }

  (void)now_lo (&hz);

  if (hz == 0)
    {
      return 0;
    }

  for (secs = CONFIRM_SECS; secs > 0; secs--)
    {
      ULONG start;
      ULONG dummy;
      ULONG elapsed;

      clear_line ();
      puts ("Did the mode change work correctly [y/N] (");
      putu ((ULONG)secs);
      puts ("s)? ");

      start = now_lo (&dummy);

      for (;;)
        {
          if (bdos (BDOS_CONST, 0) != 0)
            {
              int c = (int)bdos (6, 0xFF) & 0xFF;

              clear_line ();

              return (c == 'y' || c == 'Y');
            }

          /* Unsigned arithmetic, so the 32-bit tick wrap takes care of
           * itself. */
          elapsed = now_lo (&dummy) - start;

          if (elapsed >= hz)
            {
              break;
            }
        }
    }

  clear_line ();

  return 0;
}

/*****************************************************************************/

static void
set_mode (unsigned mode, int assume_yes, int transient)
{
  struct vidset s;
  struct vidmode v;
  UWORD r;

  zero (&s, sizeof (s));
  s.mode = (UWORD)mode;
  s.action = (UWORD)(transient ? VIDA_TRANSIENT : VIDA_CONSOLE);
  r = bdos (BDOS_VID_SET, (LONG)(ULONG)&s);

  if (r != VIDR_OK)
    {
      report (r);

      return;
    }

  /* A transient mode belongs to the running program, not to the console */
  if (transient)
    {
      puts ("Transient mode ");
      putu ((ULONG)mode);
      puts (" set; the console mode returns when this program exits.\r\n");

      return;
    }

  if (!assume_yes && !confirm ())
    {
      zero (&s, sizeof (s));
      s.action = VIDA_REVERT;
      (void)bdos (BDOS_VID_SET, (LONG)(ULONG)&s);
      puts ("Mode change undone.\r\n");

      return;
    }

  zero (&s, sizeof (s));
  s.action = VIDA_COMMIT;
  (void)bdos (BDOS_VID_SET, (LONG)(ULONG)&s);

  zero (&v, sizeof (v));

  if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v) == VIDR_OK)
    {
      puts ("Console mode is now ");
      putu (v.mode);
      puts (" (");
      putu (v.cols);
      puts ("x");
      putu (v.rows);
      puts (")\r\n");
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char tail [128];
  unsigned tlen, i;
  int assume_yes = 0;
  int transient = 0;
  int list_all = 0;
  int have_mode = 0;
  unsigned mode = 0;

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

  while (tail [i])
    {
      while (tail [i] == ' ' || tail [i] == '\t')
        {
          i++;
        }

      if (!tail [i])
        {
          break;
        }

      if (tail [i] == '-' || tail [i] == '/')
        {
          i++;

          while (tail [i] && tail [i] != ' ' && tail [i] != '\t')
            {
              char c = tail [i++];

              if (c >= 'a' && c <= 'z')
                {
                  c = (char)(c - 32);
                }

              if (c == 'Y')
                {
                  assume_yes = 1;
                }
              else if (c == 'T')
                {
                  transient = 1;
                }
              else if (c == 'A')
                {
                  list_all = 1;
                }
              else if (c == 'H')
                {
                  usage ();
                  (void)bdos (0, 0);
                }
              else
                {
                  usage ();
                  (void)bdos (0, 0);
                }
            }

          continue;
        }

      if (tail [i] >= '0' && tail [i] <= '9')
        {
          mode = 0;

          while (tail [i] >= '0' && tail [i] <= '9')
            {
              mode = mode * 10 + (unsigned)(tail [i++] - '0');
            }

          have_mode = 1;

          continue;
        }

      usage ();
      (void)bdos (0, 0);
    }

  puts ("\r\n");

  if (!have_mode)
    {
      show_modes (list_all);
    }
  else
    {
      set_mode (mode, assume_yes, transient);
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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
