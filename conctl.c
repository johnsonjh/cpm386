/* conctl.c - builds VGAON / VGAOFF / SERON / SEROFF for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_CON_VGA 222
#define BDOS_CON_SER 223

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
  UWORD r;
  WORD func;
  LONG arg;

#if defined(PROG_VGAON)
  func = BDOS_CON_VGA;
  arg = 1;
  r = bdos (func, arg);
  puts (r == 0xFF ? "VGA ON failed\r\n" : "VGA console ON\r\n");

#elif defined(PROG_VGAOFF)
  func = BDOS_CON_VGA;
  arg = 0;
  /* Announce while VGA still enabled so both paths may show it */
  puts ("VGA console OFF\r\n");
  r = bdos (func, arg);

  if (r == 0xFF)
    {
      puts ("VGA OFF refused (last console?)\r\n");
    }

#elif defined(PROG_SERON)
  func = BDOS_CON_SER;
  arg = 1;
  r = bdos (func, arg);
  puts (r == 0xFF ? "Serial ON failed\r\n" : "Serial console ON\r\n");

#elif defined(PROG_SEROFF)
  func = BDOS_CON_SER;
  arg = 0;
  /* Announce on serial before disabling it */
  puts ("Serial console OFF\r\n");
  r = bdos (func, arg);

  if (r == 0xFF)
    {
      puts ("Serial OFF refused (last console?)\r\n");
    }

#else
# error "Define PROG_VGAON, PROG_VGAOFF, PROG_SERON, or PROG_SEROFF!"
#endif

  bdos (0, 0);
}

/*****************************************************************************/
