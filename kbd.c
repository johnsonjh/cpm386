/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 05d2347c-91a7-11f1-befd-80ee73e9b8e7
 */

/*****************************************************************************/

/* kbd.c - PS/2 (8042) keyboard scancode set 1 decoding and key mapping */

/*****************************************************************************/

/*
 * What the keys produce
 * ---------------------
 *
 *   Alphanumerics   ASCII, through Shift and Caps Lock.  Ctrl+letter gives
 *                   0x01..0x1A; Ctrl with [ \ ] ^ _ and @ gives 0x1B..0x1F
 *                   and NUL, so the whole C0 set is reachable.
 *
 *   Alt+key         ESC followed by the unmodified key.  This is the usual
 *                   "meta prefix" convention and costs nothing, because the
 *                   PC keyboard has no other use for Alt with a letter.
 *
 *   Alt+nnn         Hold Alt, type up to three decimal digits on the numeric
 *                   keypad, release Alt: the byte with that value is
 *                   delivered.  This is how the top half of code page 437
 *                   gets typed, and it works whatever Num Lock is doing,
 *                   exactly as it does under DOS.
 *
 *   Arrows          ESC [ A..D normally, ESC O A..D in application cursor
 *                   key mode (DECCKM), ESC A..D in VT52 mode.
 *
 *   Home End Ins    ESC [ 1 ~, ESC [ 4 ~, ESC [ 2 ~, ESC [ 5 ~, ESC [ 6 ~.
 *   PgUp PgDn       Delete sends RUB (0x7F): CP/M line editing and ED both
 *   Delete          want a rubout there, and no CP/M program is looking for
 *                   ESC [ 3 ~.
 *
 *   F1..F12         ESC O P..S for F1..F4 (ESC P..S in VT52 mode), then
 *                   ESC [ 15 ~, 17 ~, 18 ~, 19 ~, 20 ~, 21 ~, 23 ~, 24 ~
 *                   for F5..F12.  This is the VT220 layout everything else
 *                   already speaks.
 *
 *   Keypad          Digits and operators as printed while Num Lock is on;
 *                   the cursor / editing meanings while it is off.  In
 *                   application keypad mode (DECKPAM) the keys send
 *                   ESC O p..y, ESC O l m n M and so on instead.
 *
 *   Ctrl+Alt+Del    Warm reboot, as on any PC.
 *
 * Any key can be given a replacement string with DRI's ESC : sequence; a
 * definition wins over everything above except the Alt+nnn accumulator.
 */

/*****************************************************************************/

#include "absaddr.h"
#include "io.h"
#include "kbd.h"

/*****************************************************************************/

/* BIOS keyboard flags; bit 4 scroll lock, bit 5 num lock, bit 6 caps lock. */
#define BDA_KBD_FLAGS 0x417

/*****************************************************************************/

#define KBD_DATA 0x60
#define KBD_STATUS 0x64
#define KBD_CMD 0x64

#define KBD_ST_OBF 0x01 /* output buffer full: a byte is waiting for us   */
#define KBD_ST_IBF 0x02 /* input buffer full: the controller is still busy */
#define KBD_ST_AUX 0x20 /* the waiting byte came from the auxiliary port   */

#define KBD_FIFO 64
#define KBD_DEFS 12
#define KBD_DEFLEN 16

/* Bounded spins; a missing or wedged controller must never hang the OS. */
#define KBD_WAIT 20000

/*****************************************************************************/

/* Modifier and lock state. */

static unsigned char m_lshift, m_rshift, m_lctrl, m_rctrl, m_lalt, m_ralt;
static unsigned char l_caps, l_num, l_scroll;
static unsigned char leds_dirty;
static unsigned char e0_pending, e1_pending;
static unsigned char caps_mode = KBD_CAPS_ON;
static unsigned char esc_mode = KBD_ESC_NORMAL;
static unsigned char led_mask
    = KBD_LED_SCROLL | KBD_LED_NUM | KBD_LED_CAPS;

/* Alt+nnn accumulator: -1 when no digits have been typed yet. */
static int alt_num;

static unsigned char fifo [KBD_FIFO];
static unsigned char fifo_head, fifo_tail;

