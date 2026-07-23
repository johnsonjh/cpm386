/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 5537d2e2-830b-11f1-93ce-80ee73e9b8e7
 */

/*****************************************************************************/

/* hello.c test program for ring-3 loader */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;

/*****************************************************************************/

#define BDOS_INT 0x30

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
_start (void) /*cppcheck-suppress unusedFunction*/
{
  /* frame-relative locals: correct after load at any TPA base */
  char msg1[] = "\r\nRing-3 loader test: Hello from TPA+0x100!\r\n";
  char msg2[] = "Exiting via int 0x30 BDOS(0)...\r\n";
  char *p;

  for (p = msg1; *p; p++)
    {
      bdos (2, (LONG)*p);
    }

  for (p = msg2; *p; p++)
    {
      bdos (2, (LONG)*p);
    }

  bdos (0, 0);

  /* not reached! */
  for (;;)
    {
      ;
    }
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
