/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: f6253786-91a6-11f1-991c-80ee73e9b8e7
 */

/*****************************************************************************/

/* kbd.h - PS/2 (8042) keyboard scancode set 1 decoding and key mapping */

/*****************************************************************************/

#ifndef KBD_H
# define KBD_H

/*****************************************************************************/

void kbd_init (void);

/* Drain the 8042 output buffer into the FIFO.  Cheap when nothing is ready. */
void kbd_poll (void);

/* Non-zero when at least one byte is waiting. */
int kbd_stat (void);

/* Next byte, or -1 when the FIFO is empty.  Never blocks. */
int kbd_get (void);

/*****************************************************************************/

/*
 * DRI ESC : <scancode> <string> NUL - redefine a key.
 *
 * scancode is a set 1 make code; add KBD_EXT to name an E0-prefixed key
 * (KBD_EXT | 0x48 is the grey cursor up, for instance).  A zero-length
 * string removes the definition and restores the built-in meaning.
 */

# define KBD_EXT 0x80

void kbd_define_key (unsigned scancode, const char *s);

/*****************************************************************************/

/* Lock state, for anyone that wants to show or change it. */

# define KBD_LED_SCROLL 0x01
# define KBD_LED_NUM 0x02
# define KBD_LED_CAPS 0x04

unsigned kbd_locks (void);
void kbd_set_locks (unsigned mask);

/*
 * Which lamps are allowed to light.  A lock whose bit is clear here still
 * works - the shift still locks, the keypad still switches - but its lamp
 * stays dark.  All three are enabled at boot.
 */

unsigned kbd_led_mask (void);
void kbd_set_led_mask (unsigned mask);

/*****************************************************************************/

/*
 * What the Caps Lock key does.  The key is in the place the Ctrl key
 * occupied on the terminals most CP/M software was written for, which is
 * why swapping or replacing it is worth having.
 *
 *   KBD_CAPS_ON     Caps Lock locks the shift; left Ctrl is left Ctrl.
 *   KBD_CAPS_OFF    Caps Lock is dead: no shift lock, no LED, nothing.
 *   KBD_CAPS_CTRL   Caps Lock is a second left Ctrl.
 *   KBD_CAPS_SWAP   Caps Lock and left Ctrl exchange functions.
 *
 * Leaving KBD_CAPS_ON releases the shift lock, so the mode change cannot
 * strand the keyboard in upper case with no way to get out of it.
 */

# define KBD_CAPS_ON 0
# define KBD_CAPS_OFF 1
# define KBD_CAPS_CTRL 2
# define KBD_CAPS_SWAP 3

void kbd_caps_mode (unsigned mode);
unsigned kbd_caps_get (void);

/*****************************************************************************/

/*
 * Console dialect, supplied by bios.c.  When the VGA console is not the
 * active output these answer for a plain ANSI terminal, because that is what
 * is on the far end of the serial line.
 */

int kbd_term_vt52 (void);
int kbd_term_appcursor (void);
int kbd_term_appkeypad (void);

/*****************************************************************************/

#endif /* ifndef KBD_H */

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