static struct
{
  unsigned char sc; /* 0 = free slot */
  char s [KBD_DEFLEN];
} defs [KBD_DEFS];

/*****************************************************************************/

/* Scancode set 1, unshifted and shifted.  0 means "not a plain character". */

static const unsigned char map_base [0x60] = {
  0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',  /* 00-07 */
  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t', /* 08-0F */
  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  /* 10-17 */
  'o',  'p',  '[',  ']',  '\r', 0,    'a',  's',  /* 18-1F */
  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  /* 20-27 */
  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',  /* 28-2F */
  'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  /* 30-37 */
  0,    ' ',  0,    0,    0,    0,    0,    0,    /* 38-3F */
  0,    0,    0,    0,    0,    0,    0,    0,    /* 40-47 */
  0,    0,    '-',  0,    0,    0,    '+',  0,    /* 48-4F */
  0,    0,    0,    0,    0,    0,    '\\', 0,    /* 50-57 */
  0,    0,    0,    0,    0,    0,    0,    0     /* 58-5F */
};

static const unsigned char map_shift [0x60] = {
  0,    0x1B, '!',  '@',  '#',  '$',  '%',  '^',  /* 00-07 */
  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t', /* 08-0F */
  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  /* 10-17 */
  'O',  'P',  '{',  '}',  '\r', 0,    'A',  'S',  /* 18-1F */
  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  /* 20-27 */
  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',  /* 28-2F */
  'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  /* 30-37 */
  0,    ' ',  0,    0,    0,    0,    0,    0,    /* 38-3F */
  0,    0,    0,    0,    0,    0,    0,    0,    /* 40-47 */
  0,    0,    '-',  0,    0,    0,    '+',  0,    /* 48-4F */
  0,    0,    0,    0,    0,    0,    '|',  0,    /* 50-57 */
  0,    0,    0,    0,    0,    0,    0,    0     /* 58-5F */
};

/*
 * The numeric keypad, Num Lock on.  Indexed by scancode - 0x47; entries are
 * the printed characters.  0x37 (*), 0x4A (-) and 0x4E (+) are in the tables
 * above because they are unaffected by Num Lock.
 */

static const unsigned char map_keypad [13] = {
  '7', '8', '9', 0, '4', '5', '6', 0, '1', '2', '3', '0', '.'
};

/*
 * Application keypad mode (DECKPAM) final characters, same indexing.  A VT
 * keypad sends ESC O <final>; there is no PF1..PF4 row on a PC keyboard, so
 * Num Lock, / and * keep their Num Lock meanings.
 */

static const unsigned char map_keypad_app [13] = {
  'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n'
};

/*****************************************************************************/

/* FIFO. */

static void
kbd_push (unsigned char c)
{
  unsigned char next = (unsigned char)((fifo_tail + 1u) % KBD_FIFO);

  if (next == fifo_head)
    {
      return; /* full: drop, rather than wedge the decoder */
    }

  fifo [fifo_tail] = c;
  fifo_tail = next;
}

/*****************************************************************************/

static void
kbd_emit (const char *s)
{
  while (*s)
    {
      kbd_push ((unsigned char)*s++);
    }
}

/*****************************************************************************/

int
kbd_stat (void)
{
  return fifo_head != fifo_tail;
}

/*****************************************************************************/

int
kbd_get (void)
{
  int c;

  if (fifo_head == fifo_tail)
    {
      return -1;
    }

  c = (int)fifo [fifo_head];
  fifo_head = (unsigned char)((fifo_head + 1u) % KBD_FIFO);

  return c;
}

/*****************************************************************************/

unsigned
kbd_locks (void)
{
  unsigned v = 0;

  if (l_scroll)
    {
      v |= KBD_LED_SCROLL;
    }

  if (l_num)
    {
      v |= KBD_LED_NUM;
    }

  if (l_caps)
    {
      v |= KBD_LED_CAPS;
    }

  return v;
}

/*****************************************************************************/

