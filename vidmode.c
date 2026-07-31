/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 9d504d04-8caf-11f1-959e-80ee73e9b8e7
 */

/*****************************************************************************/

/* vidmode.c - text mode table and console mode ownership */

/*****************************************************************************/

#include "absaddr.h"
#include "vgacon.h"
#include "vidbios.h"
#include "vidmode.h"
#include "io.h"
#include "pmode.h"

/*****************************************************************************/

#define BDA_VIDEO_MODE 0x449

/* int 10h AH=11h subfunctions that also recalculate the CRTC and BDA. */
#define FONT_8X14 0x11
#define FONT_8X8 0x12
#define FONT_8X16 0x14
#define FONT_NONE 0xFF

/* int 10h AX=12xxh BL=30h scan line counts. */
#define SCAN_200 0x00
#define SCAN_350 0x01
#define SCAN_400 0x02
#define SCAN_NONE 0xFF

/*****************************************************************************/

/*
 * Every extended text mode here is BIOS mode 3 with a scan line count and a
 * font on top; the row count falls out of scan lines divided by cell
 * height.  The cols/rows below are what we expect, but the values actually
 * reported come from the BIOS data area after the set, so a card that does
 * something different is described honestly rather than assumed.
 */

static const struct
{
  unsigned char bios_mode;
  unsigned char scanlines;
  unsigned char font;
  unsigned char cols;
  unsigned char rows;
  unsigned char needs_vga;
  unsigned short width;  /* 0 for text modes */
  unsigned short height;
  unsigned char bpp;
} modetab [VID_MODE_STATIC] = {
  { 0x01, SCAN_NONE, FONT_NONE, 40, 25, 0, 0, 0, 0 },
  { 0x03, SCAN_400, FONT_8X16, 80, 25, 0, 0, 0, 0 },
  { 0x03, SCAN_400, FONT_8X14, 80, 28, 1, 0, 0, 0 },
  { 0x03, SCAN_350, FONT_8X8, 80, 43, 1, 0, 0, 0 },
  { 0x03, SCAN_400, FONT_8X8, 80, 50, 1, 0, 0, 0 },

  /*
   * 320x200 256 colour: one 64K window at 0xA0000, no bank switching and
   * no VESA involved.  This is exactly what DOOM used.
   */

  { 0x13, SCAN_NONE, FONT_NONE, 0, 0, 1, 320, 200, 8 }
};

/*****************************************************************************/

/* Framebuffer of the standard VGA graphics mode. */

#define VGA_GFX_BASE 0xA0000UL
#define VGA_GFX_SIZE 0x10000UL

/*****************************************************************************/

/*
 * VBE modes found at init.  Only modes with a linear framebuffer are kept:
 * a banked mode would need a BIOS call per bank switch, which means a
 * protected-to-real transition per bank and is not worth having.
 */

struct vesa_mode
{
  unsigned short number; /* VBE mode number, for 4F02h */
  unsigned short width;
  unsigned short height;
  unsigned short bpp;
  unsigned long pitch;
  unsigned long phys;
  unsigned long size;
};

static struct vesa_mode vesatab [VID_VESA_MAX];
static unsigned vesa_count;

/*****************************************************************************/

static int have_vga;
static unsigned vid_committed = VID_MODE_80X25;
static unsigned vid_current = VID_MODE_80X25;
static int has_provisional;
static unsigned vid_provisional;

/*****************************************************************************/

int
vidmode_have_vga (void)
{
  return have_vga;
}

/*****************************************************************************/

unsigned
vidmode_current (void)
{
  return vid_current;
}

/*****************************************************************************/

unsigned
vidmode_console (void)
{
  return vid_committed;
}

/*****************************************************************************/

static void
regs_clear (struct vid_regs *r)
{
  int i;

  for (i = 0; i < (int)sizeof (*r); i++)
    {
      ((unsigned char *)r) [i] = 0;
    }
}

/*****************************************************************************/

