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
#define BDOS_MEMLAYOUT 228

/*****************************************************************************/

/* MEMF_* from memmap.h */

#define MEMF_E820 0x0001
#define MEMF_E801 0x0002
#define MEMF_88 0x0004
#define MEMF_MBMMAP 0x0008
#define MEMF_MBBASIC 0x0010
#define MEMF_A20 0x0020

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

/* Must match struct cpm_memlayout in bdosdef.h */
struct memlayout
{
  ULONG kernel_base;
  ULONG kernel_end;
  ULONG ramdisk_base;
  ULONG ramdisk_size;
  ULONG lowmem_top;
  ULONG tpa_base;
  ULONG tpa_top;
  ULONG stack_top;
  ULONG mem_flags;
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

/* "start -> end (nnnK)" region line. */
static void
put_region (const char *label, ULONG start, ULONG end)
{
  puts (label);
  puthex32 (start);
  puts (" -> ");
  puthex32 (end);
  puts (" (");
  put_kb (end > start ? end - start : 0);
  puts (")\r\n");
}

/*****************************************************************************/

static void
put_detect (ULONG flags)
{
  puts ("Detected via:          ");

  if (flags & MEMF_MBMMAP)
    {
      puts ("multiboot memory map");
    }
  else if (flags & MEMF_MBBASIC)
    {
      puts ("multiboot mem_upper");
    }
  else if (flags & MEMF_E820)
    {
      puts ("BIOS int 15h e820h");
    }
  else if (flags & MEMF_E801)
    {
      puts ("BIOS int 15h e801h");
    }
  else if (flags & MEMF_88)
    {
      puts ("BIOS int 15h ah=88h");
    }
  else
    {
      puts ("unknown");
    }

  puts (", A20 ");
  puts ((flags & MEMF_A20) ? "on" : "OFF");
  puts (".\r\n");
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct tpa_req t;
  struct memlayout m;
  ULONG base, top, tpa_len, free_prog;
  int i;

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

  for (i = 0; i < (int)sizeof (m); i++)
    {
      ((UBYTE *)&m) [i] = 0;
    }

  (void)bdos (BDOS_MEMLAYOUT, (LONG)(ULONG)&m);

  /*
   * Program load at TPA+0x100.  The ring-3 stack is reserved above the
   * region reported here (see pgm_enter), so the whole of it less the base
   * page is available to the program.
   */

  free_prog = (tpa_len > 0x100UL) ? (tpa_len - 0x100UL) : 0;

  puts ("\r\nCP/M-386 Memory Map\r\n");
  puts (    "-------------------\r\n");

  /*
   * The kernel runs in conventional memory and the TPA is above 1MB; the
   * two are separated by the video/ROM hole, which is not RAM and is not
   * part of any region below.
   */

  put_region ("System CBIOS/BDOS/CCP: ", m.kernel_base, m.ramdisk_base);

  if (m.ramdisk_size > 0)
    {
      put_region ("Initial RAM Disk:      ", m.ramdisk_base,
                  m.ramdisk_base + m.ramdisk_size);
    }

  put_region ("Ring-0 stack:          ", m.kernel_end, m.lowmem_top);
  put_region ("Video / ROM (no RAM):  ", 0xA0000UL, 0x100000UL);

  puts ("TPA base:              ");
  puthex32 (base);
  puts ("\r\n");
  puts ("TPA top:               ");
  puthex32 (top);
  puts ("\r\n");

  puts ("TPA size:              ");
  put_kb (tpa_len);
  puts (" (");
  putu (tpa_len);
  puts (" bytes)\r\n");

  if (m.stack_top > m.tpa_top)
    {
      put_region ("Ring-3 stack reserve:  ", m.tpa_top, m.stack_top);
    }

  put_detect (m.mem_flags);

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