void
kbd_define_key (unsigned scancode, const char *s)
{
  int i;
  int free_slot = -1;

  if (scancode == 0 || scancode > 0xFF)
    {
      return;
    }

  for (i = 0; i < KBD_DEFS; i++)
    {
      if (defs [i].sc == (unsigned char)scancode)
        {
          break;
        }

      if (defs [i].sc == 0 && free_slot < 0)
        {
          free_slot = i;
        }
    }

  if (i == KBD_DEFS)
    {
      if (!s || !*s || free_slot < 0)
        {
          return; /* nothing to clear, or no room left */
        }

      i = free_slot;
    }

  if (!s || !*s)
    {
      defs [i].sc = 0; /* empty definition restores the built-in meaning */

      return;
    }

  {
    int n = 0;

    while (s [n] && n < KBD_DEFLEN - 1)
      {
        defs [i].s [n] = s [n];
        n++;
      }

    defs [i].s [n] = 0;
    defs [i].sc = (unsigned char)scancode;
  }
}

/*****************************************************************************/

static const char *
kbd_lookup_def (unsigned char sc, int ext)
{
  unsigned char key = (unsigned char)(ext ? (sc | KBD_EXT) : sc);
  int i;

  for (i = 0; i < KBD_DEFS; i++)
    {
      if (defs [i].sc == key)
        {
          return defs [i].s;
        }
    }

  return 0;
}

/*****************************************************************************/

/* Controller helpers.  Every wait is bounded. */

static void kbd_scancode (unsigned char sc);

/*****************************************************************************/

static int
kbd_wait_write (void)
{
  int spins = 0;

  while ((inb (KBD_STATUS) & KBD_ST_IBF) != 0 && ++spins < KBD_WAIT)
    {
      io_delay ();
    }

  return (inb (KBD_STATUS) & KBD_ST_IBF) == 0;
}

/*****************************************************************************/

static void
kbd_update_leds (void)
{
  unsigned char v;
  int spins;

  /*
   * Bits 0..2 of the 0xED parameter are scroll, num and caps in that order,
   * which is the same order as the KBD_LED_* flags.  A lamp the caller has
   * masked off never lights, whatever its lock is doing.
   */

  v = (unsigned char)(kbd_locks () & led_mask & 0x07);

  if (!kbd_wait_write ())
    {
      return;
    }

  outb (KBD_DATA, 0xED);

  /*
   * Wait for the acknowledgement.  Scancodes can arrive first if the user is
   * still typing; they go back through the decoder rather than being
   * dropped.  That cannot recurse: a lock key only sets leds_dirty, which
   * kbd_poll() acts on next time round.
   */

  for (spins = 0; spins < KBD_WAIT; spins++)
    {
      if ((inb (KBD_STATUS) & (KBD_ST_OBF | KBD_ST_AUX)) == KBD_ST_OBF)
        {
          unsigned char r = inb (KBD_DATA);

          if (r == 0xFA)
            {
              break;
            }

          if (r == 0xFE)
            {
              return; /* resend: give up rather than loop */
            }

          kbd_scancode (r);
        }

      io_delay ();
    }

  if (!kbd_wait_write ())
    {
      return;
    }

  outb (KBD_DATA, v);

  for (spins = 0; spins < KBD_WAIT; spins++)
    {
      if ((inb (KBD_STATUS) & (KBD_ST_OBF | KBD_ST_AUX)) == KBD_ST_OBF)
        {
          unsigned char r = inb (KBD_DATA);

          if (r == 0xFA)
            {
              break;
            }

          kbd_scancode (r);
        }

      io_delay ();
    }
}

/*****************************************************************************/

static int
kbd_shift_state (void)
{
  return (m_lshift || m_rshift) ? 1 : 0;
}

/*****************************************************************************/

static int
kbd_ctrl_state (void)
{
  return (m_lctrl || m_rctrl) ? 1 : 0;
}

/*****************************************************************************/

static int
kbd_alt_state (void)
{
  return (m_lalt || m_ralt) ? 1 : 0;
}

/*****************************************************************************/

/* Cursor and editing keys, in whichever dialect is current. */

