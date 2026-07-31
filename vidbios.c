/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 8a43acc4-8caf-11f1-8b93-80ee73e9b8e7
 */

/*****************************************************************************/

/* vidbios.c - C side of the real mode int 10h thunk */

/*****************************************************************************/

#include "absaddr.h"
#include "vidbios.h"

/*****************************************************************************/

extern void vid_thunk_call (void);
extern unsigned char vid_blob_start [];
extern unsigned char vid_blob_end [];

/*****************************************************************************/

static int thunk_ready;
static int thunk_busy;

/*****************************************************************************/

int
vid_thunk_busy (void)
{
  return thunk_busy;
}

/*****************************************************************************/

void
vidbios_init (void)
{
  unsigned long len = (unsigned long)(vid_blob_end - vid_blob_start);
  unsigned long i;

  thunk_ready = 0;

  /*
   * A blob that outgrew its slot would run into the register block, so
   * refuse rather than corrupt it.  512 bytes is generous; the transition
   * is well under 150.
   */

  if (len == 0 || len > VIDLOW_CODE_MAX)
    {
      return;
    }

  for (i = 0; i < len; i++)
    {
      *ABS_U8 (VIDLOW_CODE + i) = vid_blob_start [i];
    }

  thunk_ready = 1;
}

/*****************************************************************************/

int
vid_int10 (struct vid_regs *r)
{
  if (!thunk_ready || thunk_busy || !r)
    {
      return -1;
    }

  thunk_busy = 1;

  *ABS_U16 (VIDLOW_DATA + 0) = r->ax;
  *ABS_U16 (VIDLOW_DATA + 2) = r->bx;
  *ABS_U16 (VIDLOW_DATA + 4) = r->cx;
  *ABS_U16 (VIDLOW_DATA + 6) = r->dx;
  *ABS_U16 (VIDLOW_DATA + 8) = r->si;
  *ABS_U16 (VIDLOW_DATA + 10) = r->di;
  *ABS_U16 (VIDLOW_DATA + 12) = r->bp;
  *ABS_U16 (VIDLOW_DATA + 14) = r->es;
  *ABS_U16 (VIDLOW_DATA + 16) = r->ds;
  *ABS_U16 (VIDLOW_DATA + 18) = 0;

  vid_thunk_call ();

  r->ax = *ABS_U16 (VIDLOW_DATA + 0);
  r->bx = *ABS_U16 (VIDLOW_DATA + 2);
  r->cx = *ABS_U16 (VIDLOW_DATA + 4);
  r->dx = *ABS_U16 (VIDLOW_DATA + 6);
  r->si = *ABS_U16 (VIDLOW_DATA + 8);
  r->di = *ABS_U16 (VIDLOW_DATA + 10);
  r->bp = *ABS_U16 (VIDLOW_DATA + 12);
  r->es = *ABS_U16 (VIDLOW_DATA + 14);
  r->flags = *ABS_U16 (VIDLOW_DATA + 18);

  thunk_busy = 0;

  return 0;
}

/*****************************************************************************/

/*
 * int 10h AH=0Fh - get current video mode.  Read-only, so this is the
 * smoke test: if it comes back with a plausible mode number the whole
 * protected/real transition works on this machine.
 */

unsigned
vid_bios_mode (void)
{
  struct vid_regs r;
  int i;

  for (i = 0; i < (int)sizeof (r); i++)
    {
      ((unsigned char *)&r) [i] = 0;
    }

  r.ax = 0x0F00;

  if (vid_int10 (&r) != 0)
    {
      return 0xFFFF;
    }

  return (unsigned)(r.ax & 0x00FF);
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
