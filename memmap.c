/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: cc3b4f86-8c84-11f1-b6e6-80ee73e9b8e7
 */

/*****************************************************************************/

/* memmap.c - A20 and RAM verification shared by both boot paths */

/*****************************************************************************/

#include "absaddr.h"
#include "memmap.h"

/*****************************************************************************/

/* A20 wrap test */

int
mem_a20_enabled (void)
{
  volatile unsigned int *lo = ABS_U32 (0x000500);
  volatile unsigned int *hi = ABS_U32 (0x100500);
  unsigned int save_lo = *lo;
  unsigned int save_hi = *hi;
  int enabled;

  *lo = 0x0000FFFFu;
  *hi = 0xFFFF0000u;
  enabled = (*lo != *hi);

  *hi = save_hi;
  *lo = save_lo;

  return enabled;
}

/*****************************************************************************/

#define PROBE_STEP 0x10000UL /* 64K */
#define PROBE_KEY 0xA5A5A5A5UL

/*****************************************************************************/

static unsigned int
probe_val (unsigned long a)
{
  return (unsigned int)(a ^ PROBE_KEY);
}

/*****************************************************************************/

unsigned long
mem_verify_region (unsigned long base, unsigned long top)
{
  unsigned long a;
  unsigned long last;

  base = (base + 3UL) & ~3UL;
  top &= ~3UL;

  if (top < base + 4UL)
    {
      return base;
    }

  /* Last dword of the region, so short final block is covered */
  last = top - 4UL;

  for (a = base; a < last; a += PROBE_STEP)
    {
      *ABS_U32 (a) = probe_val (a);
    }

  *ABS_U32 (last) = probe_val (last);

  for (a = base; a < last; a += PROBE_STEP)
    {
      if (*ABS_U32 (a) != probe_val (a))
        {
          return a;
        }
    }

  if (*ABS_U32 (last) != probe_val (last))
    {
      return (last & ~(PROBE_STEP - 1UL));
    }

  return top;
}

/*****************************************************************************/

unsigned long
mem_get_boot_region (unsigned long *base, unsigned long *top)
{
  unsigned long b, t, flags;

  *base = 0;
  *top = 0;

  if (*ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_MAGIC) != (unsigned int)MEMMAP_MAGIC)
    {
      return 0;
    }

  b = *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_BASE);
  t = *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_TOP);
  flags = *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_FLAGS);

  if (b < TPA_MIN_BASE)
    {
      b = TPA_MIN_BASE;
    }

  if (t > TPA_MAX_TOP)
    {
      t = TPA_MAX_TOP;
    }

  if (t <= b)
    {
      return 0;
    }

  *base = b;
  *top = t;

  return flags;
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