static void
kbd_send_cursor (char final)
{
  char b [4];

  if (kbd_term_vt52 ())
    {
      b [0] = 0x1B;
      b [1] = final;
      b [2] = 0;
    }
  else
    {
      b [0] = 0x1B;
      b [1] = (char)(kbd_term_appcursor () ? 'O' : '[');
      b [2] = final;
      b [3] = 0;
    }

  kbd_emit (b);
}

/*****************************************************************************/

static void
kbd_send_tilde (const char *digits)
{
  kbd_push (0x1B);
  kbd_push ('[');
  kbd_emit (digits);
  kbd_push ('~');
}

/*****************************************************************************/

/* F1..F4 are the VT PF keys; F5 upward are the VT220 tilde codes. */

static void
kbd_send_fkey (int n)
{
  static const char *const tilde [8]
      = { "15", "17", "18", "19", "20", "21", "23", "24" };

  if (n >= 1 && n <= 4)
    {
      kbd_push (0x1B);

      if (!kbd_term_vt52 ())
        {
          kbd_push ('O');
        }

      kbd_push ((unsigned char)('P' + n - 1));

      return;
    }

  if (n >= 5 && n <= 12)
    {
      kbd_send_tilde (tilde [n - 5]);
    }
}

/*****************************************************************************/

/* The keypad with Num Lock off, and the grey editing keys. */

static void
kbd_send_edit (unsigned char sc)
{
  switch (sc)
    {
    case 0x47: /* Home */
      if (kbd_term_vt52 ())
        {
          kbd_emit ("\033H");
        }
      else
        {
          kbd_send_tilde ("1");
        }

      break;

    case 0x48:
      kbd_send_cursor ('A');

      break;

    case 0x49:
      kbd_send_tilde ("5");

      break;

    case 0x4B:
      kbd_send_cursor ('D');

      break;

    case 0x4C: /* keypad 5 with Num Lock off: nothing on a real terminal */
      break;

    case 0x4D:
      kbd_send_cursor ('C');

      break;

    case 0x4F: /* End */
      kbd_send_tilde ("4");

      break;

    case 0x50:
      kbd_send_cursor ('B');

      break;

    case 0x51:
      kbd_send_tilde ("6");

      break;

    case 0x52: /* Insert */
      kbd_send_tilde ("2");

      break;

    case 0x53: /* Delete: RUB, which is what CP/M line editing wants */
      kbd_push (0x7F);

      break;

    default:
      break;
    }
}

/*****************************************************************************/

/*
 * The keypad with Num Lock on.  In application keypad mode the printed
 * meaning is replaced by the ESC O form, which is what a VT program that
 * sent DECKPAM is expecting.
 */

static int
kbd_send_keypad (unsigned char sc)
{
  unsigned idx;
  unsigned char ch;

  if (sc < 0x47 || sc > 0x53)
    {
      return 0;
    }

  idx = (unsigned)(sc - 0x47);

  if (idx >= sizeof map_keypad || map_keypad [idx] == 0)
    {
      return 0;
    }

  if (kbd_term_appkeypad ())
    {
      kbd_push (0x1B);
      kbd_push ((unsigned char)(kbd_term_vt52 () ? '?' : 'O'));
      kbd_push (map_keypad_app [idx]);

      return 1;
    }

  ch = map_keypad [idx];
  kbd_push (ch);

  return 1;
}

/*****************************************************************************/

/* Alt held plus a keypad digit: accumulate rather than emit. */

static int
kbd_alt_digit (unsigned char sc)
{
  static const signed char digit [13]
      = { 7, 8, 9, -1, 4, 5, 6, -1, 1, 2, 3, 0, -1 };
  int d;

  if (sc < 0x47 || sc > 0x53)
    {
      return 0;
    }

  d = digit [sc - 0x47];

  if (d < 0)
    {
      return 0;
    }

  if (alt_num < 0)
    {
      alt_num = 0;
    }

  alt_num = (alt_num * 10 + d) & 0xFFFF;

  return 1;
}

/*****************************************************************************/

/* Turn a plain character key into the byte it should produce. */

