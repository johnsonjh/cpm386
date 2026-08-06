/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 67c71640-8caf-11f1-992b-80ee73e9b8e7
 */

/*****************************************************************************/

/* textmode.c - console text mode and cursor (BDOS 229/230/231/234) */

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

/*****************************************************************************/

#define BDOS_CONIN 1
#define BDOS_CONST 11
#define BDOS_GET_TICKS 225
#define BDOS_VID_QUERY 229
#define BDOS_VID_ENUM 230
#define BDOS_VID_SET 231
#define BDOS_VID_CURSOR 234

/*****************************************************************************/

#define VIDM_TEXT 0x0001
#define VIDM_GRAPHICS 0x0002
#define VIDM_CURRENT 0x0004
#define VIDM_CONSOLE 0x0008
#define VIDM_DEFAULT 0x0010
#define VIDM_LFB 0x0020
#define VIDM_VESA 0x0040

/*****************************************************************************/

#define VIDA_TRANSIENT 0
#define VIDA_CONSOLE 1
#define VIDA_COMMIT 2
#define VIDA_REVERT 3

/*****************************************************************************/

#define VIDR_OK 0x0000
#define VIDR_FAILED 0xFFFB
#define VIDR_NOMORE 0xFFFC
#define VIDR_BADMODE 0xFFFD
#define VIDR_NOHW 0xFFFE
#define VIDR_BADPTR 0xFFFF

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/* Seconds the mode change is held before it is undone. */
#define CONFIRM_SECS 9

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

/*****************************************************************************/

/* Must match struct cpm_vidset in bdosdef.h */
struct vidset
{
  UWORD mode;
  UWORD action;
  UWORD flags;
  UWORD pad;
};

/*****************************************************************************/

/* Must match struct cpm_vidcursor and the VIDC_* values in bdosdef.h */

#define VIDC_SET_SHAPE 0x0001
#define VIDC_SET_VISIBLE 0x0002
#define VIDC_SET_BLINK 0x0004

#define VIDC_SHAPE_KEEP 0
#define VIDC_SHAPE_BLOCK 1
#define VIDC_SHAPE_UNDERLINE 2
#define VIDC_SHAPE_HALF 3
#define VIDC_SHAPE_EXPLICIT 4

struct vidcursor
{
  UWORD flags;
  UWORD shape;
  UWORD start;
  UWORD end;
  UWORD visible;
  UWORD blink;
  UWORD cell_h;
  UWORD pad;
};

/*****************************************************************************/

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
  puts ("Usage: TEXTMODE [-h] [-a] [-y] [-t] [-c shape] [-b on|off] "
        "[mode]\r\n");
  puts ("  (no args)  print the current mode and the available modes\r\n");
  puts ("  mode       set the mode (asks for confirmation)\r\n");
  puts ("  -a         list graphics modes as well as text modes\r\n");
  puts ("  -b arg     cursor blink: on/off (or 1/0, true/false)\r\n");
  puts ("  -c type    cursor type: block, underline, half, or first,last\r\n");
  puts ("  -h         show this help text\r\n");
  puts ("  -t         set as a transient mode (reset at program exit)\r\n");
  puts ("  -y         set without asking; the mode is kept immediately\r\n");
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

static int
parse_bool (const char *s)
{
  if (streq_ci (s, "on") || streq_ci (s, "enable") || streq_ci (s, "enabled")
      || streq_ci (s, "1") || streq_ci (s, "true") || streq_ci (s, "yes"))
    {
      return 1;
    }

  if (streq_ci (s, "off") || streq_ci (s, "disable")
      || streq_ci (s, "disabled") || streq_ci (s, "0")
      || streq_ci (s, "false") || streq_ci (s, "no"))
    {
      return 0;
    }

  return -1;
}

/*****************************************************************************/

/*
 * Cursor shape: a name, or an explicit first,last scan line pair.  Returns
 * VIDC_SHAPE_* and fills *first / *last for the explicit form, or -1.
 */