/*
 * int 10h AH=1Ah reports a display combination code only on VGA and MCGA;
 * anything older leaves AL alone.  This is what gates the extended modes,
 * and it is a far better test than the text plane memory probe, which a CGA
 * (or a QEMU machine whose video hole is backed by plain RAM) also passes.
 */

static int
probe_vga (void)
{
  struct vid_regs r;

  regs_clear (&r);
  r.ax = 0x1A00;

  if (vid_int10 (&r) != 0)
    {
      return 0;
    }

  return ((r.ax & 0x00FF) == 0x1A);
}

/*****************************************************************************/

/* Drive the hardware into mode id m.  Returns VIDR_OK or a failure code. */

static unsigned set_hw_vesa (unsigned idx); /* forward */

static unsigned
set_hw (unsigned m)
{
  struct vid_regs r;
  unsigned got;

  if (m >= VID_MODE_VESA_BASE)
    {
      return set_hw_vesa (m - VID_MODE_VESA_BASE);
    }

  if (m >= VID_MODE_STATIC)
    {
      return VIDR_BADMODE;
    }

  if (modetab [m].needs_vga && !have_vga)
    {
      return VIDR_BADMODE;
    }

  /* Graphics: standard VGA below, VBE above. */
  if (modetab [m].width)
    {
      regs_clear (&r);
      r.ax = (unsigned short)modetab [m].bios_mode;

      if (vid_int10 (&r) != 0)
        {
          return VIDR_FAILED;
        }

      if (*ABS_U8 (BDA_VIDEO_MODE) != modetab [m].bios_mode)
        {
          return VIDR_FAILED;
        }

      pmode_set_fb (VGA_GFX_BASE, VGA_GFX_SIZE);

      return VIDR_OK;
    }

  /*
   * Leaving graphics for text: drop the framebuffer selector first, so a
   * program still holding it faults rather than scribbling on the text
   * plane.
   */

  pmode_set_fb (0, 0);

  /*
   * Scan line count first: it does not take effect until the next mode
   * set, which is the whole reason for the ordering here.
   */

  if (modetab [m].scanlines != SCAN_NONE)
    {
      regs_clear (&r);
      r.ax = (unsigned short)(0x1200 | modetab [m].scanlines);
      r.bx = 0x0030;

      if (vid_int10 (&r) != 0)
        {
          return VIDR_FAILED;
        }
    }

  /* Bit 7 clear, so the BIOS clears the screen for us. */
  regs_clear (&r);
  r.ax = (unsigned short)modetab [m].bios_mode;

  if (vid_int10 (&r) != 0)
    {
      return VIDR_FAILED;
    }

  /* Font last; the recalculating variants fix up the CRTC and the BDA. */
  if (modetab [m].font != FONT_NONE)
    {
      regs_clear (&r);
      r.ax = (unsigned short)(0x1100 | modetab [m].font);
      r.bx = 0x0000;

      if (vid_int10 (&r) != 0)
        {
          return VIDR_FAILED;
        }
    }

  /*
   * Believe the BIOS data area rather than our own table: it is the only
   * source that knows what the card actually did, and it doubles as a
   * check that the mode took effect at all.
   */

  got = *ABS_U8 (BDA_VIDEO_MODE);

  if (got != modetab [m].bios_mode)
    {
      return VIDR_FAILED;
    }

  vgacon_adopt_bda ();

  return VIDR_OK;
}

/*****************************************************************************/

/*
 * VBE 2.0 discovery.  The info block and each mode block have to live in
 * memory the real mode BIOS can reach, so the low transfer buffer is used
 * for both; nothing above 1MB is visible to it.
 *
 * Offsets are into the VBE structures as defined by the standard.
 */

#define VBE_INFO_OFF 0x000  /* VbeInfoBlock, 512 bytes */
#define VBE_MINFO_OFF 0x200 /* ModeInfoBlock, 256 bytes */

#define VBE_SIG 0x00
#define VBE_MODEPTR 0x0E

#define VBEM_ATTRS 0x00
#define VBEM_PITCH 0x10
#define VBEM_WIDTH 0x12
#define VBEM_HEIGHT 0x14
#define VBEM_BPP 0x19
#define VBEM_MEMMODEL 0x1B
#define VBEM_PHYSBASE 0x28