static void
kbd_send_char (unsigned char sc)
{
  unsigned char ch;

  ch = kbd_shift_state () ? map_shift [sc] : map_base [sc];

  if (!ch)
    {
      return;
    }

  /* Caps Lock swaps the case of letters only, and composes with Shift. */
  if (l_caps)
    {
      if (ch >= 'a' && ch <= 'z')
        {
          ch = (unsigned char)(ch - 'a' + 'A');
        }
      else if (ch >= 'A' && ch <= 'Z')
        {
          ch = (unsigned char)(ch - 'A' + 'a');
        }
    }

  if (kbd_ctrl_state ())
    {
      unsigned char u = ch;

      if (u >= 'a' && u <= 'z')
        {
          u = (unsigned char)(u - 'a' + 'A');
        }

      if (u >= 'A' && u <= 'Z')
        {
          ch = (unsigned char)(u - '@'); /* A -> 0x01 ... Z -> 0x1A */
        }
      else if (u == '[' || u == '\\' || u == ']' || u == '^' || u == '_')
        {
          ch = (unsigned char)(u - '@'); /* 0x1B .. 0x1F */
        }
      else if (u == '@' || u == ' ' || u == '2')
        {
          ch = 0; /* NUL */
        }
      else if (u == '?' || u == '/')
        {
          ch = 0x7F;
        }
      else if (u == '6')
        {
          ch = 0x1E;
        }
      else if (u == '-')
        {
          ch = 0x1F;
        }
      else if (ch != '\r' && ch != '\b' && ch != '\t' && ch != 0x1B)
        {
          return; /* Ctrl with anything else: nothing to send */
        }
    }

  /*
   * Alt as a meta prefix.  Not applied to the keypad, which Alt claims for
   * numeric entry, and not to Ctrl combinations, which are already control
   * characters.
   */

  if (kbd_alt_state () && !kbd_ctrl_state ())
    {
      kbd_push (0x1B);
    }

  kbd_push (ch);
}

/*****************************************************************************/

static void
kbd_reboot (void)
{
  extern void bios_system_reboot (int warm);

  bios_system_reboot (1);
}

/*****************************************************************************/

/* One make code, already stripped of its E0 prefix. */

static void
kbd_make (unsigned char sc, int ext)
{
  const char *def;

  /* Modifiers first: they never produce output. */
  switch (sc)
    {
    case 0x2A:
      if (!ext) /* E0 2A is part of the Print Screen sequence */
        {
          m_lshift = 1;
        }

      return;

    case 0x36:
      m_rshift = 1;

      return;

    case 0x1D:
      if (ext)
        {
          m_rctrl = 1;
        }
      else
        {
          m_lctrl = 1;
        }

      return;

    case 0x38:
      if (ext)
        {
          m_ralt = 1;
        }
      else
        {
          m_lalt = 1;
        }

      return;

    case 0x3A:
      l_caps = (unsigned char)!l_caps;
      leds_dirty = 1;

      return;

    case 0x45:
      if (!ext) /* E0 45 does not occur; E1 1D 45 is Pause and is filtered */
        {
          l_num = (unsigned char)!l_num;
          leds_dirty = 1;
        }

      return;

    case 0x46:
      if (!ext)
        {
          l_scroll = (unsigned char)!l_scroll;
          leds_dirty = 1;
        }

      return;

    default:
      break;
    }

  /* Ctrl+Alt+Del reboots, as it does on every other PC operating system. */
  if (sc == 0x53 && kbd_ctrl_state () && kbd_alt_state ())
    {
      kbd_reboot ();

      return;
    }

  /*
   * Alt+nnn on the keypad.  Checked before user definitions and before the
   * ordinary mapping, because holding Alt is what makes those digits mean
   * something else.
   */

  if (!ext && kbd_alt_state () && kbd_alt_digit (sc))
    {
      return;
    }

  def = kbd_lookup_def (sc, ext);

  if (def)
    {
      kbd_emit (def);

      return;
    }

  if (ext)
    {
      switch (sc)
        {
        case 0x1C: /* keypad Enter */
          if (kbd_term_appkeypad ())
            {
              kbd_push (0x1B);
              kbd_push ((unsigned char)(kbd_term_vt52 () ? '?' : 'O'));
              kbd_push ('M');
            }
          else
            {
              kbd_push ('\r');
            }

          return;

        case 0x35: /* keypad / */
          if (kbd_ctrl_state ())
            {
              kbd_push (0x7F);
            }
          else
            {
              kbd_push ('/');
            }

          return;

        case 0x5B: /* the Windows and Menu keys have nothing to say */
        case 0x5C:
        case 0x5D:
          return;

        default:
          kbd_send_edit (sc);

          return;
        }
    }

  /* Function keys. */
  if (sc >= 0x3B && sc <= 0x44)
    {
      kbd_send_fkey ((int)sc - 0x3B + 1);

      return;
    }

  if (sc == 0x57 || sc == 0x58)
    {
      kbd_send_fkey (sc == 0x57 ? 11 : 12);

      return;
    }

  /*
   * The numeric keypad proper.  0x4A and 0x4E are the grey - and + and are
   * excluded: Num Lock does not affect them, so they fall through to the
   * ordinary character tables like the keypad * does.
   */

  if (sc >= 0x47 && sc <= 0x53 && sc != 0x4A && sc != 0x4E)
    {
      if (l_num != kbd_shift_state ()) /* Shift temporarily inverts Num Lock */
        {
          if (kbd_send_keypad (sc))
            {
              return;
            }
        }

      kbd_send_edit (sc);

      return;
    }

  if (sc < 0x60)
    {
      kbd_send_char (sc);
    }
}

