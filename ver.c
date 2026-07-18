/* ver.c - print OS / BDOS version */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30

/*****************************************************************************/

/* Matches bdosdef.h VERSION - CP/M-68K lineage, used here for CP/M-386! */
#define VER_CPM386 0x2022

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

/* Decimal (original print_uint16 with bx=10); version nibbles are 0..15. */
static void
putu (unsigned n)
{
  char b[6];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 6)
    {
      b[i++] = (char)('0' + n % 10);
      n /= 10;
    }

  while (i)
    {
      putch (b[--i]);
    }
}

/*****************************************************************************/

/* Print BDOS version from low byte as major.minor (high/low nibble). */
static void
print_bdos_ver (UWORD ver)
{
  UBYTE lo = (UBYTE)(ver & 0xff);

  putu ((unsigned)((lo >> 4) & 0x0f));
  putch ('.');
  putu ((unsigned)(lo & 0x0f));
}

/*****************************************************************************/

/*
 * Original also calls BDOS 0xA3 for MPM/CDOS/DOS Plus OS product version.
 * That call is not implemented on CP/M-386; those branches are kept for
 * completeness if VERSION is ever set to those codes.
 */

static void
print_os_product_ver (void)
{
  UWORD r = bdos (0xA3, 0);
  UBYTE v = (UBYTE)(r & 0xff);

  putu ((unsigned)((v >> 4) & 0x0f));
  putch ('.');
  putu ((unsigned)(v & 0x0f));
  puts (", ");
}

/*****************************************************************************/

void
_start (void)
{
  UWORD ver = bdos (12, 0);
  UBYTE ah = (UBYTE)((ver >> 8) & 0xff);

  /*
   * Order matches ver.a86; CP/M-386 recognized first for our VERSION.
   * Product release (0.1) comes from BDOS 163 S_OSVER when available.
   */

  if (ver == VER_CPM386)
    {
      puts ("CP/M-386 ");
      print_os_product_ver (); /* 163 -> "0.1, " */
    }
  else if (ah == 0x15)
    {
      puts ("MP/M-86 ");
      print_os_product_ver ();
    }
  else if (ver == 0x1430 || ver == 0x1431)
    {
      puts ("Concurrent CP/M-86 ");
      print_os_product_ver ();
    }
  else if (ver >= 0x1466)
    {
      puts ("Multiuser DOS ");
      print_os_product_ver ();
    }
  else if (ver >= 0x1450)
    {
      puts ("Concurrent DOS XM ");
      print_os_product_ver ();
    }
  else if (ah == 0x14)
    {
      puts ("Concurrent DOS ");
      print_os_product_ver ();
    }
  else if (ver == 0x1041)
    {
      puts ("DOS Plus ");
      print_os_product_ver ();
    }
  else if (ver == 0x0022)
    {
      puts ("CP/M-86 1.1, ");
    }
  else
    {
      puts ("CP/M-86, ");
    }

/*****************************************************************************/

  puts ("BDOS ");
  print_bdos_ver (ver);
  puts ("\r\n");

  bdos (0, 0);
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
