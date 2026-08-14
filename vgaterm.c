/*
 * CP/M-386
 * Copyright (c) 2025-2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 48f74b44-91a6-11f1-82ea-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * NOTE: This code is a new version of my old VT102/VT52 emulation that
 * was ported to CP/M-386 and re-commented using AI/LLM-assistance.
 */

/*****************************************************************************/

/* vgaterm.c - VT102 / VT52 / DRI terminal emulation over the VGA text plane */

/*****************************************************************************/

/*
 * Dialects
 * --------
 *
 * The console powers up in ANSI mode, which is where a real VT102 starts
 * (DECANM set).  ESC [ ? 2 l selects the VT52 dialect and ESC < is the only
 * way back out of it, exactly as on the hardware: in VT52 mode ESC [ is not
 * a control sequence introducer at all.
 *
 * The Digital Research extensions are active in both dialects.  Every one of
 * them is ESC followed by a lowercase letter, a digit, '/' or ':'.  A VT52
 * uses none of those, so there is nothing to collide with there at all; a
 * VT102 in ANSI mode defines exactly one of them, ESC c (RIS), and that is
 * the only overlap in the whole set.  DRI wins it: ESC c sets the background
 * colour in both dialects and RIS is not implemented, because a console that
 * could only be coloured in one of its two modes would be no use, and
 * because RIS is something people type at terminals rather than something
 * CP/M programs emit.  ESC [ ! p (DECSTR) still performs a soft reset.
 *
 * The VT52 uppercase codes are a different matter: D, E, H, M and N are IND,
 * NEL, HTS, RI and SS2 to a VT102, so those five follow the dialect.  The
 * rest (A, B, C, F, G, I, J, K, L, Y) are undefined on a VT102 and are
 * honoured in both.
 *
 * Sequence table
 * --------------
 *
 * C0 controls (both)
 *   BEL  07     ignored (there is no speaker driver)
 *   BS   08     cursor left, stops at column 0; cancels a pending wrap
 *   HT   09     to the next tab stop
 *   LF   0A     index; also a carriage return while LNM is set (see below)
 *   VT   0B     index
 *   FF   0C     index
 *   CR   0D     column 0
 *   SO   0E     invoke G1 into GL
 *   SI   0F     invoke G0 into GL
 *   CAN  18     abandon the sequence in progress
 *   SUB  1A     abandon the sequence in progress
 *   ESC  1B     start a sequence
 *   DEL  7F     ignored
 *
 * VT52, both dialects (undefined on a VT102, so no collision)
 *   ESC A       cursor up                        ESC B   cursor down
 *   ESC C       cursor right                     ESC I   reverse index
 *   ESC J       erase to end of screen           ESC K   erase to end of line
 *   ESC L       insert line (DRI)                ESC Y r c  cursor address
 *   ESC F       select the DEC graphics set      ESC G   select ASCII
 *
 * VT52 dialect only (these are VT102 codes in ANSI mode)
 *   ESC D       cursor left                      ESC E   clear screen + home
 *   ESC H       home                             ESC M   delete line (DRI)
 *   ESC N       delete character (DRI)           ESC Z   identify: ESC / Z
 *   ESC ] V W X ^ _   VT52 print controls: accepted, no printer
 *
 * DRI DOS-Plus extensions, both dialects
 *   ESC 0       status line off                  ESC 1   status line on
 *               (accepted; this console has no status line)
 *   ESC a m     select screen mode m             (accepted, ignored)
 *   ESC b c     set foreground colour c         ESC c c   set background c
 *   ESC d h l   redirect CONIN                   ESC g h l   redirect AUXOUT
 *   ESC h h l   redirect LST                     (all accepted, ignored)
 *   ESC e       cursor on                        ESC f   cursor off
 *   ESC j       save cursor position             ESC k   restore cursor
 *   ESC l       erase the whole line             ESC o   erase up to cursor
 *   ESC m       cursor on                        ESC n   cursor off
 *   ESC p       black on white                   ESC q   white on black
 *   ESC r       bright text on                   ESC u   bright text off
 *   ESC s       flashing text on                 ESC t   flashing text off
 *   ESC v       wrap at end of line              ESC w   do not wrap
 *   ESC x       MONO mode                        ESC y   CO80 mode
 *               (accepted; use TEXTMODE / BDOS 231 to change the mode)
 *   ESC / bh bl set border / CGA palette         (accepted, ignored)
 *   ESC : sc s  redefine the key with scancode sc to the NUL-terminated s
 *
 * VT102 ESC-space, ANSI mode
 *   ESC D  IND      ESC E  NEL      ESC H  HTS      ESC M  RI
 *   ESC N  SS2      ESC O  SS3      ESC Z  DECID
 *   ESC ( ) * + F   designate a character set into G0..G3
 *   ESC # 8         DECALN (screen alignment pattern)
 *   ESC \          ST
 *
 * VT102 ESC-space, both dialects
 *   ESC 7  DECSC    ESC 8  DECRC    ESC =  DECKPAM  ESC >  DECKPNM
 *   ESC <  exit VT52 mode
 *
 * CSI, ANSI mode (in VT52 mode a CSI is swallowed and discarded)
 *   @ ICH   A CUU   B CUD   C CUF   D CUB   E CNL   F CPL   G CHA
 *   H CUP   I CHT   J ED    K EL    L IL    M DL    P DCH   S SU
 *   T SD    X ECH   Z CBT   ` HPA   a HPR   c DA    d VPA   e VPR
 *   f HVP   g TBC   h SM    l RM    m SGR   n DSR   r DECSTBM
 *   s save cursor (ANSI.SYS)         u restore cursor (ANSI.SYS)
 *   x DECREQTPARM                    ! p DECSTR (soft reset)
 *   ? h / ? l  DECSET / DECRST: 1 DECCKM, 2 DECANM, 3 DECCOLM, 5 DECSCNM,
 *              6 DECOM, 7 DECAWM, 25 DECTCEM (others accepted, ignored)
 *   h / l      4 IRM, 20 LNM
 *
 * ESC ] (OSC), ESC P (DCS), ESC ^ (PM) and ESC _ (APC) strings are swallowed
 * up to ST or BEL rather than being painted as glyphs.
 *
 * Deliberate departures from a real VT102
 * ---------------------------------------
 *
 *   LNM is set at power on rather than reset.  CP/M-386 writes bare LF from
 *   the BDOS, from MORE and from STAT, and the serial console has always
 *   added the carriage return itself.  ESC [ 20 l gives strict behaviour.
 *
 *   A CSI arriving in VT52 mode is discarded instead of having its
 *   parameters printed as text.
 *
 *   ESC c is the DRI "set background colour" sequence rather than RIS, in
 *   both dialects; see the note above.
 *
 *   Underline (SGR 4) is accepted but not rendered: a colour text adapter
 *   has no underline attribute.
 *
 *   The answer to DA and DECID is that of a VT102 (ESC [ ? 6 c).
 */

/*****************************************************************************/

#include "kbd.h"
#include "vgacon.h"
#include "vgaterm.h"

