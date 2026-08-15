/*
 * CP/M-386 - pause.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 2f1f4802-82b5-11f1-a3cf-80ee73e9b8e7
 */

/*****************************************************************************/

/* pause.c: wait for a key */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define PROMPT_COLS 40

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret = 0;

  (void)func;
  (void)info;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func),
                      "i"(BDOS_INT),
                      "d"((ULONG)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

static void
putch (char c)
{
  (void)bdos (2, (LONG)(unsigned char)c);
}

/*****************************************************************************/

static void
puts (const char *s)
{
  while (*s)
    {
      putch (*s++);
    }
}

/*****************************************************************************/

static void
clear_line (void)
{
  int i;

  /*
   * Portable clear: CR, blank the prompt, CR again.
   * Dual-console bios_conout paints both serial and VGA.
   */

  putch ('\r');
  for (i = 0; i < PROMPT_COLS; i++)
    {
      putch (' ');
    }

  putch ('\r');
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  int c;

  puts ("Press any key to continue...");

  /* Wait for a key without echo (raw console) */
  while (!(c = (int)bdos (6, 0xFF)))
    {
      ;
    }

  (void)c;
  clear_line ();

  (void)bdos (0, 0);
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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
