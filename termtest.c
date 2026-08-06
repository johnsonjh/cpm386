/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 4b2d9efe-91a6-11f1-9eff-80ee73e9b8e7
 */

/*****************************************************************************/

/* NOTE: Most of this test code was created with AI/LLM assistance! */

/*****************************************************************************/

/* termtest.c - exercise the VT102 / VT52 / DRI console and the keyboard */

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
#define BDOS_CONOUT 2
#define BDOS_CONST 11
#define BDOS_DIRCON 6
#define BDOS_VID_QUERY 229
#define BDOS_VID_CURSOR 234

/*****************************************************************************/

/* Must match struct cpm_vidmode in bdosdef.h (leading fields only used). */

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

/* Must match struct cpm_vidcursor and the VIDC_* values in bdosdef.h */

#define VIDC_SET_SHAPE 0x0001
#define VIDC_SET_VISIBLE 0x0002
#define VIDC_SET_BLINK 0x0004
#define VIDC_SHAPE_UNDERLINE 2

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
  (void)bdos (BDOS_CONOUT, (LONG)(unsigned char)c);
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
puthex2 (unsigned v)
{
  static const char hex [] = "0123456789ABCDEF";

  putch (hex [(v >> 4) & 0xF]);
  putch (hex [v & 0xF]);
}

/*****************************************************************************/

/* ESC [ <n> <final> */

static void
csi (ULONG n, char final)
{
  puts ("\033[");
  putu (n);
  putch (final);
}

/*****************************************************************************/

static void
goto_rc (unsigned row, unsigned col)
{
  puts ("\033[");
  putu (row);
  putch (';');
  putu (col);
  putch ('H');
}

/*****************************************************************************/

static void
cls (void)
{
  puts ("\033[2J\033[H");
}

/*****************************************************************************/

static void
sgr0 (void)
{
  puts ("\033[0m");
}

/*****************************************************************************/

/* Wait for a key, discarding whatever is already typed ahead. */

static int
anykey (const char *prompt)
{
  int drain = 0;

  while (bdos (BDOS_CONST, 0) != 0 && drain++ < 64)
    {
      (void)bdos (BDOS_DIRCON, 0xFF);
    }

  puts (prompt);

  for (;;)
    {
      UWORD c = bdos (BDOS_DIRCON, 0xFF);

      if (c)
        {
          return (int)(c & 0xFF);
        }
    }
}

/*****************************************************************************/

/* 1. SGR: the eight colours, bold, reverse and blink. */

static void
test_sgr (void)
{
  int i;

  cls ();
  puts ("VT102 SGR\r\n\r\n");

  puts (" foreground 30-37: ");

  for (i = 0; i < 8; i++)
    {
      csi ((ULONG)(30 + i), 'm');
      puts ("##");
    }

  sgr0 ();
  puts ("\r\n bold      1;30-37: ");

  for (i = 0; i < 8; i++)
    {
      puts ("\033[1;");
      putu ((ULONG)(30 + i));
      putch ('m');
      puts ("##");
    }

  sgr0 ();
  puts ("\r\n background 40-47: ");

  for (i = 0; i < 8; i++)
    {
      csi ((ULONG)(40 + i), 'm');
      puts ("  ");
    }

  sgr0 ();
  puts ("\r\n\r\n ");
  puts ("\033[7m reverse (7) \033[0m  ");
  puts ("\033[5m blink (5) \033[0m  ");
  puts ("\033[1m bold (1) \033[0m  ");
  puts ("\033[4m underline (4) \033[0m");
  sgr0 ();
  puts ("\r\n");
}

/*****************************************************************************/

/* 2. The DRI DOS-Plus extensions. */

