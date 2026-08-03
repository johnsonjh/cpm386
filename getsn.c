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
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30

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
  (void)bdos (2, (LONG)c);
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
puthex48 (const unsigned char *sn)
{
  static const char hex [] = "0123456789ABCDEF";
  int i;

  for (i = 0; i < 3; ++i)
    {
      unsigned char c = sn [i];

      putch (hex [(c >> 4) & 0xF]);
      putch (hex [c & 0xF]);
    }

  putch ('-');

  for (i = 3; i < 6; ++i)
    {
      unsigned char c = sn [i];

      putch (hex [(c >> 4) & 0xF]);
      putch (hex [c & 0xF]);
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  const char msg1 [] = "Serial Number: ";
  unsigned char sn [6];
  int printable = 1;
  int i;

  for (i = 0; i < 6; ++i)
    {
      sn [i] = '?';
    }

  (void)bdos (107, (LONG)sn);

  for (i = 0; i < 6; ++i)
    {
      if (sn [i] < 0x20 || sn [i] > 0x7E)
        {
          printable = 0;

          break;
        }
    }

  puts (msg1);

  if (printable)
    {
      for (i = 0; i < 6; ++i)
        {
          putch (sn [i]);
        }

      puts (" (");
      puthex48 (sn);
      puts (")");
    }
  else
    {
      puthex48 (sn);
    }

  puts ("\r\n");

  (void)bdos (0, 0);

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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