/*****************************************************************************/

static void
kbd_break (unsigned char sc, int ext)
{
  switch (sc)
    {
    case 0x2A:
      if (!ext)
        {
          m_lshift = 0;
        }

      break;

    case 0x36:
      m_rshift = 0;

      break;

    case 0x1D:
      if (ext)
        {
          m_rctrl = 0;
        }
      else
        {
          m_lctrl = 0;
        }

      break;

    case 0x38:
      if (ext)
        {
          m_ralt = 0;
        }
      else
        {
          m_lalt = 0;
        }

      /*
       * Releasing the last Alt is what completes an Alt+nnn entry.  Values
       * above 255 wrap, which is what DOS does with them.
       */

      if (!kbd_alt_state () && alt_num >= 0)
        {
          kbd_push ((unsigned char)(alt_num & 0xFF));
          alt_num = -1;
        }

      break;

    default:
      break;
    }
}

/*****************************************************************************/

/*
 * Caps Lock remapping, applied to the raw make/break code before anything
 * else looks at it, so the two keys really do change places rather than
 * merely producing each other's output.  Returns 0 for a key that has been
 * switched off entirely.
 */

static unsigned char
kbd_remap (unsigned char sc, int ext)
{
  if (ext)
    {
      return sc; /* E0 1D is the right Ctrl and is never remapped */
    }

  if (sc == 0x01 && esc_mode == KBD_ESC_SWAP) /* ESC */
    {
      return 0x29;
    }

  if (sc == 0x29 && esc_mode == KBD_ESC_SWAP) /* Tilde */
    {
      return 0x01;
    }

  if (sc == 0x3A) /* Caps Lock */
    {
      switch (caps_mode)
        {
        case KBD_CAPS_OFF:
          return 0;

        case KBD_CAPS_CTRL:
        case KBD_CAPS_SWAP:
          return 0x1D;

        default:
          return 0x3A;
        }
    }

  if (sc == 0x1D && caps_mode == KBD_CAPS_SWAP) /* left Ctrl */
    {
      return 0x3A;
    }

  return sc;
}

/*****************************************************************************/

void
kbd_set_locks (unsigned mask)
{
  l_scroll = (unsigned char)((mask & KBD_LED_SCROLL) ? 1 : 0);
  l_num = (unsigned char)((mask & KBD_LED_NUM) ? 1 : 0);

  /* Caps Lock cannot be latched on while the key that clears it is gone. */
  if (caps_mode == KBD_CAPS_ON)
    {
      l_caps = (unsigned char)((mask & KBD_LED_CAPS) ? 1 : 0);
    }
  else
    {
      l_caps = 0;
    }

  leds_dirty = 1;
}

/*****************************************************************************/

unsigned
kbd_led_mask (void)
{
  return led_mask & 0x07u;
}

/*****************************************************************************/