static void
test_dri (void)
{
  int i;

  cls ();
  puts ("DRI DOS-Plus extensions\r\n\r\n");

  puts (" ESC b <c> foreground: ");

  for (i = 0; i < 16; i++)
    {
      puts ("\033b");
      putch ((char)i);
      puts ("#");
    }

  puts ("\033b\007");
  puts ("\r\n ESC c <c> background: ");

  for (i = 0; i < 8; i++)
    {
      puts ("\033c");
      putch ((char)i);
      puts ("  ");
    }

  puts ("\033c");
  putch (0);
  puts ("\r\n\r\n ");
  puts ("\033p black on white (ESC p) \033q back to normal (ESC q)\r\n");
  puts (" \033r bright (ESC r) \033u normal (ESC u)\r\n");
  puts (" \033s flashing (ESC s) \033t steady (ESC t)\r\n");

  puts ("\r\n ESC j / ESC k save and restore: [");
  puts ("\033j");     /* save here */
  puts ("XXXXXX");
  puts ("\033k");     /* back to the saved column */
  puts ("ok    ]\r\n");

  puts ("\r\n ESC f hides the cursor for two seconds...");
  puts ("\033f");
}

/*****************************************************************************/

/* 3. VT52 mode: switch, address the cursor, switch back. */

static void
test_vt52 (void)
{
  int r;

  cls ();
  puts ("VT52 mode (ESC [ ? 2 l in, ESC < out)\r\n");

  puts ("\033[?2l"); /* DECANM reset: VT52 from here on */

  /* ESC Y row col, both biased by 0x20. */
  for (r = 0; r < 6; r++)
    {
      puts ("\033Y");
      putch ((char)(0x20 + 4 + r));
      putch ((char)(0x20 + 4 + r * 3));
      puts ("ESC Y row ");
      putch ((char)('0' + 4 + r));
    }

  puts ("\033Y");
  putch ((char)(0x20 + 12));
  putch ((char)(0x20 + 2));
  puts ("ESC A/B/C/D: ");
  puts ("....");
  puts ("\033D\033D\033D\033D"); /* four cursor lefts */
  puts ("OK++");

  puts ("\033Y");
  putch ((char)(0x20 + 14));
  putch ((char)(0x20 + 2));
  puts ("ESC K erases to end of line: XXXXXXXXXXXXXXXX");
  puts ("\033Y");
  putch ((char)(0x20 + 14));
  putch ((char)(0x20 + 31));
  puts ("\033K");

  puts ("\033Y");
  putch ((char)(0x20 + 16));
  putch ((char)(0x20 + 2));
  puts ("DRI colour still works in VT52 mode: ");
  puts ("\033b");
  putch (10);
  puts ("green");
  puts ("\033b");
  putch (7);

  puts ("\033Y");
  putch ((char)(0x20 + 18));
  putch ((char)(0x20 + 2));
  puts ("\033<"); /* back to ANSI */
  puts ("ESC < returned to ANSI mode; CSI works again: ");
  puts ("\033[1;33mYES\033[0m");
  puts ("\r\n");
}

/*****************************************************************************/

/* 4. Editing: scroll region, IL / DL, ICH / DCH, ECH. */

static void
test_edit (void)
{
  int i;

  cls ();
  puts ("Scrolling region and line / character editing\r\n");

  for (i = 0; i < 10; i++)
    {
      goto_rc ((unsigned)(4 + i), 3);
      puts ("line ");
      putu ((ULONG)i);
      puts (" ......................................");
    }

  goto_rc (16, 1);
  puts ("ESC[5;13r sets a region over rows 5..13; ESC[M there scrolls it.");

  puts ("\033[5;13r"); /* DECSTBM */
  goto_rc (5, 1);
  csi (2, 'M');        /* DL x2 inside the region */
  goto_rc (13, 3);
  puts ("<- two lines deleted, this row is new");

  puts ("\033[r"); /* full screen again */

  goto_rc (18, 3);
  puts ("ICH/DCH: 0123456789");
  goto_rc (18, 14); /* on the '3' */
  csi (3, 'P');     /* delete three */
  goto_rc (18, 30);
  puts ("(deleted 3 characters)");

  goto_rc (19, 3);
  puts ("ECH:     0123456789");
  goto_rc (19, 14);
  csi (4, 'X');
  goto_rc (19, 30);
  puts ("(erased 4 in place)");

  goto_rc (21, 1);
}

/*****************************************************************************/

/* 5. DEC special graphics: a box drawn with ESC ( 0. */

