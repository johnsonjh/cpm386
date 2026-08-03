/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 77d66392-82b4-11f1-bcfb-80ee73e9b8e7
 */

/*****************************************************************************/

/* conctl.c - builds VGAON / VGAOFF / SERON / SEROFF for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_CON_VGA 222
#define BDOS_CON_SER 223

/*****************************************************************************/

/* BDOS 222/223 status codes; must match bios.c */

#define CON_OFF 0x00
#define CON_ON 0x01
#define CON_ABSENT 0xFE /* no such adapter fitted, request ignored */
#define CON_LAST 0xFF   /* refused, would leave no console at all  */

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
report (UWORD r, const char *dev, int turning_on)
{
  puts (dev);

  switch (r)
    {
    case CON_ABSENT:
      puts (" console not present\r\n");

      break;

    case CON_LAST:
      puts (turning_on ? " console ON failed\r\n"
                       : " console OFF refused (only console)\r\n");

      break;

    case CON_ON:
      puts (" console ON\r\n");

      break;

    case CON_OFF:
      puts (" console OFF\r\n");

      break;

    default:
      puts (" console: unexpected status\r\n");

      break;
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UWORD r;

#if defined(PROG_VGAON)
  r = bdos (BDOS_CON_VGA, 1);
  report (r, "VGA", 1);

#elif defined(PROG_VGAOFF)
  if (bdos (BDOS_CON_VGA, 0xFFFF) == CON_ON
      && bdos (BDOS_CON_SER, 0xFFFF) == CON_ON)
    {
      puts ("VGA console OFF\r\n");
    }

  r = bdos (BDOS_CON_VGA, 0);

  if (r != CON_OFF)
    {
      report (r, "VGA", 0);
    }

#elif defined(PROG_SERON)
  r = bdos (BDOS_CON_SER, 1);
  report (r, "Serial", 1);

#elif defined(PROG_SEROFF)
  if (bdos (BDOS_CON_SER, 0xFFFF) == CON_ON
      && bdos (BDOS_CON_VGA, 0xFFFF) == CON_ON)
    {
      puts ("Serial console OFF\r\n");
    }

  r = bdos (BDOS_CON_SER, 0);

  if (r != CON_OFF)
    {
      report (r, "Serial", 0);
    }

#else
# error "Define PROG_VGAON, PROG_VGAOFF, PROG_SERON, or PROG_SEROFF!"
#endif

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