/* ModeAttributes bits we insist on. */
#define VBEA_SUPPORTED 0x0001
#define VBEA_GRAPHICS 0x0010
#define VBEA_LFB 0x0080

static unsigned long
low_rd32 (unsigned long off)
{
  return *ABS_U32 (VIDLOW_XFER + off);
}

/*****************************************************************************/

static unsigned
low_rd16 (unsigned long off)
{
  return *ABS_U16 (VIDLOW_XFER + off);
}

/*****************************************************************************/

static unsigned
low_rd8 (unsigned long off)
{
  return *ABS_U8 (VIDLOW_XFER + off);
}

/*****************************************************************************/

static void
vesa_probe (void)
{
  struct vid_regs r;
  unsigned long listp;
  unsigned long i;

  vesa_count = 0;

  if (!have_vga)
    {
      return;
    }

  /* Ask for VBE 2 information by seeding the signature. */
  *ABS_U32 (VIDLOW_XFER + VBE_INFO_OFF) = 0x32454256UL; /* "VBE2" */

  regs_clear (&r);
  r.ax = 0x4F00;
  r.es = (unsigned short)(VIDLOW_XFER >> 4);
  r.di = (unsigned short)(VBE_INFO_OFF + (VIDLOW_XFER & 0x0F));

  if (vid_int10 (&r) != 0 || r.ax != 0x004F)
    {
      return;
    }

  if (low_rd32 (VBE_INFO_OFF + VBE_SIG) != 0x41534556UL) /* "VESA" */
    {
      return;
    }

  /* VideoModePtr is a real mode far pointer: segment in the high half. */
  listp = low_rd32 (VBE_INFO_OFF + VBE_MODEPTR);
  listp = ((listp >> 16) << 4) + (listp & 0xFFFFUL);

  if (listp == 0)
    {
      return;
    }

  for (i = 0; i < 512 && vesa_count < VID_VESA_MAX; i++)
    {
      unsigned number = *ABS_U16 (listp + i * 2);
      unsigned attrs, bpp, model;
      struct vesa_mode *v;

      if (number == 0xFFFF)
        {
          break;
        }

      regs_clear (&r);
      r.ax = 0x4F01;
      r.cx = (unsigned short)number;
      r.es = (unsigned short)(VIDLOW_XFER >> 4);
      r.di = (unsigned short)(VBE_MINFO_OFF + (VIDLOW_XFER & 0x0F));

      if (vid_int10 (&r) != 0 || r.ax != 0x004F)
        {
          continue;
        }

      attrs = low_rd16 (VBE_MINFO_OFF + VBEM_ATTRS);

      if ((attrs & VBEA_SUPPORTED) == 0 || (attrs & VBEA_GRAPHICS) == 0
          || (attrs & VBEA_LFB) == 0)
        {
          continue;
        }

      bpp = low_rd8 (VBE_MINFO_OFF + VBEM_BPP);
      model = low_rd8 (VBE_MINFO_OFF + VBEM_MEMMODEL);

      /* 4 = packed pixel, 6 = direct colour.  Anything else is planar. */
      if ((model != 4 && model != 6) || (bpp != 8 && bpp != 15 && bpp != 16
                                         && bpp != 24 && bpp != 32))
        {
          continue;
        }

      v = &vesatab [vesa_count];
      v->number = (unsigned short)number;
      v->width = (unsigned short)low_rd16 (VBE_MINFO_OFF + VBEM_WIDTH);
      v->height = (unsigned short)low_rd16 (VBE_MINFO_OFF + VBEM_HEIGHT);
      v->bpp = (unsigned short)bpp;
      v->pitch = low_rd16 (VBE_MINFO_OFF + VBEM_PITCH);
      v->phys = low_rd32 (VBE_MINFO_OFF + VBEM_PHYSBASE);

      if (v->width == 0 || v->height == 0 || v->pitch == 0 || v->phys == 0)
        {
          continue;
        }

      v->size = v->pitch * (unsigned long)v->height;
      vesa_count++;
    }
}