static int
parse_shape (const char *s, UWORD *first, UWORD *last)
{
  if (streq_ci (s, "block") || streq_ci (s, "b") || streq_ci (s, "full"))
    {
      return VIDC_SHAPE_BLOCK;
    }

  if (streq_ci (s, "underline") || streq_ci (s, "under")
      || streq_ci (s, "line") || streq_ci (s, "u"))
    {
      return VIDC_SHAPE_UNDERLINE;
    }

  if (streq_ci (s, "half") || streq_ci (s, "h"))
    {
      return VIDC_SHAPE_HALF;
    }

  if (*s >= '0' && *s <= '9')
    {
      unsigned a = 0, b = 0;

      while (*s >= '0' && *s <= '9')
        {
          a = a * 10 + (unsigned)(*s++ - '0');
        }

      if (*s != ',' && *s != '-' && *s != ':')
        {
          return -1;
        }

      s++;

      if (!(*s >= '0' && *s <= '9'))
        {
          return -1;
        }

      while (*s >= '0' && *s <= '9')
        {
          b = b * 10 + (unsigned)(*s++ - '0');
        }

      if (*s || a > 31 || b > 31 || a > b)
        {
          return -1;
        }

      *first = (UWORD)a;
      *last = (UWORD)b;

      return VIDC_SHAPE_EXPLICIT;
    }

  return -1;
}

/*****************************************************************************/

static void
show_cursor (const struct vidcursor *c)
{
  puts ("Cursor: ");

  if (!c->visible)
    {
      puts ("hidden");
    }
  else
    {
      switch (c->shape)
        {
        case VIDC_SHAPE_BLOCK:
          puts ("block");

          break;

        case VIDC_SHAPE_UNDERLINE:
          puts ("underline");

          break;

        default:
          puts ("scan lines ");
          putu (c->start);
          putch (',');
          putu (c->end);

          break;
        }

      puts (c->blink ? ", blinking" : ", steady");
    }

  puts (" (cell height ");
  putu (c->cell_h);
  puts (")\r\n");
}

/*****************************************************************************/

/*
 * Apply the cursor options.  shape < 0 and blink < 0 mean "leave alone", so
 * TEXTMODE -c and TEXTMODE -b are independent of each other and of the mode.
 */

static void
set_cursor (int shape, UWORD first, UWORD last, int blink)
{
  struct vidcursor c;
  UWORD r;

  zero (&c, sizeof (c));

  if (shape >= 0)
    {
      c.flags |= VIDC_SET_SHAPE;
      c.shape = (UWORD)shape;
      c.start = first;
      c.end = last;
    }

  if (blink >= 0)
    {
      c.flags |= VIDC_SET_BLINK;
      c.blink = (UWORD)blink;
    }

  r = bdos (BDOS_VID_CURSOR, (LONG)(ULONG)&c);

  if (r != VIDR_OK)
    {
      report (r);

      return;
    }

  show_cursor (&c);
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
  puts (")\r\n");

  {
    struct vidcursor c;

    zero (&c, sizeof (c));

    if (bdos (BDOS_VID_CURSOR, (LONG)(ULONG)&c) == VIDR_OK)
      {
        show_cursor (&c);
      }
  }

  puts ("\r\n");

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

/* Overwrite the prompt line; ANSI cannot be assumed on the VGA path. */
static void
clear_line (void)
{
  int i;

  putch ('\r');

  for (i = 0; i < 39; i++)
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
  int drain = 0;
  struct cpm_ticks t;
  ULONG start_lo, start_hi;
  ULONG now_lo, now_hi;
  ULONG elapsed;
  ULONG total_ticks;
  ULONG last_secs = (ULONG)CONFIRM_SECS;

  /* Throw away anything already typed ahead! */
  while (bdos (BDOS_CONST, 0) != 0 && drain++ < 64)
    {
      (void)bdos (6, 0xFF);
    }

  zero (&t, sizeof (t));

  if (bdos (BDOS_GET_TICKS, (LONG)(ULONG)&t) != 0)
    {
      return 0;
    }

  hz = t.hz;

  if (hz == 0)
    {
      return 0;
    }

  start_lo = t.lo;
  start_hi = t.hi;
  total_ticks = hz * (ULONG)CONFIRM_SECS;

  clear_line ();
  puts ("Mode changed correctly [y/N] (");
  putu (last_secs + 1);
  puts ("s)? ");

  for (;;)
    {
      if (bdos (BDOS_CONST, 0) != 0)
        {
          int c = (int)bdos (6, 0xFF) & 0xFF;

          clear_line ();

          return (c == 'y' || c == 'Y');
        }

      zero (&t, sizeof (t));

      if (bdos (BDOS_GET_TICKS, (LONG)(ULONG)&t) != 0)
        {
          return 0;
        }

      now_lo = t.lo;
      now_hi = t.hi;

      if (now_hi < start_hi ||
          (now_hi == start_hi && now_lo < start_lo))
        {
          continue;
        }

      if (now_hi == start_hi)
        {
          elapsed = now_lo - start_lo;
        }
      else
        {
          elapsed = (0xFFFFFFFFUL - start_lo) + 1 + now_lo;
        }

      if (elapsed >= total_ticks)
        {
          break;
        }

      {
        ULONG remaining_secs = (total_ticks - elapsed) / hz;

        if (remaining_secs != last_secs)
          {
            last_secs = remaining_secs;

            clear_line ();
            puts ("Mode changed correctly [y/N] (");
            putu (last_secs + 1);
            puts ("s)? ");
          }
      }
    }

  clear_line ();

  return 0;
}

/*****************************************************************************/

static void
report_mode (void)
{
  struct vidmode v;

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

static void
set_mode (unsigned mode, int assume_yes, int transient)
{
  struct vidset s;
  UWORD r;

  if (!transient)
    {
      struct vidmode cur;

      zero (&cur, sizeof (cur));

      if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&cur) == VIDR_OK
          && cur.mode == (UWORD)mode)
        {
          if (!(cur.flags & VIDM_CONSOLE))
            {
              zero (&s, sizeof (s));
              s.mode = (UWORD)mode;
              s.action = VIDA_CONSOLE;
              (void)bdos (BDOS_VID_SET, (LONG)(ULONG)&s);

              zero (&s, sizeof (s));
              s.action = VIDA_COMMIT;
              (void)bdos (BDOS_VID_SET, (LONG)(ULONG)&s);
            }

          report_mode ();

          return;
        }
    }

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

  report_mode ();
}

