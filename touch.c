/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: e48c929e-82b5-11f1-a4ac-80ee73e9b8e7
 */

/*****************************************************************************/

/* touch.c: create empty file if missing */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func), "i"(BDOS_INT),
                      "d"((unsigned long)info)
                    : "memory", "cc");
  return ret;
}

/*****************************************************************************/

void
_start (void)
{
  UBYTE fcb[36];
  UWORD r;
  int i;

  /* Need a filename in the default FCB */
  if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
    {
      bdos (0, 0);
    }

  for (i = 0; i < 36; i++)
    {
      fcb[i] = DEF_FCB[i];
    }

  fcb[12] = 0; /* extent */
  fcb[14] = 0; /* s2 */
  fcb[32] = 0;

  /* Already there? leave it */
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r <= 3)
    {
      bdos (16, (LONG)(unsigned long)fcb); /* close */
      bdos (0, 0);
    }

  /* Create empty file (no data records) */
  for (i = 0; i < 36; i++)
    {
      fcb[i] = DEF_FCB[i];
    }

  fcb[12] = 0;
  fcb[13] = 0; /* s1 / LRBC */
  fcb[14] = 0;
  fcb[15] = 0; /* rc */
  fcb[32] = 0;
  r = bdos (22, (LONG)(unsigned long)fcb);

  if (r <= 3)
    {
      bdos (16, (LONG)(unsigned long)fcb);
    }

  bdos (0, 0);
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