/*****************************************************************************/

static unsigned
set_hw_vesa (unsigned idx)
{
  struct vid_regs r;
  struct vesa_mode *v;

  if (idx >= vesa_count)
    {
      return VIDR_BADMODE;
    }

  v = &vesatab [idx];

  pmode_set_fb (0, 0);

  regs_clear (&r);
  r.ax = 0x4F02;
  r.bx = (unsigned short)(v->number | 0x4000); /* bit 14: linear */

  if (vid_int10 (&r) != 0 || r.ax != 0x004F)
    {
      return VIDR_FAILED;
    }

  pmode_set_fb (v->phys, v->size);

  return VIDR_OK;
}

/*****************************************************************************/

/*
 * DAC entries.  The VGA takes 6 bits per channel; a value is written as
 * index then red, green, blue in sequence.
 */

unsigned
vidmode_palette (const unsigned char *rgb, unsigned first, unsigned count)
{
  unsigned i;

  if (!have_vga)
    {
      return VIDR_NOHW;
    }

  if (!rgb || count == 0 || first > 255 || count > 256 - first)
    {
      return VIDR_BADFONT;
    }

  outb (0x3C8, (unsigned char)first);

  for (i = 0; i < count * 3u; i++)
    {
      outb (0x3C9, (unsigned char)(rgb [i] & 0x3F));
    }

  return VIDR_OK;
}

/*****************************************************************************/

void
vidmode_init (void)
{
  unsigned bios_mode;
  unsigned cols, rows;
  unsigned i;

  have_vga = probe_vga ();
  vesa_probe ();

  /*
   * Work out where the BIOS left us without changing anything: booting
   * must not flash the display through a mode set.
   */

  bios_mode = vid_bios_mode ();
  cols = vgacon_cols ();
  rows = vgacon_rows ();

  vid_committed = VID_MODE_80X25;

  /*
   * Bound by the static table, not VID_MODE_COUNT: the id space now runs
   * up through the VBE range, but modetab only has the static entries.
   * Graphics entries are skipped outright - the console mode has to be
   * something the text driver can print on.
   */

  for (i = 0; i < VID_MODE_STATIC; i++)
    {
      if (modetab [i].width != 0)
        {
          continue;
        }

      if (modetab [i].bios_mode == bios_mode && modetab [i].cols == cols
          && modetab [i].rows == rows)
        {
          vid_committed = i;

          break;
        }
    }

  vid_current = vid_committed;
  has_provisional = 0;
}

/*****************************************************************************/

unsigned
vidmode_describe (unsigned idx, struct vidmode_info *out)
{
  int i;

  if (!out)
    {
      return VIDR_NOMORE;
    }

  for (i = 0; i < (int)sizeof (*out); i++)
    {
      ((unsigned char *)out) [i] = 0;
    }

  if (idx >= VID_MODE_VESA_BASE)
    {
      struct vesa_mode *v;

      if (idx - VID_MODE_VESA_BASE >= vesa_count)
        {
          return VIDR_NOMORE;
        }

      v = &vesatab [idx - VID_MODE_VESA_BASE];
      out->mode = idx;
      out->width = v->width;
      out->height = v->height;
      out->bpp = v->bpp;
      out->pitch = v->pitch;
      out->fb_size = v->size;
      out->fb_phys = v->phys;
      out->flags = VIDM_GRAPHICS | VIDM_LFB | VIDM_VESA;

      if (idx == vid_current)
        {
          out->flags |= VIDM_CURRENT;
        }

      return VIDR_OK;
    }

  if (idx >= VID_MODE_STATIC)
    {
      return VIDR_NOMORE;
    }

  /* Modes the adapter cannot do are not offered at all. */
  if (modetab [idx].needs_vga && !have_vga)
    {
      return VIDR_NOMORE;
    }

  out->mode = idx;

  if (modetab [idx].width)
    {
      out->width = modetab [idx].width;
      out->height = modetab [idx].height;
      out->bpp = modetab [idx].bpp;
      out->pitch = modetab [idx].width;
      out->fb_size = VGA_GFX_SIZE;
      out->fb_phys = VGA_GFX_BASE;
      out->flags = VIDM_GRAPHICS;

      if (idx == vid_current)
        {
          out->flags |= VIDM_CURRENT;
        }

      return VIDR_OK;
    }

  out->cols = modetab [idx].cols;
  out->rows = modetab [idx].rows;
  out->cell_h = (idx == vid_current) ? vgacon_cell_h () : 0;
  out->flags = VIDM_TEXT;

  if (idx == vid_current)
    {
      out->flags |= VIDM_CURRENT;
    }

  if (idx == vid_committed)
    {
      out->flags |= VIDM_CONSOLE;
    }

  if (idx == VID_MODE_80X25)
    {
      out->flags |= VIDM_DEFAULT;
    }

  return VIDR_OK;
}