/*****************************************************************************/

#define VT_MAXCOLS 256          /* tab bitmap width; VGA text tops out well
                                   below this                              */
#define VT_TABBYTES (VT_MAXCOLS / 8)
#define VT_PARMS 16
#define VT_REPLY 48
#define VT_KEYDEF 24

/*****************************************************************************/

/* Parser states. */

enum vt_state
{
  VS_GROUND,
  VS_ESC,
  VS_CSI,      /* ESC [, collecting private/parameter/intermediate bytes  */
  VS_CSI_IGN,  /* ESC [ in VT52 mode: collected and thrown away           */
  VS_STRING,   /* ESC ] / P / ^ / _, swallowing up to ST or BEL           */
  VS_STRING_E, /* saw ESC inside a string; a following \ ends it          */
  VS_CHARSET,  /* ESC ( ) * +, awaiting the designator                    */
  VS_HASH,     /* ESC #, awaiting the final                               */
  VS_VT52_Y1,  /* ESC Y, awaiting the row byte                            */
  VS_VT52_Y2,  /* ESC Y r, awaiting the column byte                       */
  VS_DRI_FG,   /* ESC b, awaiting the colour                              */
  VS_DRI_BG,   /* ESC c, awaiting the colour                              */
  VS_DRI_MODE, /* ESC a, awaiting the mode                                */
  VS_SKIP2,    /* ESC d/g/h and ESC /, swallowing two parameter bytes     */
  VS_KEYDEF_SC,/* ESC :, awaiting the scancode                            */
  VS_KEYDEF_S  /* ESC : sc, collecting the NUL-terminated replacement     */
};

/*****************************************************************************/

static struct
{
  unsigned char inited;

  /* Geometry we last synchronised with; a mode set changes it under us. */
  unsigned cols;
  unsigned rows;

  /* Logical cursor.  col may equal cols only while wrap_pend is set. */
  unsigned row;
  unsigned col;
  unsigned char wrap_pend;

  /* Scrolling region, inclusive, in absolute rows. */
  unsigned top;
  unsigned bot;

  /* Rendition. */
  unsigned char fg;
  unsigned char bg;
  unsigned char bold;
  unsigned char blink;
  unsigned char reverse;
  unsigned char conceal;
  unsigned char underline;

  /* Modes. */
  unsigned char vt52;
  unsigned char autowrap;
  unsigned char origin;
  unsigned char insert;
  unsigned char newline;
  unsigned char appcursor;
  unsigned char appkeypad;
  unsigned char revscreen;
  unsigned char cursor_on;

  /* Character sets: g [0..3] hold designators, gl selects the active one. */
  unsigned char g [4];
  unsigned char gl;
  unsigned char ss_set;   /* G-set a single shift selects */
  unsigned char ss;

  /* DECSC / DECRC and the ANSI.SYS ESC[s / ESC[u pair. */
  struct
  {
    unsigned char valid;
    unsigned row;
    unsigned col;
    unsigned char fg;
    unsigned char bg;
    unsigned char bold;
    unsigned char blink;
    unsigned char reverse;
    unsigned char conceal;
    unsigned char underline;
    unsigned char origin;
    unsigned char g [4];
    unsigned char gl;
  } save;

  /* DRI ESC j / ESC k keep their own slot, per the DRI console. */
  struct
  {
    unsigned char valid;
    unsigned row;
    unsigned col;
  } dri_save;

  unsigned char tabs [VT_TABBYTES];

  /* Parser. */
  enum vt_state state;
  unsigned parm [VT_PARMS];
  unsigned char nparm;
  unsigned char priv;   /* '?', '>', '<' or '=' */
  unsigned char inter;  /* ' ', '!', '"', '$' ... */
  unsigned char scratch;
  unsigned char keydef_sc;
  unsigned char keydef_len;
  char keydef [VT_KEYDEF];

  /* Reply FIFO. */
  unsigned char reply [VT_REPLY];
  unsigned char rhead;
  unsigned char rtail;
} T;

/*****************************************************************************/

/*
 * ANSI SGR colour order (black red green yellow blue magenta cyan white) to
 * the IBM attribute order (black blue green cyan red magenta brown white).
 */

