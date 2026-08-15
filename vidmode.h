/*
 * CP/M-386 - vidmode.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: a34b45ba-8caf-11f1-a827-80ee73e9b8e7
 */

/*****************************************************************************/

/* vidmode.h - video mode table, selection, and console mode ownership */

/*****************************************************************************/

#ifndef VIDMODE_H
# define VIDMODE_H

/*****************************************************************************/

/*
 * CP/M-386 mode identifiers.  Deliberately not BIOS mode numbers: several
 * of these are mode 3 plus a scan line count and a font, and a stable small
 * identifier is what BDOS 229-231 and TEXTMODE trade in.
 */

# define VID_MODE_40X25 0
# define VID_MODE_80X25 1 /* the default console mode and hard fallback */
# define VID_MODE_80X28 2
# define VID_MODE_80X43 3
# define VID_MODE_80X50 4
# define VID_MODE_320X200 5 /* 320x200x256, standard VGA, no VESA needed */
# define VID_MODE_STATIC 6

/*
 * VBE modes are discovered at run time, so they cannot have fixed ids.
 * They are numbered from here upward in discovery order.
 */

# define VID_MODE_VESA_BASE 16
# define VID_VESA_MAX 24
# define VID_MODE_COUNT (VID_MODE_VESA_BASE + VID_VESA_MAX)

/*****************************************************************************/

/* Flags reported per mode; mirrored into struct cpm_vidmode. */

# define VIDM_TEXT 0x0001
# define VIDM_GRAPHICS 0x0002
# define VIDM_CURRENT 0x0004  /* the mode the hardware is in now      */
# define VIDM_CONSOLE 0x0008  /* the committed console mode           */
# define VIDM_DEFAULT 0x0010  /* the 80x25 fallback                   */
# define VIDM_LFB 0x0020      /* graphics: linear framebuffer (stage 2) */
# define VIDM_VESA 0x0040     /* came from VBE, not standard VGA      */

/*****************************************************************************/

/* Actions for BDOS 231. */

# define VIDA_TRANSIENT 0 /* program mode; reverted when it exits    */
# define VIDA_CONSOLE 1   /* console mode, provisional until commit  */
# define VIDA_COMMIT 2    /* promote the provisional mode            */
# define VIDA_REVERT 3    /* back to the committed console mode      */

/*****************************************************************************/

/* Result codes returned in AX by BDOS 229-232. */

# define VIDR_OK 0x0000
# define VIDR_BADFONT 0xFFFA /* font geometry does not fit the mode */
# define VIDR_FAILED 0xFFFB  /* the BIOS did not take the mode       */
# define VIDR_NOMORE 0xFFFC  /* enumeration index past the end       */
# define VIDR_BADMODE 0xFFFD /* unknown or unsupported mode          */
# define VIDR_NOHW 0xFFFE    /* no usable video adapter              */
# define VIDR_BADPTR 0xFFFF  /* caller pointer rejected              */

/*****************************************************************************/

struct vidmode_info
{
  unsigned mode;
  unsigned flags;
  unsigned cols;   /* text modes */
  unsigned rows;   /* text modes */
  unsigned cell_h; /* text modes */
  unsigned width;  /* graphics modes */
  unsigned height; /* graphics modes */
  unsigned bpp;    /* graphics modes */
  unsigned long pitch;
  unsigned long fb_size;
  unsigned long fb_phys;
};

/*****************************************************************************/

/*
 * Learn what the BIOS left us in, without changing it.  Booting must not
 * flash the display through a mode set.
 */

void vidmode_init (void);

/* Non-zero when int 10h AH=1Ah reported a VGA, so the extended modes exist. */
int vidmode_have_vga (void);

unsigned vidmode_current (void);
unsigned vidmode_console (void);

/* Fill *out for a mode id.  Returns VIDR_OK or VIDR_NOMORE. */
unsigned vidmode_describe (unsigned mode_id, struct vidmode_info *out);

/*
 * Fill *out for the ordinal'th available mode.  Ids are sparse, so this is
 * what a caller listing modes must walk; the id itself comes back in .mode.
 */

unsigned vidmode_enum (unsigned ordinal, struct vidmode_info *out);

/* BDOS 231 back end. */
unsigned vidmode_action (unsigned mode, unsigned action);

/*
 * Put the hardware back into the committed console mode, dropping any
 * provisional one.  Called when a program exits, faults, or warm boots, so
 * a crashed or wedged program can never strand the console.  Cheap and
 * idempotent: does nothing when the mode already matches.
 */

void vidmode_restore_console (void);

/*
 * Replace console glyphs.  data == 0 restores the ROM font by re-applying
 * the committed mode.  height must equal the current cell height: keeping
 * glyph replacement and geometry changes separate means VGAFONT never has
 * to reprogram the CRTC, which is the part that differs between cards.
 * Use TEXTMODE to choose the cell height, then load a font to match.
 */

unsigned vidmode_font_load (const unsigned char *data, unsigned height,
                            unsigned count, unsigned first);

/*
 * Load DAC entries.  Ring 3 cannot reach ports 0x3C8/0x3C9, so a palette
 * has to come through the BDOS.  Components are the VGA's native 6 bits
 * per channel (0-63); count entries of three bytes starting at index first.
 */

unsigned vidmode_palette (const unsigned char *rgb, unsigned first,
                          unsigned count);

/*****************************************************************************/

#endif /* ifndef VIDMODE_H */

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
