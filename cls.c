/* cls.c - clear screen */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_CLS 221

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
  bdos (BDOS_CLS, 0);

  bdos (0, 0);
}

/*****************************************************************************/
