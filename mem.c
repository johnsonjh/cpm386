/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: e5cc02ee-82b4-11f1-ae70-80ee73e9b8e7
 */

/*****************************************************************************/

/* mem.c */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_TPA 63

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

/* Must match kernel set_tpa_struct layout */
struct tpa_req
{
  UWORD parms; /* 0 = get, bit0 = set, bit1 = sticky */
  UWORD _pad;
  ULONG low; /* BYTE * in kernel */
  ULONG high;
};

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
putu (ULONG n)
{
  char b [12];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 12)
    {
      b [i++] = (char)('0' + (n % 10));
      n /= 10;
    }

  while (i)
    {
      putch (b [--i]);
    }
}

/*****************************************************************************/

static void
puthex32 (ULONG v)
{
  static const char h [] = "0123456789ABCDEF";
  int i;

  puts ("0x");

  for (i = 7; i >= 0; i--)
    {
      putch (h [(v >> (i * 4)) & 0xF]);
    }
}

/*****************************************************************************/

/* Print n bytes as KB (rounded down) with unit. */
static void
put_kb (ULONG bytes)
{
  putu (bytes / 1024UL);
  puts ("K");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct tpa_req t;
  ULONG base, top, tpa_len, free_prog, ramdisk_size, kernel_size;

  t.parms = 0; /* get */
  t._pad = 0;
  t.low = 0;
  t.high = 0;

  (void)bdos (BDOS_TPA, (LONG)(ULONG)&t);

  base = t.low;
  top = t.high;

  if (top < base)
    {
      top = base;
    }

  tpa_len = top - base;

  /*
   * Program load at TPA+0x100; ring-3 stack uses up to ~1M at the top
   * of the TPA (see pgm_enter).  Report bulk free as TPA minus base page.
   */

  free_prog = (tpa_len > 0x100UL) ? (tpa_len - 0x100UL) : 0;

  puts ("\r\nCP/M-386 Memory Map\r\n");
  puts (    "-------------------\r\n");

  /* Usable RAM is [0 .. TPA top); TPA is the transient region. */
  puts ("Usable RAM top:        ");
  puthex32 (top);
  puts (" (");
  put_kb (top);
  puts (" from 0)\r\n");

  ramdisk_size = (ULONG)bdos (227, 0) * 1024UL;
  kernel_size = base - ramdisk_size;

  puts ("System CBIOS/BDOS/CCP: ");
  puthex32 (0);
  puts (" -> ");
  puthex32 (kernel_size);
  puts (" (");
  put_kb (kernel_size);
  puts (")\r\n");

  if (ramdisk_size > 0)
    {
      puts ("Initial RAM Disk:      ");
      puthex32 (kernel_size);
      puts (" -> ");
      puthex32 (base);
      puts (" (");
      put_kb (ramdisk_size);
      puts (")\r\n");
    }

  puts ("TPA base:              ");
  puthex32 (base);
  puts (" (");
  put_kb (base);
  puts (")\r\n");
  puts ("TPA top:               ");
  puthex32 (top);
  puts (" (");
  put_kb (top);
  puts (")\r\n");

  puts ("TPA size:              ");
  put_kb (tpa_len);
  puts (" (");
  putu (tpa_len);
  puts (" bytes)\r\n");

  puts ("Program load:          TPA+0x100\r\n");
  puts ("Approximate free RAM:  ");
  put_kb (free_prog);
  puts (" (TPA minus base page)\r\n");

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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