static void
test_gfx (void)
{
  int i;

  cls ();
  puts ("DEC special graphics (ESC ( 0)\r\n\r\n");

  puts ("\033(0"); /* G0 = line drawing */

  puts ("  lqqqqqqqqqqqqqqqqqqqqqqqqqqk\r\n");

  for (i = 0; i < 3; i++)
    {
      puts ("  x                          x\r\n");
    }

  puts ("  tqqqqqqqqqqqqqqqqqqqqqqqqqqu\r\n");
  puts ("  x                          x\r\n");
  puts ("  mqqqqqqqqqqqqqqqqqqqqqqqqqqj\r\n");

  puts ("\033(B"); /* back to ASCII */
  puts ("\r\n  Plus: ~ (dot)  ` (diamond)  a (checker)  f (degree)  g (+/-)\r\n");
  puts ("\033(0  ~ ` a f g\033(B\r\n");
}

/*****************************************************************************/

/* 6. Autowrap on and off. */

static void
test_wrap (void)
{
  int i;

  cls ();
  puts ("Line wrap\r\n\r\n");

  puts ("Wrap on (the default; ESC v / ESC [ ? 7 h):\r\n");
  puts ("\033[?7h");

  for (i = 0; i < 100; i++)
    {
      putch ((char)('0' + i % 10));
    }

  puts ("\r\n\r\nWrap off (ESC w / ESC [ ? 7 l), same 100 characters:\r\n");
  puts ("\033[?7l");

  for (i = 0; i < 100; i++)
    {
      putch ((char)('a' + i % 26));
    }

  puts ("\033[?7h");
  puts ("\r\n\r\nWrap is back on.\r\n");
}

/*****************************************************************************/

/*
 * 7. Keyboard.  Prints the bytes each key press delivers, which is the only
 * way to check that the arrows, the keypad, the function keys and Alt+nnn
 * are producing what they should.
 */

static void
test_keys (void)
{
  int col = 0;
  int escapes = 0;

  cls ();
  puts ("Keyboard: every byte a key delivers is shown in hex.\r\n");
  puts ("Try the arrows, Home/End/PgUp/PgDn, the keypad, F1-F12,\r\n");
  puts ("Ctrl+letter, Alt+letter, and Alt+<digits on the keypad>.\r\n\r\n");
  puts ("Press Ctrl-C three times to finish.\r\n\r\n");

  for (;;)
    {
      UWORD c = bdos (BDOS_DIRCON, 0xFF);

      if (!bdos (BDOS_CONST, 0) && c == 0)
        {
          continue;
        }

      if (c == 0x03)
        {
          if (++escapes >= 3)
            {
              break;
            }
        }
      else
        {
          escapes = 0;
        }

      puthex2 ((unsigned)(c & 0xFF));

      /* Printable bytes are shown as themselves as well. */
      if ((c & 0xFF) >= 0x20 && (c & 0xFF) < 0x7F)
        {
          putch ('(');
          putch ((char)(c & 0xFF));
          putch (')');
        }
      else
        {
          puts ("   ");
        }

      putch (' ');

      if (++col >= 12)
        {
          col = 0;
          puts ("\r\n");
        }
    }

  puts ("\r\n");
}

/*****************************************************************************/

/*
 * Put the console back into a known-good state.  Everything a program can
 * leave behind is undone here: the dialect, the keypad and cursor key modes,
 * wrap, the scrolling region, the character sets, the tab stops, the
 * rendition and the cursor itself.
 */

