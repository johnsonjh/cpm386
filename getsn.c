/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 5aaa331e-830b-11f1-8bd5-80ee73e9b8e7
 */

/*****************************************************************************/

/* Get serial number - BDOS 107 */

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

static void
puts (const char *s)
{
  while (*s)
    {
      bdos (2, (LONG)*s++);
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  const char msg1[] = "Serial Number: ";
  char sn[7];

  sn[0] = '?';
  sn[1] = '?';
  sn[2] = '?';
  sn[3] = '?';
  sn[4] = '?';
  sn[5] = '?';
  sn[6] = '\0';

  bdos (107, (LONG)sn);

  puts (msg1);
  puts (sn);
  puts ("\r\n");

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
