/* reboot.c */

/*****************************************************************************/

/*
 * Usage:
 *   REBOOT       cold reboot (BIOS reset flag 0)
 *   REBOOT W     warm reboot (BIOS data area 40:72 = 0x1234)
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_REBOOT 220

/*****************************************************************************/

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

static void
putch (char c)
{
  bdos (2, (LONG)(unsigned char)c);
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
_start (void)
{
  UBYTE ch = DEF_FCB[1];
  int warm = 0;

  if (ch == 'W' || ch == 'w')
    {
      warm = 1;
    }

  puts (warm ? "Warm reboot...\r\n" : "Cold reboot...\r\n");

  bdos (BDOS_REBOOT, (LONG)warm);

  puts ("Reboot not supported\r\n");

  bdos (0, 0);
}

/*****************************************************************************/