static const unsigned char ansi_to_pc [8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

/*
 * DEC Special Graphics (charset '0') 0x5F..0x7E rendered in code page 437.
 * The box drawing, arrows and blocks are exact; the three scan line rulings
 * DEC draws at different heights, "not equal" and the pound sign are the
 * closest glyphs the ROM font has.
 */

static const unsigned char dec_gfx [32] = {
  0x20, 0x04, 0xB1, 0x09, 0x0C, 0x0D, 0x0A, 0xF8, /* _ ` a b c d e f */
  0xF1, 0x20, 0x0B, 0xD9, 0xBF, 0xDA, 0xC0, 0xC5, /* g h i j k l m n */
  0x2D, 0x2D, 0xC4, 0x2D, 0x5F, 0xC3, 0xB4, 0xC1, /* o p q r s t u v */
  0xC2, 0xB3, 0xF3, 0xF2, 0xE3, 0xF7, 0x9C, 0xFA  /* w x y z { | } ~ */
};

/*****************************************************************************/

/* Reply queue.  Silently drops when full; a wedged reader must not block. */

static void
vt_reply_char (unsigned char c)
{
  unsigned char next = (unsigned char)((T.rtail + 1u) % VT_REPLY);

  if (next == T.rhead)
    {
      return;
    }

  T.reply [T.rtail] = c;
  T.rtail = next;
}

/*****************************************************************************/

static void
vt_reply (const char *s)
{
  while (*s)
    {
      vt_reply_char ((unsigned char)*s++);
    }
}

/*****************************************************************************/

static void
vt_reply_num (unsigned n)
{
  char b [12];
  int i = 0;

  if (n == 0)
    {
      vt_reply_char ('0');

      return;
    }

  while (n && i < (int)sizeof b)
    {
      b [i++] = (char)('0' + (n % 10u));
      n /= 10u;
    }

  while (i)
    {
      vt_reply_char ((unsigned char)b [--i]);
    }
}

/*****************************************************************************/

int
vgaterm_reply_avail (void)
{
  return T.rhead != T.rtail;
}

/*****************************************************************************/

int
vgaterm_reply_get (void)
{
  int c;

  if (T.rhead == T.rtail)
    {
      return -1;
    }

  c = (int)T.reply [T.rhead];
  T.rhead = (unsigned char)((T.rhead + 1u) % VT_REPLY);

  return c;
}

/*****************************************************************************/

/* Current VGA attribute byte from the rendition flags. */

static unsigned char
vt_attr (void)
{
  unsigned char fg = (unsigned char)(T.fg & 0x0F);
  unsigned char bg = (unsigned char)(T.bg & 0x0F);
  unsigned char a;

  if (T.bold)
    {
      fg |= 0x08;
    }

  /*
   * DECSCNM inverts the whole screen, so it composes with per-character
   * reverse rather than overriding it.  There is no underline attribute on
   * a colour text adapter, so SGR 4 is accepted and left unrendered.
   */

  if ((T.reverse ^ T.revscreen) & 1u)
    {
      unsigned char t = fg;

      fg = bg;
      bg = t;
    }

  if (T.conceal)
    {
      fg = bg;
    }

  a = (unsigned char)((fg & 0x0F) | (unsigned char)((bg & 0x0F) << 4));

  if (T.blink)
    {
      a |= 0x80;
    }

  return a;
}

/*****************************************************************************/

static void
vt_tab_defaults (void)
{
  unsigned i;

  for (i = 0; i < VT_TABBYTES; i++)
    {
      T.tabs [i] = 0;
    }

  for (i = 8; i < VT_MAXCOLS; i += 8)
    {
      T.tabs [i >> 3] = (unsigned char)(T.tabs [i >> 3] | (1u << (i & 7u)));
    }
}

/*****************************************************************************/

static int
vt_tab_at (unsigned col)
{
  if (col >= VT_MAXCOLS)
    {
      return 0;
    }

  return (T.tabs [col >> 3] & (1u << (col & 7u))) ? 1 : 0;
}

/*****************************************************************************/

static void
vt_tab_set (unsigned col, int on)
{
  if (col >= VT_MAXCOLS)
    {
      return;
    }

  if (on)
    {
      T.tabs [col >> 3] =
        (unsigned char)(T.tabs [col >> 3] | (1u << (col & 7u)));
    }
  else
    {
      T.tabs [col >> 3]
          = (unsigned char)(T.tabs [col >> 3] & ~(1u << (col & 7u)));
    }
}

/*****************************************************************************/

/* Push the logical cursor at the hardware.  A pending wrap stays visible on
   the last column, which is where a real VT102 leaves it. */

static void
vt_sync (void)
{
  unsigned c = T.col;

  if (T.cols && c >= T.cols)
    {
      c = T.cols - 1;
    }

  vgacon_cursor (T.row, c);
}

/*****************************************************************************/

static void
vt_defaults (void)
{
  unsigned i;

  T.cols = vgacon_cols ();
  T.rows = vgacon_rows ();

  T.row = 0;
  T.col = 0;
  T.wrap_pend = 0;
  T.top = 0;
  T.bot = T.rows ? T.rows - 1 : 0;

  T.fg = VGACON_DEF_ATTR & 0x07;
  T.bg = (VGACON_DEF_ATTR >> 4) & 0x07;
  T.bold = 0;
  T.blink = 0;
  T.reverse = 0;
  T.conceal = 0;
  T.underline = 0;

  /*
   * ANSI mode, which is where a real VT102 powers up (DECANM set).  A
   * program selects the VT52 dialect with ESC [ ? 2 l and comes back with
   * ESC <.  The DRI extensions do not need either: they are active in both.
   */

  T.vt52 = 0;
  T.autowrap = 1;
  T.origin = 0;
  T.insert = 0;

  /*
   * LNM set is the one deliberate departure from the VT102 power-on state,
   * which has it reset.  CP/M-386 writes bare LF from the BDOS, from MORE
   * and from STAT, and the serial console has always supplied the carriage
   * return itself; a VGA console that did not would stair-step.  ESC [ 20 l
   * gives the strict behaviour.
   */

  T.newline = 1;
  T.appcursor = 0;
  T.appkeypad = 0;
  T.revscreen = 0;
  T.cursor_on = 1;

  for (i = 0; i < 4; i++)
    {
      T.g [i] = 'B';
    }

  T.gl = 0;
  T.ss_set = 0;
  T.ss = 0;

  T.save.valid = 0;
  T.dri_save.valid = 0;

  vt_tab_defaults ();

  T.state = VS_GROUND;
  T.nparm = 0;
  T.priv = 0;
  T.inter = 0;

  vgacon_set_attr (vt_attr ());
  vgacon_cursor_visible (1);
  T.inited = 1;
}

/*****************************************************************************/

void
vgaterm_soft_reset (void)
{
  vt_defaults ();
  vt_sync ();
}

/*****************************************************************************/

void
vgaterm_reset (void)
{
  vt_defaults ();
  vgacon_clear ();
  vt_sync ();
}

/*****************************************************************************/

/*
 * A BDOS 231 mode set or a VGAFONT reload can change the geometry underneath
 * us.  Rather than have vidmode call in, notice it here: the margins and the
 * cursor have to be brought back inside the new screen before anything else
 * touches the plane.
 */

static void
vt_sync_geometry (void)
{
  unsigned cols = vgacon_cols ();
  unsigned rows = vgacon_rows ();

  if (!T.inited)
    {
      vt_defaults ();

      return;
    }

  if (cols == T.cols && rows == T.rows)
    {
      return;
    }

  T.cols = cols;
  T.rows = rows;
  T.top = 0;
  T.bot = rows ? rows - 1 : 0;
  T.wrap_pend = 0;

  if (rows && T.row >= rows)
    {
      T.row = rows - 1;
    }

  if (cols && T.col >= cols)
    {
      T.col = cols - 1;
    }

  vt_tab_defaults ();
}

/*****************************************************************************/

/* Cursor limits.  Origin mode confines row addressing to the region. */

static unsigned
vt_row_min (void)
{
  return T.origin ? T.top : 0;
}

/*****************************************************************************/

static unsigned
vt_row_max (void)
{
  if (T.origin)
    {
      return T.bot;
    }

  return T.rows ? T.rows - 1 : 0;
}

/*****************************************************************************/

static void
vt_goto (unsigned row, unsigned col)
{
  unsigned lo = vt_row_min ();
  unsigned hi = vt_row_max ();

  if (row < lo)
    {
      row = lo;
    }

  if (row > hi)
    {
      row = hi;
    }

  if (T.cols && col >= T.cols)
    {
      col = T.cols - 1;
    }

  T.row = row;
  T.col = col;
  T.wrap_pend = 0;
}

/*****************************************************************************/

/* IND: down one, scrolling the region when already on the bottom margin. */

static void
vt_index (void)
{
  if (T.row == T.bot)
    {
      vgacon_scroll_region (T.top, T.bot, 1, vt_attr ());
    }
  else if (T.rows && T.row + 1 < T.rows)
    {
      T.row++;
    }
}

/*****************************************************************************/

/* RI: up one, scrolling the region when already on the top margin. */

static void
vt_rindex (void)
{
  if (T.row == T.top)
    {
      vgacon_scroll_region (T.top, T.bot, -1, vt_attr ());
    }
  else if (T.row > 0)
    {
      T.row--;
    }
}

/*****************************************************************************/

static void
vt_cr (void)
{
  T.col = 0;
  T.wrap_pend = 0;
}

/*****************************************************************************/

static void
vt_tab (void)
{
  unsigned c = T.col;

  T.wrap_pend = 0;

  if (!T.cols)
    {
      return;
    }

  do
    {
      c++;
    }
  while (c + 1 < T.cols && !vt_tab_at (c));

  T.col = (c < T.cols) ? c : T.cols - 1;
}

/*****************************************************************************/

static void
vt_backtab (unsigned n)
{
  while (n--)
    {
      unsigned c = T.col;

      if (c == 0)
        {
          break;
        }

      do
        {
          c--;
        }
      while (c > 0 && !vt_tab_at (c));

      T.col = c;
    }

  T.wrap_pend = 0;
}

/*****************************************************************************/

/* Translate through the active G-set. */

static unsigned char
vt_map (unsigned char c)
{
  unsigned char set = T.g [T.ss ? T.ss_set : T.gl];

  /*
   * Only the DEC Special Graphics set is remapped.  'A' (UK) differs from
   * 'B' (US ASCII) in one glyph, and code page 437 has no pound sign at
   * 0x23, so the two are treated alike.
   */

  if (set == '0' && c >= 0x5F && c <= 0x7E)
    {
      return dec_gfx [c - 0x5F];
    }

  return c;
}

/*****************************************************************************/

static void
vt_putglyph (unsigned char c)
{
  unsigned char attr = vt_attr ();

  if (!T.cols || !T.rows)
    {
      return;
    }

  if (T.wrap_pend)
    {
      vt_cr ();
      vt_index ();
    }

  if (T.col >= T.cols)
    {
      T.col = T.cols - 1;
    }

  if (T.insert)
    {
      vgacon_ins_chars (T.row, T.col, 1, attr);
    }

  vgacon_cell (T.row, T.col, vt_map (c), attr);

  if (T.col + 1 >= T.cols)
    {
      /*
       * VT100 deferred wrap: the cursor stays on the last column and only
       * moves when the next printable arrives.  Without this, a program that
       * fills the bottom line scrolls the screen one line too early.
       */

      T.wrap_pend = (unsigned char)(T.autowrap ? 1 : 0);
    }
  else
    {
      T.col++;
    }

  if (T.ss)
    {
      T.ss = 0;
    }
}

/*****************************************************************************/

/* Erase helpers.  Erasure paints the current attribute, so a program that
   sets a background colour and clears gets that colour, as ANSI.SYS and the
   DRI console both do. */

static void
vt_erase_line (unsigned row, unsigned c0, unsigned c1)
{
  if (!T.cols)
    {
      return;
    }

  if (c1 >= T.cols)
    {
      c1 = T.cols - 1;
    }

  if (c0 > c1)
    {
      return;
    }

  vgacon_fill (row, c0, row, c1, ' ', vt_attr ());
}

/*****************************************************************************/

static void
vt_erase_display (int mode)
{
  if (!T.rows || !T.cols)
    {
      return;
    }

  switch (mode)
    {
    case 0: /* cursor to end of screen */
      vt_erase_line (T.row, T.col, T.cols - 1);

      if (T.row + 1 < T.rows)
        {
          vgacon_fill (T.row + 1, 0, T.rows - 1, T.cols - 1, ' ', vt_attr ());
        }

      break;

    case 1: /* start of screen to cursor */
      if (T.row > 0)
        {
          vgacon_fill (0, 0, T.row - 1, T.cols - 1, ' ', vt_attr ());
        }

      vt_erase_line (T.row, 0, T.col);

      break;

    default: /* 2 and 3: the whole screen */
      vgacon_fill (0, 0, T.rows - 1, T.cols - 1, ' ', vt_attr ());

      break;
    }

  T.wrap_pend = 0;
}

/*****************************************************************************/

/*
 * DECSCNM.  The attribute nibbles of every cell already on screen have to be
 * swapped, otherwise only text written after the mode change would invert.
 */

static void
vt_flip_screen (void)
{
  unsigned r, c;

  for (r = 0; r < T.rows; r++)
    {
      for (c = 0; c < T.cols; c++)
        {
          unsigned char ch, a;

          vgacon_read_cell (r, c, &ch, &a);
          a = (unsigned char)(((a & 0x0F) << 4) | ((a >> 4) & 0x0F));
          vgacon_cell (r, c, ch, a);
        }
    }
}

/*****************************************************************************/

static void
vt_save_cursor (void)
{
  unsigned i;

  T.save.valid = 1;
  T.save.row = T.row;
  T.save.col = T.col;
  T.save.fg = T.fg;
  T.save.bg = T.bg;
  T.save.bold = T.bold;
  T.save.blink = T.blink;
  T.save.reverse = T.reverse;
  T.save.conceal = T.conceal;
  T.save.underline = T.underline;
  T.save.origin = T.origin;
  T.save.gl = T.gl;

  for (i = 0; i < 4; i++)
    {
      T.save.g [i] = T.g [i];
    }
}

/*****************************************************************************/

static void
vt_restore_cursor (void)
{
  unsigned i;

  if (!T.save.valid)
    {
      vt_goto (0, 0);

      return;
    }

  T.fg = T.save.fg;
  T.bg = T.save.bg;
  T.bold = T.save.bold;
  T.blink = T.save.blink;
  T.reverse = T.save.reverse;
  T.conceal = T.save.conceal;
  T.underline = T.save.underline;
  T.origin = T.save.origin;
  T.gl = T.save.gl;

  for (i = 0; i < 4; i++)
    {
      T.g [i] = T.save.g [i];
    }

  vgacon_set_attr (vt_attr ());
  vt_goto (T.save.row, T.save.col);
}

/*****************************************************************************/

/* SGR. */

static void
vt_sgr (unsigned p)
{
  switch (p)
    {
    case 0:
      T.fg = VGACON_DEF_ATTR & 0x07;
      T.bg = (VGACON_DEF_ATTR >> 4) & 0x07;
      T.bold = 0;
      T.blink = 0;
      T.reverse = 0;
      T.conceal = 0;
      T.underline = 0;

      break;

    case 1:
      T.bold = 1;

      break;

    case 2: /* faint: the nearest thing is "not bright" */
      T.bold = 0;

      break;

    case 4:
      T.underline = 1;

      break;

    case 5:
    case 6:
      T.blink = 1;

      break;

    case 7:
      T.reverse = 1;

      break;

    case 8:
      T.conceal = 1;

      break;

    case 21:
    case 22:
      T.bold = 0;

      break;

    case 24:
      T.underline = 0;

      break;

    case 25:
      T.blink = 0;

      break;

    case 27:
      T.reverse = 0;

      break;

    case 28:
      T.conceal = 0;

      break;

    case 39:
      T.fg = VGACON_DEF_ATTR & 0x07;
      T.bold = 0;

      break;

    case 49:
      T.bg = (VGACON_DEF_ATTR >> 4) & 0x07;

      break;

    default:
      if (p >= 30 && p <= 37)
        {
          T.fg = ansi_to_pc [p - 30];
        }
      else if (p >= 40 && p <= 47)
        {
          T.bg = ansi_to_pc [p - 40];
        }
      else if (p >= 90 && p <= 97)
        {
          T.fg = (unsigned char)(ansi_to_pc [p - 90] | 0x08);
        }
      else if (p >= 100 && p <= 107)
        {
          T.bg = (unsigned char)(ansi_to_pc [p - 100] | 0x08);
        }

      break;
    }

  vgacon_set_attr (vt_attr ());
}

/*****************************************************************************/

/* DECSET / DECRST. */

static void
vt_dec_mode (unsigned p, int set)
{
  switch (p)
    {
    case 1: /* DECCKM */
      T.appcursor = (unsigned char)(set ? 1 : 0);

      break;

    case 2: /* DECANM */
      T.vt52 = (unsigned char)(set ? 0 : 1);

      break;

    case 3: /* DECCOLM: the width is the BIOS mode's, but the side effect
               (erase and home) is what programs rely on */
      vt_erase_display (2);
      T.top = 0;
      T.bot = T.rows ? T.rows - 1 : 0;
      vt_goto (0, 0);

      break;

    case 5: /* DECSCNM */
      if ((unsigned char)(set ? 1 : 0) != T.revscreen)
        {
          T.revscreen = (unsigned char)(set ? 1 : 0);
          vt_flip_screen ();
          vgacon_set_attr (vt_attr ());
        }

      break;

    case 6: /* DECOM */
      T.origin = (unsigned char)(set ? 1 : 0);
      vt_goto (vt_row_min (), 0);

      break;

    case 7: /* DECAWM */
      T.autowrap = (unsigned char)(set ? 1 : 0);

      if (!T.autowrap)
        {
          T.wrap_pend = 0;
        }

      break;

    case 25: /* DECTCEM */
      T.cursor_on = (unsigned char)(set ? 1 : 0);
      vgacon_cursor_visible (T.cursor_on);

      break;

    default: /* 4 DECSCLM, 8 DECARM, 18/19 print extent, ... : accepted */
      break;
    }
}

/*****************************************************************************/

/* SM / RM (no private marker). */

static void
vt_ansi_mode (unsigned p, int set)
{
  switch (p)
    {
    case 4: /* IRM */
      T.insert = (unsigned char)(set ? 1 : 0);

      break;

    case 20: /* LNM */
      T.newline = (unsigned char)(set ? 1 : 0);

      break;

    default:
      break;
    }
}

/*****************************************************************************/

/* Soft reset (DECSTR / ESC[!p): modes and margins, but not the screen. */

static void
vt_soft_reset (void)
{
  unsigned i;

  T.origin = 0;
  T.insert = 0;
  T.appcursor = 0;
  T.appkeypad = 0;
  T.wrap_pend = 0;
  T.top = 0;
  T.bot = T.rows ? T.rows - 1 : 0;
  T.cursor_on = 1;
  T.save.valid = 0;

  for (i = 0; i < 4; i++)
    {
      T.g [i] = 'B';
    }

  T.gl = 0;
  T.ss = 0;

  /*
   * DECSTR resets DECAWM on a real VT220.  It is left set here: a console
   * that silently stopped wrapping because a program asked for a soft reset
   * would be a poor default, and wrapping is what CP/M-386 wants.
   */

  T.autowrap = 1;

  if (T.revscreen)
    {
      T.revscreen = 0;
      vt_flip_screen ();
    }

  vt_sgr (0);
  vgacon_cursor_visible (1);
}

/*****************************************************************************/

static unsigned
vt_parm (unsigned i, unsigned dflt)
{
  if (i >= T.nparm || T.parm [i] == 0)
    {
      return dflt;
    }

  return T.parm [i];
}

/*****************************************************************************/

static void
vt_csi_dispatch (unsigned char final)
{
  unsigned n = vt_parm (0, 1);
  unsigned i;

  switch (final)
    {
    case '@': /* ICH */
      vgacon_ins_chars (T.row, T.col, n, vt_attr ());

      break;

    case 'A': /* CUU */
      {
        unsigned lo = (T.row >= T.top) ? T.top : 0;

        T.row = (T.row > lo + n) ? T.row - n : lo;
        T.wrap_pend = 0;
      }

      break;

    case 'B': /* CUD */
      {
        unsigned hi = (T.row <= T.bot) ? T.bot : (T.rows ? T.rows - 1 : 0);

        T.row = (T.row + n < hi) ? T.row + n : hi;
        T.wrap_pend = 0;
      }

      break;

    case 'C': /* CUF */
    case 'a': /* HPR */
      T.col = (T.col + n < T.cols) ? T.col + n : (T.cols ? T.cols - 1 : 0);
      T.wrap_pend = 0;

      break;

    case 'D': /* CUB */
      T.col = (T.col > n) ? T.col - n : 0;
      T.wrap_pend = 0;

      break;

    case 'E': /* CNL */
      T.col = 0;
      T.row = (T.row + n < T.bot) ? T.row + n : T.bot;
      T.wrap_pend = 0;

      break;

    case 'F': /* CPL */
      T.col = 0;
      T.row = (T.row > T.top + n) ? T.row - n : T.top;
      T.wrap_pend = 0;

      break;

    case 'G': /* CHA */
    case '`': /* HPA */
      vt_goto (T.row, n - 1);

      break;

    case 'H': /* CUP */
    case 'f': /* HVP */
      {
        unsigned r = vt_parm (0, 1) - 1;
        unsigned c = vt_parm (1, 1) - 1;

        vt_goto (T.origin ? T.top + r : r, c);
      }

      break;

    case 'I': /* CHT */
      for (i = 0; i < n; i++)
        {
          vt_tab ();
        }

      break;

    case 'J': /* ED */
      vt_erase_display ((int)vt_parm (0, 0));

      break;

    case 'K': /* EL */
      switch (vt_parm (0, 0))
        {
        case 1:
          vt_erase_line (T.row, 0, T.col);

          break;

        case 2:
          vt_erase_line (T.row, 0, T.cols ? T.cols - 1 : 0);

          break;

        default:
          vt_erase_line (T.row, T.col, T.cols ? T.cols - 1 : 0);

          break;
        }

      T.wrap_pend = 0;

      break;

    case 'L': /* IL */
      if (T.row >= T.top && T.row <= T.bot)
        {
          vgacon_scroll_region (T.row, T.bot, -(int)n, vt_attr ());
          T.col = 0;
          T.wrap_pend = 0;
        }

      break;

    case 'M': /* DL */
      if (T.row >= T.top && T.row <= T.bot)
        {
          vgacon_scroll_region (T.row, T.bot, (int)n, vt_attr ());
          T.col = 0;
          T.wrap_pend = 0;
        }

      break;

    case 'P': /* DCH */
      vgacon_del_chars (T.row, T.col, n, vt_attr ());

      break;

    case 'S': /* SU */
      vgacon_scroll_region (T.top, T.bot, (int)n, vt_attr ());

      break;

    case 'T': /* SD */
      vgacon_scroll_region (T.top, T.bot, -(int)n, vt_attr ());

      break;

    case 'X': /* ECH */
      if (T.cols)
        {
          unsigned last = T.col + n - 1;

          vt_erase_line (T.row, T.col, last);
        }

      break;

    case 'Z': /* CBT */
      vt_backtab (n);

      break;

    case 'c': /* DA */
      if (T.priv == 0 && vt_parm (0, 0) == 0)
        {
          vt_reply ("\033[?6c"); /* VT102 */
        }

      break;

    case 'd': /* VPA */
    case 'e': /* VPR */
      if (final == 'd')
        {
          vt_goto (T.origin ? T.top + n - 1 : n - 1, T.col);
        }
      else
        {
          vt_goto (T.row + n, T.col);
        }

      break;

    case 'g': /* TBC */
      if (vt_parm (0, 0) == 3)
        {
          for (i = 0; i < VT_TABBYTES; i++)
            {
              T.tabs [i] = 0;
            }
        }
      else
        {
          vt_tab_set (T.col, 0);
        }

      break;

    case 'h': /* SM / DECSET */
    case 'l': /* RM / DECRST */
      {
        int set = (final == 'h');

        if (T.nparm == 0)
          {
            break;
          }

        for (i = 0; i < T.nparm; i++)
          {
            if (T.priv == '?')
              {
                vt_dec_mode (T.parm [i], set);
              }
            else
              {
                vt_ansi_mode (T.parm [i], set);
              }
          }
      }

      break;

    case 'm': /* SGR */
      if (T.nparm == 0)
        {
          vt_sgr (0);
        }
      else
        {
          for (i = 0; i < T.nparm; i++)
            {
              /*
               * 38;5;n and 48;5;n select from the 256 colour cube.  Only the
               * bottom sixteen entries exist here, so the index is folded.
               */

              if ((T.parm [i] == 38 || T.parm [i] == 48) && i + 2 < T.nparm
                  && T.parm [i + 1] == 5)
                {
                  unsigned c = T.parm [i + 2] & 0x0F;

                  if (T.parm [i] == 38)
                    {
                      T.fg = (unsigned char)c;
                    }
                  else
                    {
                      T.bg = (unsigned char)c;
                    }

                  vgacon_set_attr (vt_attr ());
                  i += 2;

                  continue;
                }

              vt_sgr (T.parm [i]);
            }
        }

      break;

    case 'n': /* DSR */
      if (T.priv == 0)
        {
          if (vt_parm (0, 0) == 6)
            {
              unsigned r = T.origin ? T.row - T.top : T.row;

              vt_reply ("\033[");
              vt_reply_num (r + 1);
              vt_reply_char (';');
              vt_reply_num ((T.col < T.cols ? T.col : T.cols - 1) + 1);
              vt_reply_char ('R');
            }
          else if (vt_parm (0, 0) == 5)
            {
              vt_reply ("\033[0n");
            }
        }

      break;

    case 'p': /* DECSTR is ESC [ ! p */
      if (T.inter == '!')
        {
          vt_soft_reset ();
        }

      break;

    case 'r': /* DECSTBM */
      {
        unsigned top = vt_parm (0, 1) - 1;
        unsigned bot = vt_parm (1, T.rows) - 1;

        if (T.rows && bot >= T.rows)
          {
            bot = T.rows - 1;
          }

        if (top < bot)
          {
            T.top = top;
            T.bot = bot;
            vt_goto (vt_row_min (), 0);
          }
      }

      break;

    case 's': /* ANSI.SYS save cursor */
      vt_save_cursor ();

      break;

    case 'u': /* ANSI.SYS restore cursor */
      vt_restore_cursor ();

      break;

    case 'x': /* DECREQTPARM */
      /*
       * Only requests 0 and 1 are answered, and the report selector is the
       * request plus two, as on a VT102.  The rest of the report is the
       * fixed "no parity, 8 bits, 9600 baud, no multiplier" answer.
       */

      if (vt_parm (0, 0) < 2)
        {
          vt_reply ("\033[");
          vt_reply_num (vt_parm (0, 0) + 2);
          vt_reply (";1;1;120;120;1;0x");
        }

      break;

    default:
      break;
    }
}

/*****************************************************************************/

/* DRI colour setters.  The parameter is a raw PC colour index, not ASCII. */

static void
vt_dri_fg (unsigned char c)
{
  T.fg = (unsigned char)(c & 0x0F);
  T.bold = 0; /* the index already carries the intensity bit */
  vgacon_set_attr (vt_attr ());
}

/*****************************************************************************/

static void
vt_dri_bg (unsigned char c)
{
  T.bg = (unsigned char)(c & 0x0F);
  vgacon_set_attr (vt_attr ());
}

/*****************************************************************************/

/*
 * The DRI DOS-Plus extensions.  Every one of them is an ESC followed by a
 * lowercase letter, a digit, '/' or ':'.  A VT102 leaves all of those
 * undefined in ANSI mode apart from ESC c (RIS), so honouring them in both
 * dialects adds the DRI console's vocabulary without redefining anything a
 * conforming VT102 program can send.
 *
 * Returns 1 when the byte was one of them.
 */

static int
vt_esc_dri (unsigned char c)
{
  switch (c)
    {
    case '0': /* status line off - this console has none */
    case '1': /* status line on                          */
      return 1;

    case 'a': /* select screen mode; parameter swallowed */
      T.state = VS_DRI_MODE;

      return 1;

    case 'b': /* set foreground colour */
      T.state = VS_DRI_FG;

      return 1;

    /*
     * ESC c is the single code where the DRI set and the VT102 set collide:
     * VT102 ANSI mode calls it RIS.  DRI wins in both dialects here, because
     * a CP/M console that could only set the background colour in one of
     * them would be no use, and because RIS is a sequence users type at
     * terminals rather than one CP/M programs emit.  ESC [ ! p (DECSTR) is
     * still there for a soft reset, and BDOS 221 / CLS for a screen clear.
     */

    case 'c': /* set background colour */
      T.state = VS_DRI_BG;

      return 1;

    case 'd': /* redirect CONIN  */
    case 'g': /* redirect AUXOUT */
    case 'h': /* redirect LST    */
    case '/': /* int 10h AH=0Bh: set border / CGA palette */
      T.scratch = 2;
      T.state = VS_SKIP2;

      return 1;

    /*
     * ESC e and ESC f are CONOUT and AUXIN redirection in the DRI console
     * and cursor on / cursor off in the Amstrad CP/M Plus one.  Nothing
     * here can be redirected, so the cursor meaning is the useful one; the
     * documented DRI codes ESC m and ESC n do the same thing.
     */

    case 'e':
    case 'm':
      T.cursor_on = 1;
      vgacon_cursor_visible (1);

      return 1;

    case 'f':
    case 'n':
      T.cursor_on = 0;
      vgacon_cursor_visible (0);

      return 1;

    case 'j': /* save cursor position    */
      T.dri_save.valid = 1;
      T.dri_save.row = T.row;
      T.dri_save.col = T.col;

      return 1;

    case 'k': /* restore cursor position */
      if (T.dri_save.valid)
        {
          vt_goto (T.dri_save.row, T.dri_save.col);
        }

      return 1;

    case 'l': /* erase the whole line */
      vt_erase_line (T.row, 0, T.cols ? T.cols - 1 : 0);
      T.col = 0;
      T.wrap_pend = 0;

      return 1;

    case 'o': /* erase up to the cursor */
      vt_erase_line (T.row, 0, T.col);

      return 1;

    case 'p': /* black on white */
      T.fg = 0;
      T.bg = 7;
      T.bold = 0;
      T.reverse = 0;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 'q': /* white on black */
      T.fg = 7;
      T.bg = 0;
      T.bold = 0;
      T.reverse = 0;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 'r': /* bright on  */
      T.bold = 1;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 'u': /* bright off */
      T.bold = 0;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 's': /* flashing on  */
      T.blink = 1;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 't': /* flashing off */
      T.blink = 0;
      vgacon_set_attr (vt_attr ());

      return 1;

    case 'v': /* wrap on  */
      T.autowrap = 1;

      return 1;

    case 'w': /* wrap off */
      T.autowrap = 0;
      T.wrap_pend = 0;

      return 1;

    case 'x': /* MONO */
    case 'y': /* CO80 */
      /*
       * Accepted and ignored on purpose: the console mode belongs to BDOS
       * 231 / TEXTMODE, and a stray ESC x in a text file must not reprogram
       * the display.
       */

      return 1;

    case ':': /* redefine a function key */
      T.state = VS_KEYDEF_SC;

      return 1;

    default:
      break;
    }

  return 0;
}

/*****************************************************************************/

/*
 * The VT52 uppercase set.  These are shared with ANSI mode only where a
 * VT102 leaves the code undefined; D, E, H, M and N are excluded here
 * because ANSI mode owes them to IND, NEL, HTS, RI and SS2.
 */

static int
vt_esc_vt52_common (unsigned char c)
{
  switch (c)
    {
    case 'A': /* cursor up */
      if (T.row > 0)
        {
          T.row--;
        }

      T.wrap_pend = 0;

      return 1;

    case 'B': /* cursor down */
      if (T.rows && T.row + 1 < T.rows)
        {
          T.row++;
        }

      T.wrap_pend = 0;

      return 1;

    case 'C': /* cursor right */
      if (T.cols && T.col + 1 < T.cols)
        {
          T.col++;
        }

      T.wrap_pend = 0;

      return 1;

    case 'F': /* enter graphics mode */
      T.g [0] = '0';

      return 1;

    case 'G': /* exit graphics mode */
      T.g [0] = 'B';

      return 1;

    case 'I': /* reverse line feed */
      vt_rindex ();

      return 1;

    case 'J': /* erase to end of screen */
      vt_erase_display (0);

      return 1;

    case 'K': /* erase to end of line */
      vt_erase_line (T.row, T.col, T.cols ? T.cols - 1 : 0);

      return 1;

    case 'L': /* DRI insert line */
      if (T.row >= T.top && T.row <= T.bot)
        {
          vgacon_scroll_region (T.row, T.bot, -1, vt_attr ());
          T.col = 0;
        }

      return 1;

    case 'Y': /* direct cursor address */
      T.state = VS_VT52_Y1;

      return 1;

    default:
      break;
    }

  return 0;
}

/*****************************************************************************/

/*
 * VT52 mode (DECANM reset).  A real VT102 in this state understands only
 * the VT52 vocabulary: ESC [ is not a control sequence introducer and the
 * only way back to ANSI is ESC <.  That is reproduced here, except that an
 * ESC [ sequence is swallowed whole rather than having its parameters
 * printed - a stray ANSI sequence should not spray the screen.
 */

static void
vt_esc_vt52 (unsigned char c)
{
  if (vt_esc_dri (c))
    {
      return;
    }

  if (vt_esc_vt52_common (c))
    {
      return;
    }

  switch (c)
    {
    case 'D': /* cursor left */
      if (T.col > 0)
        {
          T.col--;
        }

      T.wrap_pend = 0;

      break;

    case 'E': /* clear screen and home */
      vt_erase_display (2);
      vt_goto (0, 0);

      break;

    case 'H': /* home */
      vt_goto (0, 0);

      break;

    case 'M': /* DRI delete line */
      if (T.row >= T.top && T.row <= T.bot)
        {
          vgacon_scroll_region (T.row, T.bot, 1, vt_attr ());
          T.col = 0;
        }

      break;

    case 'N': /* DRI delete character */
      vgacon_del_chars (T.row, T.col, 1, vt_attr ());

      break;

    case 'Z': /* identify: a VT52 answers ESC / Z */
      vt_reply ("\033/Z");

      break;

    case '[': /* not a CSI in VT52 mode; swallow it rather than print it */
      T.state = VS_CSI_IGN;

      break;

    case ']': /* VT52 print controls: there is no printer */
    case 'V':
    case 'W':
    case 'X':
    case '^':
    case '_':
      break;

    default:
      break;
    }
}

/*****************************************************************************/

/*
 * ANSI mode (DECANM set), which is where a VT102 powers up.  Everything a
 * VT102 defines keeps its standard meaning; the DRI extensions and the VT52
 * codes the VT102 leaves undefined ride along on top.
 */

static void
vt_esc_ansi (unsigned char c)
{
  switch (c)
    {
    case '[': /* CSI */
      T.state = VS_CSI;
      T.nparm = 0;
      T.priv = 0;
      T.inter = 0;
      T.parm [0] = 0;

      return;

    case ']': /* OSC */
    case 'P': /* DCS */
    case '^': /* PM  */
    case '_': /* APC */
      T.state = VS_STRING;

      return;

    case '(':
    case ')':
    case '*':
    case '+':
      T.scratch = (unsigned char)(c == '(' ? 0 : c == ')' ? 1
                                  : c == '*' ? 2 : 3);
      T.state = VS_CHARSET;

      return;

    case '#':
      T.state = VS_HASH;

      return;

    case '7': /* DECSC */
      vt_save_cursor ();

      return;

    case '8': /* DECRC */
      vt_restore_cursor ();

      return;

    case '\\': /* stray ST */
      return;

    case 'D': /* IND */
      vt_index ();

      return;

    case 'E': /* NEL */
      vt_cr ();
      vt_index ();

      return;

    case 'H': /* HTS */
      vt_tab_set (T.col, 1);

      return;

    case 'M': /* RI */
      vt_rindex ();

      return;

    case 'N': /* SS2 */
      T.ss_set = 2;
      T.ss = 1;

      return;

    case 'O': /* SS3 */
      T.ss_set = 3;
      T.ss = 1;

      return;

    case 'Z': /* DECID */
      vt_reply ("\033[?6c");

      return;

    default:
      break;
    }

  if (vt_esc_dri (c))
    {
      return;
    }

  (void)vt_esc_vt52_common (c);
}

/*****************************************************************************/

/*
 * ESC-space dispatch.  Sequences that need a parameter byte push the parser
 * into the matching state; everything else acts and returns to ground.
 */

static void
vt_esc_dispatch (unsigned char c)
{
  T.state = VS_GROUND;

  /* Recognised in both dialects. */
  switch (c)
    {
    case '<': /* leave VT52 mode; the only way out, as on a VT102 */
      T.vt52 = 0;

      return;

    case '=': /* DECKPAM / VT52 alternate keypad */
      T.appkeypad = 1;

      return;

    case '>': /* DECKPNM / VT52 numeric keypad */
      T.appkeypad = 0;

      return;

    default:
      break;
    }

  if (T.vt52)
    {
      vt_esc_vt52 (c);
    }
  else
    {
      vt_esc_ansi (c);
    }
}

/*****************************************************************************/

/* C0 controls, acted on wherever they appear (except inside a string). */

static int
vt_c0 (unsigned char c)
{
  switch (c)
    {
    case 0x07: /* BEL */
      return 1;

    case 0x08: /* BS */
      /*
       * A pending wrap means the cursor is still displayed on the last
       * column, so backspace has to cancel the wrap and then move, the same
       * as it would from any other column.
       */

      T.wrap_pend = 0;

      if (T.col > 0)
        {
          T.col--;
        }

      return 1;

    case 0x09: /* HT */
      vt_tab ();

      return 1;

    case 0x0A: /* LF */
    case 0x0B: /* VT */
    case 0x0C: /* FF */
      T.wrap_pend = 0;
      vt_index ();

      if (T.newline)
        {
          T.col = 0;
        }

      return 1;

    case 0x0D: /* CR */
      vt_cr ();

      return 1;

    case 0x0E: /* SO */
      T.gl = 1;

      return 1;

    case 0x0F: /* SI */
      T.gl = 0;

      return 1;

    case 0x7F: /* DEL */
      return 1;

    default:
      break;
    }

  return 0;
}

/*****************************************************************************/

void
vgaterm_putc (unsigned char c)
{
  vt_sync_geometry ();

  /* CAN and SUB abandon whatever sequence is in progress. */
  if (c == 0x18 || c == 0x1A)
    {
      T.state = VS_GROUND;
      vt_sync ();

      return;
    }

  if (c == 0x1B && T.state != VS_STRING && T.state != VS_STRING_E)
    {
      T.state = VS_ESC;
      T.ss = 0;

      return;
    }

  switch (T.state)
    {
    case VS_GROUND:
      if (c < 0x20 || c == 0x7F)
        {
          (void)vt_c0 (c);
        }
      else
        {
          vt_putglyph (c);
        }

      break;

    case VS_ESC:
      vt_esc_dispatch (c);

      break;

    case VS_CSI:
      if (c >= '0' && c <= '9')
        {
          if (T.nparm == 0)
            {
              T.nparm = 1;
              T.parm [0] = 0;
            }

          if (T.nparm <= VT_PARMS)
            {
              unsigned *p = &T.parm [T.nparm - 1];

              if (*p < 100000u)
                {
                  *p = *p * 10u + (unsigned)(c - '0');
                }
            }

        }
      else if (c == ';' || c == ':')
        {
          if (T.nparm == 0)
            {
              T.nparm = 1;
              T.parm [0] = 0;
            }

          if (T.nparm < VT_PARMS)
            {
              T.parm [T.nparm] = 0;
              T.nparm++;
            }
        }
      else if (c >= 0x3C && c <= 0x3F)
        {
          T.priv = c; /* ? > < = */
        }
      else if (c >= 0x20 && c <= 0x2F)
        {
          T.inter = c; /* intermediate: space ! " # $ % & ' ( ) * + , - . / */
        }
      else if (c >= 0x40 && c <= 0x7E)
        {
          vt_csi_dispatch (c);
          T.state = VS_GROUND;
        }
      else if (c < 0x20)
        {
          /* Controls are executed in flight and do not break the sequence. */
          (void)vt_c0 (c);
        }
      else
        {
          T.state = VS_GROUND;
        }

      break;

    case VS_CSI_IGN:
      /* Final bytes are 0x40..0x7E; anything below 0x20 still executes. */
      if (c >= 0x40 && c <= 0x7E)
        {
          T.state = VS_GROUND;
        }
      else if (c < 0x20)
        {
          (void)vt_c0 (c);
        }
      else if (c > 0x3F)
        {
          T.state = VS_GROUND;
        }

      break;

    case VS_STRING:
      if (c == 0x07)
        {
          T.state = VS_GROUND;
        }
      else if (c == 0x1B)
        {
          T.state = VS_STRING_E;
        }

      break;

    case VS_STRING_E:
      T.state = (c == '\\') ? VS_GROUND : VS_STRING;

      break;

    case VS_CHARSET:
      if (T.scratch < 4)
        {
          T.g [T.scratch] = c;
        }

      T.state = VS_GROUND;

      break;

    case VS_HASH:
      if (c == '8') /* DECALN */
        {
          if (T.rows && T.cols)
            {
              vgacon_fill (0, 0, T.rows - 1, T.cols - 1, 'E', vt_attr ());
            }

          vt_goto (0, 0);
        }

      T.state = VS_GROUND;

      break;

    case VS_VT52_Y1:
      T.scratch = (unsigned char)((c >= 0x20) ? (c - 0x20) : 0);
      T.state = VS_VT52_Y2;

      break;

    case VS_VT52_Y2:
      vt_goto (T.scratch, (unsigned)((c >= 0x20) ? (c - 0x20) : 0));
      T.state = VS_GROUND;

      break;

    case VS_DRI_FG:
      vt_dri_fg (c);
      T.state = VS_GROUND;

      break;

    case VS_DRI_BG:
      vt_dri_bg (c);
      T.state = VS_GROUND;

      break;

    case VS_DRI_MODE:
      T.state = VS_GROUND;

      break;

    case VS_SKIP2:
      if (--T.scratch == 0)
        {
          T.state = VS_GROUND;
        }

      break;

    case VS_KEYDEF_SC:
      T.keydef_sc = c;
      T.keydef_len = 0;
      T.state = VS_KEYDEF_S;

      break;

    case VS_KEYDEF_S:
      if (c == 0 || T.keydef_len + 1 >= VT_KEYDEF)
        {
          T.keydef [T.keydef_len] = 0;
          kbd_define_key (T.keydef_sc, T.keydef);
          T.state = VS_GROUND;
        }
      else
        {
          T.keydef [T.keydef_len++] = (char)c;
        }

      break;

    default:
      T.state = VS_GROUND;

      break;
    }

  vt_sync ();
}

/*****************************************************************************/

/*
 * Clear and home, keeping the current modes.  This is what BDOS 221 / CLS
 * wants; going through the byte stream instead would not work in VT52 mode,
 * where ESC [ 2 J is not a control sequence at all.
 */

void
vgaterm_clear (void)
{
  if (!T.inited)
    {
      vt_defaults ();
    }
  else
    {
      vt_sync_geometry ();
    }

  vt_erase_display (2);
  vt_goto (0, 0);
  vt_sync ();
}

/*****************************************************************************/

int
vgaterm_is_vt52 (void)
{
  return T.inited ? (T.vt52 ? 1 : 0) : 1;
}

/*****************************************************************************/

int
vgaterm_appcursor (void)
{
  return T.appcursor ? 1 : 0;
}

/*****************************************************************************/

int
vgaterm_appkeypad (void)
{
  return T.appkeypad ? 1 : 0;
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