void
kbd_set_led_mask (unsigned mask)
{
  led_mask = (unsigned char)(mask & 0x07u);
  leds_dirty = 1;
}

/*****************************************************************************/

void
kbd_caps_mode (unsigned mode)
{
  if (mode > KBD_CAPS_SWAP)
    {
      return;
    }

  caps_mode = (unsigned char)mode;

  /*
   * Any mode but the normal one takes the shift lock away, so it must not
   * be left latched on with no key able to clear it.
   */

  if (mode != KBD_CAPS_ON && l_caps)
    {
      l_caps = 0;
      leds_dirty = 1;
    }

  /* A held-down Caps Lock that has just become a Ctrl must not stick. */
  if (mode != KBD_CAPS_ON)
    {
      m_lctrl = 0;
    }
}

/*****************************************************************************/

unsigned
kbd_caps_get (void)
{
  return caps_mode;
}

/*****************************************************************************/

void
kbd_esc_mode (unsigned mode)
{
  if (mode <= KBD_ESC_SWAP)
    {
      esc_mode = (unsigned char)mode;
    }
}

/*****************************************************************************/

unsigned
kbd_esc_get (void)
{
  return esc_mode;
}

/*****************************************************************************/

/* One byte straight off the controller. */

static void
kbd_scancode (unsigned char sc)
{
  if (sc == 0xE0)
    {
      e0_pending = 1;

      return;
    }

  if (sc == 0xE1)
    {
      e1_pending = 2; /* Pause: E1 1D 45 (and E1 9D C5 on release) */

      return;
    }

  if (e1_pending)
    {
      e1_pending--;

      return; /* Pause / Break produces nothing */
    }

  {
    int ext = e0_pending;
    int brk = (sc & 0x80) != 0;
    unsigned char key = kbd_remap ((unsigned char)(sc & 0x7F), ext);

    e0_pending = 0;

    if (key == 0)
      {
        return; /* the key has been switched off */
      }

    if (brk)
      {
        kbd_break (key, ext);
      }
    else
      {
        kbd_make (key, ext);
      }
  }
}

/*****************************************************************************/

void
kbd_poll (void)
{
  int spins = 0;

  /*
   * Bounded so a controller that reports data forever cannot lock the
   * system up; 64 codes is far more than a human generates between polls.
   */

  while (spins++ < 64)
    {
      unsigned char st = inb (KBD_STATUS);

      if ((st & KBD_ST_OBF) == 0)
        {
          break;
        }

      if ((st & KBD_ST_AUX) != 0)
        {
          (void)inb (KBD_DATA); /* mouse byte: discard */

          continue;
        }

      kbd_scancode (inb (KBD_DATA));
    }

  if (leds_dirty)
    {
      leds_dirty = 0;
      kbd_update_leds ();
    }
}

/*****************************************************************************/

void
kbd_init (void)
{
  int i;

  m_lshift = m_rshift = m_lctrl = m_rctrl = m_lalt = m_ralt = 0;
  e0_pending = e1_pending = 0;
  alt_num = -1;
  fifo_head = fifo_tail = 0;

  for (i = 0; i < KBD_DEFS; i++)
    {
      defs [i].sc = 0;
    }

  /*
   * Adopt the lock state the BIOS left in the keyboard flags at 0040:0017,
   * so the shift behaviour agrees with what the user last saw.
   *
   * Num Lock is the exception: it is forced on, because a numeric keypad
   * that does not type numbers is a surprise, and everything it does with
   * Num Lock off is already on the dedicated cursor and editing keys.  The
   * LED is driven from this below, so the keyboard still tells the truth.
   */

  {
    unsigned char f = *ABS_U8 (BDA_KBD_FLAGS);

    l_scroll = (unsigned char)((f & 0x10) ? 1 : 0);
    l_caps = (unsigned char)((f & 0x40) ? 1 : 0);
    l_num = 1;
  }

  /* Drain anything left over from the boot loader, then set the LEDs. */
  for (i = 0; i < 32; i++)
    {
      if ((inb (KBD_STATUS) & KBD_ST_OBF) == 0)
        {
          break;
        }

      (void)inb (KBD_DATA);
    }

  kbd_update_leds ();
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
