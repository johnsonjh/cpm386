/*
 * CP/M-386 - test110.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: d4862434-9703-11f1-9ef5-80ee73e9b8e7
 */

/*****************************************************************************/

/* test110.c - test program for BDOS 110 (C_DELIMIT) */

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

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  const char test1[] = "TEST: This should print!# BUT NOT THIS! - FAIL$#";
  const char test2[] = "TEST: This should print!$ BUT NOT THIS! - FAIL$#";

  UWORD old_delim = bdos (110, 0xFFFF);
  UWORD new_delim;

  if (old_delim != '$')
    {
      puts ("FAIL: Invalid default delimiter '");
      putch (old_delim);
      puts ("'\r\n");

      (void)bdos (0, 0);
    }

  puts ("Setting delimiter to '#'.\r\n");

  bdos (110, (LONG)'#');
  bdos (9, (LONG)test1);
  puts ("\r\n");

  puts ("Resetting delimiter to '$'.\r\n");

  bdos (110, (LONG)'$');
  bdos (9, (LONG)test2);
  puts ("\r\n");

  puts ("Reading back delimiter: ");

  new_delim = bdos (110, 0xFFFF);

  if (new_delim == '$')
    {
      puts ("Got '");
      putch (new_delim);
      puts ("' - PASS\r\n");
    }
  else
    {
      puts ("Got '");
      putch (new_delim);
      puts ("' - FAIL\r\n");
    }

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
