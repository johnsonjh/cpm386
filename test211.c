/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 3c303faa-830b-11f1-b015-80ee73e9b8e7
 */

/*****************************************************************************/

/* BDOS 211 test program (print decimal) */

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
  char msg1[] = "\r\nTesting BDOS 211 (Print decimal number):\r\n";
  char msg2[] = " -> ";
  char newline[] = "\r\n";
  char *p;
  int i;
  UWORD tests[9];

  tests[0] = 0;
  tests[1] = 7;
  tests[2] = 42;
  tests[3] = 1234;
  tests[4] = 65535;
  tests[5] = -1;    /* 65535 */
  tests[6] = 4321;
  tests[7] = 0;
  tests[8] = 7;

  for (p = msg1; *p; p++)
    {
      bdos (2, (LONG)*p);
    }

  for (i = 0; i < 9; i++)
    {
      for (p = msg2; *p; p++)
        {
          bdos (2, (LONG)*p);
        }

      bdos (211, (LONG)tests[i]);

      for (p = newline; *p; p++)
        {
          bdos (2, (LONG)*p);
        }
    }

  bdos (0, 0);

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