/*****************************************************************************/

/*
 * Enumeration by dense ordinal rather than by mode id.  The id space has
 * gaps - VBE modes start well above the static ones, and unsupported
 * entries are skipped - so a caller walking ids would stop at the first
 * hole and never see the VBE list.  The ordinal is what BDOS 230 takes;
 * the real id comes back in .mode.
 */

unsigned
vidmode_enum (unsigned ordinal, struct vidmode_info *out)
{
  unsigned id;
  unsigned seen = 0;

  for (id = 0; id < VID_MODE_STATIC; id++)
    {
      if (vidmode_describe (id, out) != VIDR_OK)
        {
          continue;
        }

      if (seen++ == ordinal)
        {
          return VIDR_OK;
        }
    }

  for (id = VID_MODE_VESA_BASE; id < VID_MODE_VESA_BASE + vesa_count; id++)
    {
      if (vidmode_describe (id, out) != VIDR_OK)
        {
          continue;
        }

      if (seen++ == ordinal)
        {
          return VIDR_OK;
        }
    }

  return VIDR_NOMORE;
}

/*****************************************************************************/

unsigned
vidmode_action (unsigned mode, unsigned action)
{
  unsigned r;

  if (!have_vga && mode != VID_MODE_80X25 && mode != VID_MODE_40X25)
    {
      return VIDR_NOHW;
    }

  /*
   * Only a text mode can become the console; the console has to be
   * something the text driver can actually print on.
   */

  if (action == VIDA_CONSOLE && mode >= VID_MODE_STATIC)
    {
      return VIDR_BADMODE;
    }

  if (action == VIDA_CONSOLE && mode < VID_MODE_STATIC
      && modetab [mode].width != 0)
    {
      return VIDR_BADMODE;
    }

  switch (action)
    {
    case VIDA_COMMIT:
      if (has_provisional)
        {
          vid_committed = vid_provisional;
          has_provisional = 0;
        }

      return VIDR_OK;

    case VIDA_REVERT:
      has_provisional = 0;

      if (vid_current != vid_committed)
        {
          r = set_hw (vid_committed);

          if (r != VIDR_OK)
            {
              return r;
            }

          vid_current = vid_committed;
        }

      return VIDR_OK;

    case VIDA_TRANSIENT:
    case VIDA_CONSOLE:
      r = set_hw (mode);

      if (r != VIDR_OK)
        {
          return r;
        }

      vid_current = mode;

      /*
       * A console mode is only provisional until it is committed.  If the
       * caller never gets that far - it crashed, it was killed, the user
       * did not answer the confirmation - the mode is dropped when the
       * program leaves.
       */

      if (action == VIDA_CONSOLE)
        {
          vid_provisional = mode;
          has_provisional = 1;
        }

      return VIDR_OK;

    default:
      return VIDR_BADMODE;
    }
}

/*****************************************************************************/

