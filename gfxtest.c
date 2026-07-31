/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: b78dcf04-8cb7-11f1-9112-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * gfxtest.c - graphics mode demo:
 * can be a reference for how a real graphics program should be structured.
 */

/*****************************************************************************/

#include "absaddr.h"
#include "vgafb.h"

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30

#define BDOS_CONST 11
#define BDOS_GET_TICKS 225
#define BDOS_VID_QUERY 229
#define BDOS_VID_ENUM 230
#define BDOS_VID_SET 231
#define BDOS_VID_PALETTE 233

/*****************************************************************************/

/* Must match vidmode.h */

#define VIDM_TEXT 0x0001
#define VIDM_GRAPHICS 0x0002
#define VIDM_CURRENT 0x0004
#define VIDM_VESA 0x0040

#define VIDA_TRANSIENT 0
#define VIDA_REVERT 3

#define VIDR_OK 0x0000
#define VIDR_NOMORE 0xFFFC
#define VIDR_NOHW 0xFFFE

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/* 320x200 packed pixel; the frame buffer we compose into. */
#define W 320
#define H 200

/* How long the pattern is left on screen before returning to text. */
#define SHOW_SECS 5

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

/* Must match struct cpm_vidpal in bdosdef.h */
struct vidpal
{
  ULONG data;
  UWORD first;
  UWORD count;
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

/* Composed here, then pushed across in one blit. */
static UBYTE frame [W * H];
static UBYTE pal [256 * 3];

/*****************************************************************************/

/*
 * A 256 entry ramp: 64 greys, then red, green and blue ramps.  Components
 * are the VGA's native 6 bits, so 0 to 63.
 */

static void
build_palette (void)
{
  int i;

  for (i = 0; i < 64; i++)
    {
      pal [i * 3 + 0] = (UBYTE)i;
      pal [i * 3 + 1] = (UBYTE)i;
      pal [i * 3 + 2] = (UBYTE)i;

      pal [(64 + i) * 3 + 0] = (UBYTE)i;
      pal [(64 + i) * 3 + 1] = 0;
      pal [(64 + i) * 3 + 2] = 0;

      pal [(128 + i) * 3 + 0] = 0;
      pal [(128 + i) * 3 + 1] = (UBYTE)i;
      pal [(128 + i) * 3 + 2] = 0;

      pal [(192 + i) * 3 + 0] = 0;
      pal [(192 + i) * 3 + 1] = 0;
      pal [(192 + i) * 3 + 2] = (UBYTE)i;
    }
}

/*****************************************************************************/

/*
 * Four horizontal bands of ramps, with a one pixel border and cross hairs
 * so the edges of the visible area are obvious.
 */

static void
build_frame (void)
{
  int x, y;

  for (y = 0; y < H; y++)
    {
      int band = (y * 4) / H;

      for (x = 0; x < W; x++)
        {
          frame [y * W + x] = (UBYTE)(band * 64 + (x * 64) / W);
        }
    }

  for (x = 0; x < W; x++)
    {
      frame [x] = 63;
      frame [(H - 1) * W + x] = 63;
      frame [(H / 2) * W + x] = 63;
    }

  for (y = 0; y < H; y++)
    {
      frame [y * W] = 63;
      frame [y * W + W - 1] = 63;
      frame [y * W + W / 2] = 63;
    }
}

/*****************************************************************************/

/*
 * Read a sample of the framebuffer back through the selector and compare
 * it with what was sent.  The pattern itself needs eyes on a screen, but
 * this part does not: it proves the mode, the selector, the pitch and the
 * blit all agree, and it is what catches a wrong stride rather than a
 * wrong colour.  The step sizes are coprime with the width so the samples
 * do not all land in the same column.
 */

static unsigned g_checked;
static unsigned g_bad;
static UWORD g_palrc = VIDR_OK;
static int g_bykey;

static void
verify_frame (unsigned short sel, unsigned long pitch)
{
  unsigned y, x;

  g_checked = 0;
  g_bad = 0;

  for (y = 0; y < H; y += 7)
    {
      for (x = 0; x < W; x += 11)
        {
          UBYTE got = vgafb_load8 (sel, (unsigned long)y * pitch + x);

          g_checked++;

          if (got != frame [y * W + x])
            {
              g_bad++;
            }
        }
    }
}

/*****************************************************************************/

/*
 * Wait n seconds, or until a key is pressed.  Returns 1 if a keypress
 * ended it, 0 if the time ran out.
 */

static int
wait_secs (int n)
{
  struct cpm_ticks t;
  ULONG hz, start, now;
  int drain = 0;

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
  start = t.lo;

  for (;;)
    {
      if (bdos (BDOS_CONST, 0) != 0)
        {
          (void)bdos (6, 0xFF);

          return 1;
        }

      zero (&t, sizeof (t));

      if (bdos (BDOS_GET_TICKS, (LONG)(ULONG)&t) != 0)
        {
          return 0;
        }

      now = t.lo - start;

      if (now >= hz * (ULONG)n)
        {
          return 0;
        }
    }
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: GFXTEST [-h] [mode]\r\n");
  puts ("  (no args)  use the standard 320x200x8 VGA mode\r\n");
  puts ("  mode       use that mode instead (see TEXTMODE -A)\r\n");
  puts ("\r\n");
  puts ("Sets the mode transiently, draws a test pattern, and returns\r\n");
  puts ("to the console after 5 seconds or a keypress.  Only 8 bits\r\n");
  puts ("per pixel modes are drawn.\r\n");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct vidmode v;
  struct vidset s;
  struct vidpal p;
  UWORD r;
  unsigned want = 5; /* the standard 320x200x256 mode */
  unsigned tlen, i;
  int have_arg = 0;

  puts ("\r\n");

  tlen = CMD_TAIL [0];
  i = 1;

  while (i <= tlen && (CMD_TAIL [i] == ' ' || CMD_TAIL [i] == '\t'))
    {
      i++;
    }

  if (i <= tlen && (CMD_TAIL [i] == '-' || CMD_TAIL [i] == '/'))
    {
      usage ();
      (void)bdos (0, 0);
    }

  if (i <= tlen && CMD_TAIL [i] >= '0' && CMD_TAIL [i] <= '9')
    {
      want = 0;

      while (i <= tlen && CMD_TAIL [i] >= '0' && CMD_TAIL [i] <= '9')
        {
          want = want * 10 + (unsigned)(CMD_TAIL [i++] - '0');
        }

      have_arg = 1;
    }

  (void)have_arg;

  /*
   * Find the mode in the enumeration so its geometry is known before it is
   * set - once the mode is up, printing an error would go to a screen that
   * is no longer showing text.
   */

  for (i = 0;; i++)
    {
      zero (&v, sizeof (v));
      v.index = (UWORD)i;
      r = bdos (BDOS_VID_ENUM, (LONG)(ULONG)&v);

      if (r == VIDR_NOHW)
        {
          puts ("No video adapter present\r\n");
          (void)bdos (0, 0);
        }

      if (r == VIDR_NOMORE)
        {
          puts ("No such mode; try TEXTMODE -A for the list\r\n");
          (void)bdos (0, 0);
        }

      if (r == VIDR_OK && v.mode == want)
        {
          break;
        }
    }

  if (!(v.flags & VIDM_GRAPHICS))
    {
      puts ("Mode ");
      putu (want);
      puts (" is a text mode; use TEXTMODE for those\r\n");
      (void)bdos (0, 0);
    }

  if (v.bpp != 8)
    {
      puts ("Mode ");
      putu (want);
      puts (" is ");
      putu (v.bpp);
      puts (" bits per pixel; this demo only draws 8\r\n");
      (void)bdos (0, 0);
    }

  if (v.width < W || v.height < H)
    {
      puts ("Mode is smaller than the test pattern\r\n");
      (void)bdos (0, 0);
    }

  puts ("Mode ");
  putu (v.mode);
  puts (": ");
  putu (v.width);
  puts ("x");
  putu (v.height);
  puts ("x");
  putu (v.bpp);
  puts (v.flags & VIDM_VESA ? " (VESA linear)" : " (VGA)");
  puts ("\r\n");
  puts ("Drawing for ");
  putu (SHOW_SECS);
  puts (" seconds, or press a key...\r\n");

  build_palette ();
  build_frame ();

  /*
   * Transient, so the kernel restores the console mode when this program
   * exits - including if it crashes before getting to the revert below.
   */

  zero (&s, sizeof (s));
  s.mode = (UWORD)v.mode;
  s.action = VIDA_TRANSIENT;
  r = bdos (BDOS_VID_SET, (LONG)(ULONG)&s);

  if (r != VIDR_OK)
    {
      puts ("Could not set the mode\r\n");
      (void)bdos (0, 0);
    }

  /* Re-query: the selector only exists once the mode is actually up. */
  zero (&v, sizeof (v));

  if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v) == VIDR_OK && v.sel != 0)
    {
      zero (&p, sizeof (p));
      p.data = (ULONG)(unsigned long)pal;
      p.first = 0;
      p.count = 256;
      g_palrc = bdos (BDOS_VID_PALETTE, (LONG)(ULONG)&p);

      /*
       * One row at a time, because a mode wider than the pattern has a
       * pitch larger than W.  A full-width renderer would blit the whole
       * frame in a single call.
       */

      for (i = 0; i < H; i++)
        {
          vgafb_blit (v.sel, (unsigned long)i * v.pitch, &frame [i * W], W);
        }

      verify_frame (v.sel, v.pitch);
      g_bykey = wait_secs (SHOW_SECS);
    }

  zero (&s, sizeof (s));
  s.action = VIDA_REVERT;
  (void)bdos (BDOS_VID_SET, (LONG)(ULONG)&s);

  puts ("\r\nGFXTEST: ");
  putu ((ULONG)(g_checked - g_bad));
  putch ('/');
  putu ((ULONG)g_checked);
  puts (" sampled pixels read back correctly\r\n");

  if (g_bad)
    {
      puts ("GFXTEST: FRAMEBUFFER MISMATCH\r\n");
    }

  /*
   * A rejected palette leaves the BIOS default in the DAC, which still
   * shows something - but silently discarding the result would hide a
   * real failure, and a wrong palette is one way a correct framebuffer
   * still looks wrong.
   */

  if (g_palrc != VIDR_OK)
    {
      puts ("GFXTEST: palette rejected, status 0x");
      {
        static const char h [] = "0123456789ABCDEF";
        int k;

        for (k = 3; k >= 0; k--)
          {
            putch (h [(g_palrc >> (k * 4)) & 0xF]);
          }
      }
      puts ("\r\n");
    }

  puts (g_bykey ? "GFXTEST: ended by keypress\r\n"
                : "GFXTEST: displayed for the full 5 seconds\r\n");
  puts ("GFXTEST done.\r\n");

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