static void
reset_console (void)
{
  struct vidcursor c;
  struct vidmode v;
  unsigned cols = 80;
  unsigned col;
  int i;

  /*
   * ESC < first: in VT52 mode none of the CSI sequences below would be
   * recognised at all.  It is harmless when the console is already in ANSI
   * mode, where a VT102 leaves ESC < undefined.
   */

  puts ("\033<");

  puts ("\033[!p");       /* DECSTR: modes, margins, rendition, charsets */
  puts ("\033>");         /* numeric keypad                              */
  puts ("\033[?1l");      /* normal cursor keys                          */
  puts ("\033[?7h");      /* wrap on                                     */
  puts ("\033[?5l");      /* normal, not reverse, screen                 */
  puts ("\033[?6l");      /* absolute, not origin-relative, addressing   */
  puts ("\033[?25h");     /* cursor visible                              */
  puts ("\033[4l");       /* replace, not insert                         */
  puts ("\033[20h");      /* LNM, which is the CP/M-386 console default  */
  puts ("\033[r");        /* full-screen scrolling region                */
  puts ("\033(B\033)B");  /* G0 and G1 back to US ASCII                  */
  putch (0x0F);           /* SI: GL = G0                                 */
  puts ("\033[0m");       /* default rendition                           */

  /* Tab stops every eight columns, which neither DECSTR nor CSI touches. */
  for (i = 0; i < 12; i++)
    {
      ((UBYTE *)&v) [i] = 0;
    }

  if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v) == 0 && v.cols >= 20)
    {
      cols = v.cols;
    }

  puts ("\033[3g"); /* clear every tab stop */

  for (col = 9; col <= cols; col += 8)
    {
      puts ("\033[");
      putu ((ULONG)col);
      putch ('G');
      puts ("\033H"); /* HTS */
    }

  puts ("\033[2J\033[H"); /* clear and home */

  /* The cursor shape and blink are not reachable from the byte stream. */
  for (i = 0; i < (int)sizeof c; i++)
    {
      ((UBYTE *)&c) [i] = 0;
    }

  c.flags = VIDC_SET_SHAPE | VIDC_SET_VISIBLE | VIDC_SET_BLINK;
  c.shape = VIDC_SHAPE_UNDERLINE;
  c.visible = 1;
  c.blink = 1;
  (void)bdos (BDOS_VID_CURSOR, (LONG)(ULONG)&c);

  puts ("Console reset.\r\n");
}

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: TERMTEST [-a] [-k] [-p] [-r] [-5]\r\n");
  puts ("  (no args)  run the display tests, one screen at a time\r\n");
  puts ("  -5         keyboard test in VT52 mode\r\n");
  puts ("  -a         run the display tests and then the keyboard test\r\n");
  puts ("  -k         run only the keyboard test\r\n");
  puts ("  -p         keyboard test in application cursor / keypad mode\r\n");
  puts ("  -r         reset the console to a sane state and exit\r\n");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  unsigned tlen, i;
  int keys_only = 0;
  int keys_too = 0;
  int app_mode = 0;
  int vt52_mode = 0;
  int do_reset = 0;

  tlen = CMD_TAIL [0];

  for (i = 0; i < tlen && i < 126; i++)
    {
      char c = (char)CMD_TAIL [1 + i];

      if (c == ' ' || c == '\t' || c == '-' || c == '/')
        {
          continue;
        }

      if (c >= 'a' && c <= 'z')
        {
          c = (char)(c - 32);
        }

      if (c == 'K')
        {
          keys_only = 1;
        }
      else if (c == 'A')
        {
          keys_too = 1;
        }
      else if (c == 'P')
        {
          app_mode = 1;
          keys_only = 1;
        }
      else if (c == '5')
        {
          vt52_mode = 1;
          keys_only = 1;
        }
      else if (c == 'R')
        {
          do_reset = 1;
        }
      else
        {
          puts ("\r\n");
          usage ();
          (void)bdos (0, 0);

          return;
        }
    }

  if (do_reset)
    {
      reset_console ();
      (void)bdos (0, 0);

      return;
    }

  if (!keys_only)
    {
      test_sgr ();
      (void)anykey ("\r\nPress any key for the DRI extensions...");

      test_dri ();
      (void)anykey ("\r\nPress any key for VT52 mode...");
      puts ("\033e"); /* the cursor comes back */

      test_vt52 ();
      (void)anykey ("\r\nPress any key for the editing tests...");

      test_edit ();
      (void)anykey ("\r\nPress any key for DEC graphics...");

      test_gfx ();
      (void)anykey ("\r\nPress any key for the wrap test...");

      test_wrap ();

      if (!keys_too)
        {
          (void)anykey ("\r\nPress any key to finish...");
          cls ();
          (void)bdos (0, 0);

          return;
        }

      (void)anykey ("\r\nPress any key for the keyboard test...");
    }

  if (app_mode)
    {
      puts ("\033[?1h\033="); /* DECCKM and DECKPAM */
    }

  if (vt52_mode)
    {
      puts ("\033[?2l"); /* DECANM reset */
    }

  test_keys ();

  /*
   * Leave the console the way it was found, whatever the program did to it:
   * ANSI mode, numeric keypad, normal cursor keys.
   */

  puts ("\033<\033[?1l\033>");
  sgr0 ();
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
