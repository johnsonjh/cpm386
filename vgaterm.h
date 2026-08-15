/*
 * CP/M-386 - vgaterm.h
 * Copyright (c) 2025-2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: c3ad0618-91a5-11f1-af2b-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * NOTE: This code is a new version of my old VT102/VT52 emulation that
 * was ported to CP/M-386 and re-commented using AI/LLM-assistance.
 */

/*****************************************************************************/

/* vgaterm.h - VT102 / VT52 / DRI terminal emulation over the VGA text plane */

/*****************************************************************************/

#ifndef VGATERM_H
# define VGATERM_H

/*****************************************************************************/

/*
 * This is the layer vgacon.h describes as "the escape sequence interpreter".
 * It owns the logical cursor, the current attribute, the scrolling region,
 * tab stops and the mode flags, and drives the plane exclusively through the
 * vgacon primitives.
 *
 * Three dialects are spoken at once, because CP/M software expects all of
 * them:
 *
 *   VT52 / DRI   ESC <letter>.  The default.  The Digital Research console
 *                on the PC is documented as "a superset of VT52 codes", and
 *                that superset is what DOS-Plus, Concurrent DOS and the
 *                CP/M-86 terminal emulator present.
 *
 *   VT102        ESC [ ... .  CSI sequences never collide with the VT52 or
 *                DRI sets, so they are always interpreted.  The remaining
 *                VT102 ESC-space codes (IND, NEL, HTS, RI, RIS) collide with
 *                VT52/DRI letters and are only active in ANSI mode, which a
 *                program enters with ESC < or ESC [ ? 2 h and leaves with
 *                ESC [ ? 2 l.
 *
 *   DRI          The lowercase extensions (colour, cursor, wrap, ...).  These
 *                are active in both modes; the single exception is ESC c,
 *                which is "set background colour" in VT52/DRI mode and RIS
 *                (hard reset) in ANSI mode.
 *
 * See vgaterm.c for the sequence-by-sequence table.
 */

/*****************************************************************************/

/* Byte stream entry point.  Replaces vgacon_putc() on the console path. */

void vgaterm_putc (unsigned char c);

/*
 * Power-on state: default attribute, full-screen scrolling region, tab stops
 * every eight columns, wrap on, cursor on, VT52/DRI mode.  Clears the screen.
 */

void vgaterm_reset (void);

/* Reset without clearing the screen; used at first use and after a mode set. */

void vgaterm_soft_reset (void);

/*
 * Clear the screen and home the cursor without disturbing the modes.  BDOS
 * 221 / CLS goes through this rather than writing ESC [ 2 J into the byte
 * stream, because in VT52 mode that is not a control sequence at all.
 */

void vgaterm_clear (void);

/*****************************************************************************/

/*
 * Modes the keyboard has to see, so that the bytes it sends back match the
 * dialect the program has selected.
 */

int vgaterm_is_vt52 (void);     /* DECANM reset */
int vgaterm_appcursor (void);   /* DECCKM set   */
int vgaterm_appkeypad (void);   /* DECKPAM      */

/*****************************************************************************/

/*
 * Terminal-to-host replies: DSR (ESC[6n), DA (ESC[c), DECID (ESC Z) and
 * DECREQTPARM all require an answer on the input stream.  The BIOS console
 * input path drains this queue ahead of the keyboard.
 */

int vgaterm_reply_avail (void);
int vgaterm_reply_get (void); /* -1 when the queue is empty */

/*****************************************************************************/

#endif /* ifndef VGATERM_H */

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