/*****************************************************************************/

/* Split the command tail into NUL-terminated words. */

#define MAX_ARGS 16

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
  unsigned tlen, i;
  int a;
  int assume_yes = 0;
  int transient = 0;
  int list_all = 0;
  int have_mode = 0;
  unsigned mode = 0;
  int shape = -1;
  int blink = -1;
  UWORD first = 0, last = 0;

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

      if (w [0] == '-' || w [0] == '/')
        {
          int j = 1;

          while (w [j])
            {
              char c = w [j++];

              if (c >= 'A' && c <= 'Z')
                {
                  c = (char)(c + 32);
                }

              if (c == 'c' || c == 'b')
                {
                  const char *arg;

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
                      usage ();
                      (void)bdos (0, 0);

                      return;
                    }

                  if (c == 'c')
                    {
                      shape = parse_shape (arg, &first, &last);

                      if (shape < 0)
                        {
                          puts ("\r\nUnknown cursor shape: ");
                          puts (arg);
                          puts ("\r\n\r\n");
                          usage ();
                          (void)bdos (0, 0);

                          return;
                        }
                    }
                  else
                    {
                      blink = parse_bool (arg);

                      if (blink < 0)
                        {
                          puts ("\r\nExpected on or off, got: ");
                          puts (arg);
                          puts ("\r\n\r\n");
                          usage ();
                          (void)bdos (0, 0);

                          return;
                        }
                    }

                  continue;
                }

              if (c == 'y')
                {
                  assume_yes = 1;
                }
              else if (c == 't')
                {
                  transient = 1;
                }
              else if (c == 'a')
                {
                  list_all = 1;
                }
              else
                {
                  puts ("\r\n");
                  usage ();
                  (void)bdos (0, 0);

                  return;
                }
            }

          continue;
        }

      if (w [0] >= '0' && w [0] <= '9')
        {
          int j = 0;

          mode = 0;

          while (w [j] >= '0' && w [j] <= '9')
            {
              mode = mode * 10 + (unsigned)(w [j++] - '0');
            }

          if (w [j])
            {
              puts ("\r\n");
              usage ();
              (void)bdos (0, 0);

              return;
            }

          have_mode = 1;

          continue;
        }

      puts ("\r\n");
      usage ();
      (void)bdos (0, 0);

      return;
    }

  puts ("\r\n");

  if (have_mode)
    {
      set_mode (mode, assume_yes, transient);
    }

  if (shape >= 0 || blink >= 0)
    {
      set_cursor (shape, first, last, blink);
    }
  else if (!have_mode)
    {
      show_modes (list_all);
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