/*
 * Glyph memory lives in plane 2, which is not reachable in the normal text
 * addressing mode.  The sequence below unchains the planes and maps 64K at
 * 0xA0000 so plane 2 can be written directly, then puts everything back.
 * This is standard VGA and behaves identically on every card - unlike CRTC
 * timing, which is why the font path does not go through the BIOS.
 *
 * Each glyph occupies 32 bytes regardless of how many scan lines it uses.
 */

#define FONT_PLANE ((volatile unsigned char *)0xA0000UL)
#define FONT_STRIDE 32

static void
font_access_on (void)
{
  outb (0x3C4, 0x00);
  outb (0x3C5, 0x01); /* synchronous reset                    */
  outb (0x3C4, 0x02);
  outb (0x3C5, 0x04); /* map mask: plane 2 only               */
  outb (0x3C4, 0x04);
  outb (0x3C5, 0x07); /* extended memory, not odd/even        */
  outb (0x3C4, 0x00);
  outb (0x3C5, 0x03); /* release reset                        */

  outb (0x3CE, 0x04);
  outb (0x3CF, 0x02); /* read map select: plane 2             */
  outb (0x3CE, 0x05);
  outb (0x3CF, 0x00); /* write mode 0, no odd/even            */
  outb (0x3CE, 0x06);
  outb (0x3CF, 0x04); /* 0xA0000, 64K, graphics addressing    */
}

/*****************************************************************************/

static void
font_access_off (void)
{
  outb (0x3C4, 0x00);
  outb (0x3C5, 0x01);
  outb (0x3C4, 0x02);
  outb (0x3C5, 0x03); /* map mask: planes 0 and 1             */
  outb (0x3C4, 0x04);
  outb (0x3C5, 0x03); /* odd/even, chained                    */
  outb (0x3C4, 0x00);
  outb (0x3C5, 0x03);

  outb (0x3CE, 0x04);
  outb (0x3CF, 0x00);
  outb (0x3CE, 0x05);
  outb (0x3CF, 0x10); /* odd/even                             */
  outb (0x3CE, 0x06);
  outb (0x3CF, 0x0E); /* 0xB8000, 32K, text addressing        */
}

/*****************************************************************************/

unsigned
vidmode_font_load (const unsigned char *data, unsigned height, unsigned count,
                   unsigned first)
{
  unsigned g, y;

  if (!have_vga)
    {
      return VIDR_NOHW;
    }

  /* No data means "put the stock font back", which a mode set does. */
  if (!data)
    {
      unsigned r = set_hw (vid_committed);

      if (r != VIDR_OK)
        {
          return r;
        }

      vid_current = vid_committed;

      return VIDR_OK;
    }

  if (height == 0 || height > 32 || count == 0 || first > 255
      || count > 256 - first)
    {
      return VIDR_BADFONT;
    }

  /*
   * Refuse a height the current mode is not using.  Changing it would mean
   * reprogramming the CRTC and recomputing the row count behind the mode
   * table's back; selecting a mode with the right cell height first keeps
   * the two concerns apart.
   */

  if (height != vgacon_cell_h ())
    {
      return VIDR_BADFONT;
    }

  font_access_on ();

  for (g = 0; g < count; g++)
    {
      volatile unsigned char *dst
          = FONT_PLANE + (unsigned long)(first + g) * FONT_STRIDE;

      for (y = 0; y < height; y++)
        {
          dst [y] = data [g * height + y];
        }

      /* Blank the unused scan lines of the cell. */
      for (y = height; y < FONT_STRIDE; y++)
        {
          dst [y] = 0;
        }
    }

  font_access_off ();

  return VIDR_OK;
}

/*****************************************************************************/

void
vidmode_restore_console (void)
{
  has_provisional = 0;

  if (vid_current == vid_committed)
    {
      return;
    }

  /*
   * A restore that fails leaves the user staring at nothing, so fall all
   * the way back to plain 80x25 and make that the console mode from now
   * on rather than trying the broken one again.
   */

  if (set_hw (vid_committed) != VIDR_OK)
    {
      vid_committed = VID_MODE_80X25;

      if (set_hw (VID_MODE_80X25) != VIDR_OK)
        {
          return;
        }
    }

  vid_current = vid_committed;
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
