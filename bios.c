/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 4f421a0c-82b4-11f1-8e42-80ee73e9b8e7
 */

/*****************************************************************************/

/* bios.c: 386-specific BIOS layer and glue for CP/M-386 */

/*
 * Implements the bios_* interface expected by BDOS/CCP.
 * Console: PC-VGA+AT-kbd plus serial 1.
 * Using in-memory RAM disk for initial FS.
 */

#include "bdosinc.h" /* UBYTE/WORD etc                                    */
#include "bdosdef.h" /* for CPM386_HDR / loader core                      */
#include "biosdef.h" /* for bios_* decls/macros                           */
#include "bringup.h" /* ramdisk, dph0, cpm_bringup(), struct dpb/dph defs */
#include "pmode.h"   /* ring-3 GDT/IDT/TSS + enter_ring3                  */
#include "absaddr.h" /* abs. addr                                         */
#include "disk.h"    /* V86 int 13h disk server                           */
#include "memmap.h"  /* loader memory descriptor + A20 / RAM verification */
#include "io.h"      /* shared port I/O primitives                        */
#include "vgacon.h"  /* VGA text console primitives                       */
#include "vidbios.h" /* real mode int 10h thunk                           */
#include "vidmode.h" /* video mode table + console restore                */

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t; /* rough */

/* --- console: 8250/16550 (COM1) 0x3f8 --- */

#define COM1_PORT 0x3f8

static int com_present;
static int vga_present;

static int com_probe(void)
{
  uint8_t a, b, lcr, ier;

  /* 16450/16550 scratch register test */
  outb(COM1_PORT + 7, 0x55);
  a = inb(COM1_PORT + 7);

  outb(COM1_PORT + 7, 0xAA);
  b = inb(COM1_PORT + 7);

  if (a == 0x55 && b == 0xAA)
    return 1;

  /* 8250 IER test */
  lcr = inb(COM1_PORT + 3);
  outb(COM1_PORT + 3, (uint8_t)(lcr & ~0x80)); /* DLAB = 0 -> +1 is IER */

  outb(COM1_PORT + 1, 0x00);
  ier = inb(COM1_PORT + 1);

  if ((ier & 0xF0) != 0) {
    outb(COM1_PORT + 3, lcr); /* restore LCR */

    return 0;
  }

  outb(COM1_PORT + 1, 0x0F);
  ier = inb(COM1_PORT + 1);

  outb(COM1_PORT + 1, 0x00); /* leave interrupts off if we init later */
  outb(COM1_PORT + 3, lcr);

  /* Low nibble should stick; high nibble must remain clear. */
  return (ier & 0x0F) == 0x0F && (ier & 0xF0) == 0;
}


static void com_init(void) {
  com_present = com_probe();

  if (com_present) {
    /* init for 9600 8N1 */
    outb(COM1_PORT + 1, 0x00); /* disable ints */
    outb(COM1_PORT + 3, 0x80); /* enable DLAB */
    outb(COM1_PORT + 0, 0x0C); /* 9600 divisor low (115200/9600=12) */
    outb(COM1_PORT + 1, 0x00); /* high */
    outb(COM1_PORT + 3, 0x03); /* 8N1 */
    outb(COM1_PORT + 2, 0xC7); /* fifo */
    outb(COM1_PORT + 4, 0x0B); /* DTR/RTS/OUT2 */
  }

  vga_present = vgacon_probe();

  /*
   * A machine with neither adapter has no console at all!
   * Assume the VGA probe was wrong.
   */

  if (!vga_present && !com_present)
    vga_present = 1;

  if (vga_present)
    vgacon_init();
}

static int com_stat(void) {
  if (!com_present)
    return 0;

  return (inb(COM1_PORT + 5) & 0x01) ? 0x00FF : 0x0000;
}

static void com_out(unsigned char c) {
  int spins = 0;

  if (!com_present)
    return;

  /* Bounded poll if UART is stuck */
  while ((inb(COM1_PORT + 5) & 0x20) == 0 && ++spins < 1000000)
    ;

  outb(COM1_PORT, c);

  if (c == '\n') { /* auto CR for terminals */
    spins = 0;
    while ((inb(COM1_PORT + 5) & 0x20) == 0 && ++spins < 1000000)
      ;
    outb(COM1_PORT, '\r');
  }
}

static unsigned char com_in(void) {
  int spins = 0;

  if (!com_present)
    return 0;

  while ((inb(COM1_PORT + 5) & 0x01) == 0 && ++spins < 1000000)
    ;

  return inb(COM1_PORT);
}

/*
 * --- Video (VGA text console, see vgacon.c) + AT keyboard ---
 * Input: prefer keyboard if ready else serial
 */

static int have_kbd_input = 0; /* once we see PS/2 input, ignore serial input */

int bios_vga_present(void)
{
  return vga_present;
}

int bios_com_present(void)
{
  return com_present;
}

/* Basic PS/2 scancode set1 to ASCII (no numpad yet, basic shifts) */

static int kbd_shift = 0;
static int kbd_ctrl = 0;

static unsigned char kbd_map [128] = {
  0, 0x1b, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
  'z','x','c','v','b','n','m',',','.','/', 0, 0, 0, ' ', 0,
  /* rest 0 for F1 etc */
};

static unsigned char kbd_map_shift [128] = {
  0, 0x1b, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
  '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
  'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|',
  'Z','X','C','V','B','N','M','<','>','?', 0, 0, 0, ' ', 0,
};

/*
 * -1 = no ASCII pending.  Scancodes that are not mapped (breaks, shift, ...)
 * must not make bios_const() true - otherwise conbrk() calls bios_conin()
 * which used to spin until a real key arrived (looked like "press Enter
 * to continue" during HD/OD dumps)
 */

static int kbd_peek = -1;

/* Consume one scancode from the controller; return ASCII or 0. */

static unsigned char kbd_scancode_to_ascii(void)
{
  uint8_t sc;
  unsigned char ch;

  sc = inb(0x60);

  if (sc & 0x80) { /* break */
    if (sc == 0xaa || sc == 0xb6)
      kbd_shift = 0; /* left/right shift release */
    if (sc == 0x9d)
      kbd_ctrl = 0; /* Ctrl release */

    return 0;
  }

  if (sc == 0x2a || sc == 0x36) {
    kbd_shift = 1;

    return 0;
  }

  if (sc == 0x1d) { /* left Ctrl (right is E0 1D; treat 1D as Ctrl) */
    kbd_ctrl = 1;

    return 0;
  }

  if (sc >= 128)
    return 0;

  ch = kbd_shift ? kbd_map_shift [sc] : kbd_map [sc];

  if (!ch)
    return 0;

  /* Ctrl+letter -> 0x01..0x1A (so Ctrl-Z is ENDFILE for ED, etc.) */
  if (kbd_ctrl) {
    unsigned char u = ch;

    if (u >= 'a' && u <= 'z')
      u = (unsigned char)(u - 'a' + 'A');

    if (u >= 'A' && u <= 'Z')
      return (unsigned char)(u - '@'); /* A->1 ... Z->0x1A */

    if (u == ' ')
      return 0; /* Ctrl-Space: ignore */

    if (u == '[')
      return 0x1b; /* often ESC */

    return 0;
  }

  return ch;
}

/* Drain PS/2 output buffer into kbd_peek (first real ASCII char only). */
static void kbd_drain(void)
{
  int spins = 0;

  while (kbd_peek < 0 && (inb(0x64) & 0x01) && spins++ < 64) {
    unsigned char ch = kbd_scancode_to_ascii();

    if (ch)
      kbd_peek = (int)ch;
  }
}

static int kbd_stat(void)
{
  kbd_drain();

  return (kbd_peek >= 0) ? 0x00FF : 0;
}

static unsigned char kbd_in(void)
{
  unsigned char ch;

  for (;;) {
    kbd_drain();

    if (kbd_peek >= 0) {
      ch = (unsigned char)kbd_peek;
      kbd_peek = -1;

      return ch;
    }

    { /* Block for a scancode (bounded idle poll). */
      int spins = 0;

      while ((inb(0x64) & 0x01) == 0 && ++spins < 1000000);

      if (!(inb(0x64) & 0x01))
        return 0; /* no controller / timeout */
    }
  }
}

/* Memory region table for TPA dynamic sized to end of memory after kernel */

typedef struct {
  uint16_t count;
  void *base;
  unsigned long length;
} mrt_t;

static mrt_t mrt;

/* --- BIOS entry points, some to be completed) --- */

/*
 * Console output enables (BDOS 222/223).
 * Both start disabled and are set from the probes in cpm386_init.
 */

static int con_vga_en = 0;
static int con_ser_en = 0;

void bios_wboot(void) {
  extern void ccp(void);

  /*
   * The other end of the video restore.  A BDOS 47 chain, or a disk error
   * the user answers with abort, calls warmboot() and re-enters the CCP
   * here without ever unwinding through pgm_enter(), so pgm_after_exit()
   * does not run on those paths.
   */

  if (vga_present)
    vidmode_restore_console();

  ccp();
}

unsigned short int bios_const(void) {

  /*
   * Waiting on the console is the systems idle state;
   * give the floppy motor gets a chance to spin down.
   * No-op unless actually turning.
   */

  disk_poll();

  /*
   * Only report ready when a real character is available.
   * Input stays available even if that devices *output* is disabled
   * (SERON can still be typed after SEROFF on a serial-only box!).
   */

  if (kbd_stat())
    return 0x00FF;

  if (com_stat())
    return 0x00FF;

  return 0;
}

unsigned char bios_conin(void) {
  for (;;) {
    /*
     * The system spends its idle life in this loop, so it is where
     * the floppy motor gets timed out; no-op unless one is turning.
     */

    disk_poll();

    if (kbd_stat()) {
      unsigned char ch = kbd_in();

      if (ch) {
        have_kbd_input = 1;

        return ch;
      }

      continue;
    }

    if (com_stat())
        return com_in();
  }
}

void bios_conout(unsigned char c)
{
  if (con_ser_en)
    com_out(c);

  if (con_vga_en)
    vgacon_putc(c);
}

/*
 * Write raw bytes to COM1 only (no VGA, no LF->CR rewrite)
 * Bounded so a missing UART does not hang!
 */

static void com_out_raw(unsigned char c)
{
  int spins = 0;

  if (!com_present)
    return;

  while ((inb(COM1_PORT + 5) & 0x20) == 0 && ++spins < 1000000)
    ;

  outb(COM1_PORT, c);
}

/*
 * Clear enabled console paths independently:
 *   serial - ANSI clear + home (clsansi.a86: ESC[2J ESC[H])
 *   VGA    - wipe text plane + home software/hardware cursor
 *
 * Must NOT go through bios_conout(): dual-write would paint the ESC
 * sequence as glyphs on the VGA while still needing a real clear there.
 */

void bios_con_clear(void)
{
  static const char ansi [] = "\033[2J\033[H";
  const char *p;

  if (con_ser_en) {
    for (p = ansi; *p; p++)
      com_out_raw((unsigned char)*p);
  }

  if (con_vga_en)
      vgacon_clear();
}

/*
 * BDOS 222 / 223: console path enables.
 * info 0 = off, 1 = on, 0xFFFF = query.
 *
 * Returns:
 *   0            path is disabled
 *   1            path is enabled
 *   CON_ABSENT   no such adapter fitted; the request was ignored
 *   CON_LAST     refused, would leave no console output at all
 */

#define CON_ABSENT 0xFE
#define CON_LAST 0xFF

static unsigned short con_ctl(int *en, int present, int other_en,
                              unsigned short info)
{
  if (!present)
    return CON_ABSENT;

  if (info == 0xFFFF)
    return (unsigned short)(*en ? 1 : 0);

  if (info == 0) {
    if (!other_en)
      return CON_LAST;

    *en = 0;
  } else {
    *en = 1;
  }

  return (unsigned short)(*en ? 1 : 0);
}

unsigned short bios_con_vga_ctl(unsigned short info)
{
  return con_ctl(&con_vga_en, vga_present, con_ser_en && com_present, info);
}

unsigned short bios_con_ser_ctl(unsigned short info)
{
  return con_ctl(&con_ser_en, com_present, con_vga_en && vga_present, info);
}

void bios_list(unsigned char c)
{
  (void)c;
}

void bios_punch(unsigned char c)
{
  (void)c;
}

unsigned char bios_reader(void)
{
  return 0x1A;
} /* ^Z eof */

#if 0
void bios_home(void) { /* no-op for ram */ }
#endif

/*
 * Classic CP/M base page in the TPA (offsets relative to TPA base / user DS).
 * Programs load at TPA+0x100 so 0x00..0xFF remain available for this layout.
 */

void bios_setup_basepage(const void *fcb1, const void *fcb2, const char *tail)
{
  mrt_t *lmrt = bios_getmrt();
  UBYTE *tpa = (UBYTE *)lmrt->base;
  unsigned long tpa_len = lmrt->length;
  int i;
  unsigned tlen = 0;

  if (!tpa || tpa_len < 0x100)
    return;

  /* Clear base page */
  for (i = 0; i < 0x100; i++)
    tpa [i] = 0;

  /* Default FCB at 0x5C (first arg), second at 0x6C */
  if (fcb1) {
    for (i = 0; i < 36; i++)
      tpa [0x5C + i] = ((const UBYTE *)fcb1) [i];
  }

  if (fcb2) {
    for (i = 0; i < 16; i++) /* only 16 bytes fit before 0x7C..0x7F */
      tpa [0x6C + i] = ((const UBYTE *)fcb2) [i];
  }

  /*
   * Command tail at 0x80: length byte + up to 126 chars + trailing NUL.
   * CCP's tail pointer usually starts at the blank after the command.
   */

  if (tail) {
    while (*tail == ' ' || *tail == '\t')
      tail++;

    tlen = 0;

    while (tail [tlen] && tlen < 126)
      tlen++;

    tpa [0x80] = (UBYTE)tlen;

    for (i = 0; i < (int)tlen; i++)
      tpa [0x81 + i] = (UBYTE)tail [i];

    tpa [0x81 + tlen] = 0;
  }
}

static unsigned long mem_flags;
static int mem_a20_ok;

unsigned long bios_mem_flags(void) { return mem_flags; }

int bios_mem_a20_ok(void) { return mem_a20_ok; }

#define TPA_STACK_MIN 0x10000UL  /* 64K  */
#define TPA_STACK_MAX 0x100000UL /* 1M   */

static unsigned long tpa_stack_reserve;

unsigned long bios_tpa_stack_reserve(void) { return tpa_stack_reserve; }

void *bios_getmrt(void) {
  if (mrt.count == 0) {
    unsigned long base = 0;
    unsigned long top = 0;

    mem_flags = mem_get_boot_region(&base, &top);
    mem_a20_ok = mem_a20_enabled();

    if (!mem_a20_ok) {
      base = top = 0;
    }

    if (top > base) {
      top = mem_verify_region(base, top);
    }

    mrt.count = 1;

    if (top > base && top - base >= TPA_MIN_BYTES) {
      unsigned long len = top - base;
      unsigned long res = len / 8;

      if (res > TPA_STACK_MAX)
        res = TPA_STACK_MAX;

      if (res < TPA_STACK_MIN)
        res = TPA_STACK_MIN;

      res = (res + 0xFFFUL) & ~0xFFFUL;

      /* TPA_MIN_BYTES > TPA_STACK_MIN keeps this from underflowing. */
      tpa_stack_reserve = res;
      mrt.base = (void *)base;
      mrt.length = len - res;
    } else {
      /* Nothing usable; pmode_active() stays false and the CCP says so. */
      tpa_stack_reserve = 0;
      mrt.base = (void *)0;
      mrt.length = 0;
    }
  }

  return &mrt;
}

/* BDOS 228 stuff */

unsigned long bios_mem_layout(struct cpm_memlayout *mp) {
  mrt_t *lmrt = bios_getmrt();
  extern char __kernel_end [];

  mp->kernel_base = 0x10000UL;
  mp->kernel_end = (unsigned long)__kernel_end;
  mp->ramdisk_base = (unsigned long)&ramdisk [0];
  mp->ramdisk_size = (unsigned long)RAMDISK_SIZE;
  mp->lowmem_top = (unsigned long)__kernel_end + 0x4000UL;
  mp->tpa_base = (unsigned long)lmrt->base;
  mp->tpa_top = (unsigned long)lmrt->base + lmrt->length;
  mp->stack_top = mp->tpa_top + bios_tpa_stack_reserve();
  mp->mem_flags = mem_flags;

  return 0;
}

unsigned short int bios_getiobyte(void)
{
  return 0;
}

void bios_setiobyte(unsigned short int v)
{
  (void)v;
}

unsigned short int bios_flush(void)
{
  return 0;
}

void *bios_setexc(unsigned short int vec, void *h)
{
  (void)vec;
  (void)h;

  return 0;
}

/* --- glue / init / bdos wrapper for CCP --- */

/* Provide bdos() for ccp.c (calls _bdos) */

#ifndef HOST_TEST
UWORD bdos(WORD func, LONG info) {
  /*
   * 32-bit safe: full ptr value (from LONG parm) goes to infop slot (32bit);
   * low word to the UWORD info slot (as original 16-bit design)
   */

  extern UWORD _bdos(WORD func, UWORD info, UBYTE *infop);
  unsigned long pval = (unsigned long) info;

  return _bdos(func, (UWORD)pval, (UBYTE *)pval);
}
#endif

/* The CCP entry renamed? In no-RLI build, ccp() */

extern void ccp(void);

/* init_ramdisk moved to cpm_bringup.c */

/*
 * Machine reboot (for BDOS 220 / REBOOT.386).
 * warm!=0: set classic BIOS warm-boot flag at 0040:0072 = 0x1234
 *          (original reboot.a86); else clear it for cold POST.
 * Then pulse the keyboard controller reset line (port 0x64 / cmd 0xFE).
 * Does not return.
 */

void bios_system_reboot(int warm)
{
  /* Real-mode BDA word at physical 0x472 - flat identity map in pmode. */
  *ABS_U16(0x472) = warm ? (uint16_t)0x1234 : (uint16_t)0;

  /* Wait for keyboard controller input buffer empty, then pulse reset. */
  {
    int spins = 0;
    while ((inb(0x64) & 0x02) != 0 && ++spins < 100000);
  }

  outb(0x64, 0xFE);

  /* If KBC reset is ignored (rare), hang - triple-fault via bad LIDT. */
  for (;;) {
    struct { uint16_t lim; uint32_t base; } __attribute__((packed)) idtr =
      { 0, 0 };
    __asm__ volatile ("lidt %0; int $3" : : "m"(idtr) : "memory");
    __asm__ volatile ("cli; hlt");
  }
}

/* Unsigned decimal to the console, for boot diagnostics only */
static void bios_num_out(unsigned long n)
{
  char b [12];
  int i = 0;

  if (!n) {
    bios_conout('0');

    return;
  }

  while (n && i < 12) {
    b [i++] = (char)('0' + (n % 10));
    n /= 10;
  }

  while (i)
    bios_conout((unsigned char)b [--i]);
}

/* cold boot entry from asm */
void cpm386_init(void) {
  com_init();

  /* Enable devices the probes found; either alone is OK */
  con_vga_en = vga_present ? 1 : 0;
  con_ser_en = com_present ? 1 : 0;

  /* Install user TPA segments, TSS, and int 0x30 BDOS gate before any load. */
  {
    mrt_t *lmrt = bios_getmrt();

    /*
     * Without a verified TPA there is nowhere to run ring-3 code, and
     * carrying on would corrupt the kernel rather than fail cleanly.
     * Say which of the two things went wrong and stop.
     */

    if (lmrt->length == 0) {
      const char *why =
        !bios_mem_a20_ok()
          ? "\r\nA20 gate is NOT enabled!  Cannot address memory above 1MB."
          : "\r\nNo usable memory above 1MB!  2MB or more RAM is required.";

      bios_conout('\r');
      bios_conout('\n');

      while (*why)
        bios_conout((unsigned char)*why++);

      why = "\r\n\r\nCP/M-386 fatal error; system halted.\r\n\r\n";

      while (*why)
        bios_conout((unsigned char)*why++);

      for (;;)
        __asm__ volatile("cli; hlt");
    }

    /*
     * The ring-3 segment spans the loadable region plus the stack reserve
     * above it; ring 3 must be able to address its own stack, and BDOS
     * pointer arguments are routinely stack locals.
     */

    pmode_init((unsigned long)lmrt->base,
               (unsigned long)lmrt->length + bios_tpa_stack_reserve());
  }

  /*
   * Real mode int 10h thunk.  This has to come after pmode_init(), which
   * is what installs the GDT holding the 16-bit descriptors the transition
   * step down.  Only useful with an VGA video adapter fitted, and so the
   * read-only AH=0Fh call below is the proof that the protected/real mode
   * transition works on this machine before anything depends on it.
   */

  /* Clear 40 characters */
  {
    unsigned short x;

    bios_conout('\r');

    for (x = 0; x < 40; x++)
      bios_conout(' ');

    bios_conout('\r');
  }

  if (vga_present) {
    static const char vmsg [] = "VGA console enabled (mode ";
    static const char vbad [] = "VGA BIOS not callable!";
    const char *p;
    unsigned m;

    vidbios_init();
    m = vid_bios_mode();

    /* Debugging output, most users won't see it! */
    if (m == 0xFFFF) {
      for (p = vbad; *p; p++)
        bios_conout((unsigned char)*p);
    } else {
      static const char hex [] = "0123456789ABCDEF";

      for (p = vmsg; *p; p++)
        bios_conout((unsigned char)*p);

      bios_conout((unsigned char)hex [(m >> 4) & 0xF]);
      bios_conout((unsigned char)hex [m & 0xF]);
      bios_conout('h');
      bios_conout(',');
      bios_conout(' ');
      bios_num_out(vgacon_cols());
      bios_conout('x');
      bios_num_out(vgacon_rows());
      bios_conout(')');
    }
  }

  /* Clear 40 characters again */
  {
    unsigned short x;

    bios_conout('\r');

    for (x = 0; x < 40; x++)
      bios_conout(' ');

    bios_conout('\r');
  }

  /* Learn the mode the BIOS left us in, without changing it. */
  if (vga_present)
    vidmode_init();


  cpm_bringup(); /* shared bring-up (ramdisk/dph + bdosinit + select A) */
  extern void bdosinit(void);
  bdosinit(); /* reinit to ensure GBL.delim and state for clean bdos(9) banner */

  /* ensure CP/M-68K banner (portable string); $ terminator */
  extern char *copyrt;
  extern char *copyr1;
  extern char *copyr2;

  bdos(9, (LONG)"\r                                        \r$");
  bdos(9, (LONG)copyrt);
  bdos(9, (LONG)"\r\n$");
  bdos(9, (LONG)copyr1);
  bdos(9, (LONG)"\r\n$");
  bdos(9, (LONG)copyr2);
  bdos(9, (LONG)"\r\n\r\n$");

  /* report available TPA size in KB */
  {
    extern BYTE *tpa_lp;
    extern BYTE *tpa_hp;
    unsigned long tpa_bytes = (unsigned long)tpa_hp - (unsigned long)tpa_lp;
    unsigned long kb = tpa_bytes / 1024;
    char tmp [16];
    int i = 0;

    if (kb == 0) {
      tmp [i++] = '0';
    } else {
      while (kb > 0 && i < 15) {
        tmp [i++] = '0' + (kb % 10);
        kb /= 10;
      }
    }

    char buf [20];
    char *p = buf;

    while (i > 0)
      *p++ = tmp [--i];

    *p++ = 'K'; *p++ = ' '; *p++ = 'T'; *p++ = 'P'; *p++ = 'A';
    *p++ = '\r'; *p++ = '\n'; *p++ = '$'; *p = 0;
    bdos(9, (LONG)buf);
  }

  /* Bring up the V86 int 13h server and say what BIOS drives it found. */
  disk_report();

  ccp();
}

/*
 * Provide other externs expected by BDOS
 * sources that are BIOS mapped or internal
 */

#ifndef HOST_TEST
/*
 * _start is now provided by mbentry.S (Multiboot header + dual-path entry).
 * It lives in .text.start and is placed immediately after the multiboot header.
 */

# if 0
int main(void)
{
  return 0; /* not used */
}
# endif
#endif

/* --- stubs and helpers for link (swap/udiv from prior art, load stubs) --- */

WORD swap(WORD victim) {
  static int swaptype = -1;
  static unsigned short testpattern = 0x0102;
  unsigned short temp;

  if (swaptype < 0)
    {
     if (*(char *)&testpattern == 0x01) swaptype = 1; else swaptype = 0;
    }

  if (swaptype == 0)
    return victim;

  temp = ((victim & 0xff) << 8) | ((victim & 0xff00) >> 8);

  return (WORD)temp;
}

UWORD udiv(LONG dividend, UWORD divisor, UWORD *remainder) {
  if (divisor == 0)
    {
      *remainder = 0;
      return 0;
    }

  *remainder = (UWORD)(dividend % (LONG)divisor);

  return (UWORD)(dividend / (LONG)divisor);
}

#if 0
UWORD load68k(BYTE *info)
{
  (void)info;

  return 3; /* load error */
}
#endif

/*
 * Minimal load_tbl stub so ccp.o (cmd_file) links for
 * SUB paths (builtins DIR/TYPE do not use it)
 */

struct _filetyps {
  BYTE *typ;
  UWORD (*loader) ();
  BYTE user_c;
  BYTE user_0;
};

struct _filetyps load_tbl [] = {
  { (BYTE *)"386", 0, 0, 0 },
  { (BYTE *)"SUB", 0, 0, 0 }, /* SUBMIT scripts (CCP cmd_file search) */
  { (BYTE *)"", 0, 0, 0 },
  { (BYTE *)0, 0, 0, 0 }
};

/*
 * Sequential BDOS reader context for streaming
 * .386 load (multi-extent, no size cap).
 */

struct pgm_read_ctx {
  BYTE *fcb;
  UBYTE recbuf [128];
};

/*
 * One sequential record via BDOS 20. Status 1 (EOF/unwritten/hole) is returned
 * to the streaming core, which zero-fills mid-image holes. When status is 1 and
 * cur_rec did not advance, bump it so the next call progresses (holes / sparse).
 */

static UWORD pgm_read_rec(UBYTE rec [128], void *vctx)
{
  struct pgm_read_ctx *c = (struct pgm_read_ctx *)vctx;
  UWORD r;
  UBYTE prev_cr = c->fcb [32];
  int i;

  bdos(26, (LONG)(unsigned long)c->recbuf);
  r = bdos(20, (LONG)(unsigned long)c->fcb);

  for (i = 0; i < 128; i++)
    rec [i] = c->recbuf [i];

  if (r == 1 && c->fcb [32] == prev_cr) {
    /*
     * Unwritten/hole: BDOS left cur_rec unchanged - advance like a skip.
     * cur_rec is UBYTE 0..128; 128 triggers new_ext on the next read.
     */

    if ((unsigned)c->fcb [32] < 128u)
      c->fcb [32] = (UBYTE)(c->fcb [32] + 1);
  }

  return r;
}

/*
 * ZCPR-style GO: last successful load leaves the image in the TPA.
 * GO re-enters at the saved entry without reading the disk again.
 */

static unsigned long g_go_entry_off;
static int g_go_valid;

/* Common post-exit / pre-return cleanup (disk session + TPA limits). */

static void pgm_after_exit(void)
{
  extern BYTE *tpa_lt, *tpa_lp, *tpa_ht, *tpa_hp;
  static UBYTE kdma [128];

  /*
   * undo any video mode the program left behind, including a console mode
   * it set but never committed to.  Covers both a clean exit and a fault
   * abort, since both return through pgm_enter().  A noop when the mode
   * already matches so ordinary programs unaffected.
   */

  if (vga_present)
    vidmode_restore_console();

  tpa_lt = tpa_lp;
  tpa_ht = tpa_hp;

  /* XXX hax */
  bdos(13, 0); /* reset disk system */
  bdos(32, 0); /* user 0 */
  bdos(26, (LONG)(unsigned long)kdma);
  bdos(14, 0); /* select A: */
}

/* Enter ring 3 at TPA-relative entry_off; returns after BDOS(0). */
static UWORD pgm_enter(unsigned long entry_off)
{
  /*
   * Full segment length, including the stack reserve.  ESP starts at the
   * very top of the segment so the program (growing up from TPA+0x100) and
   * its stack (growing down) are as far apart as the TPA allows.  Pinning
   * ESP to a fixed 1M offset used to both waste everything above it and
   * put the stack inside the image of any program larger than 1M.
   */

  unsigned long ulen = pmode_tpa_len();
  unsigned long user_esp;

  if (ulen < 0x2000)
    return CPMLD_NOMEM; /* nowhere to run at all */

  /*
   * Round down to the page granularity pmode_init used for the segment
   * limit, then leave a little slack below the very last valid byte.
   */

  user_esp = (ulen & ~0xFFFUL) - 16;
  enter_ring3(entry_off, user_esp);
  pgm_after_exit();

  return 0;
}

UWORD pgmld(UBYTE *infop, UBYTE *dmaadr) {
  (void)dmaadr;
  mrt_t *lmrt;
  UBYTE *tpa_base;
  unsigned long tpa_len;
  UBYTE *entry = 0;
  struct pgm_read_ctx ctx;
  BYTE loadfcb [36];
  int j;

  /* get TPA from BIOS mrt (same as bgetseg) */
  lmrt = bios_getmrt();
  tpa_base = (UBYTE *)lmrt->base;
  tpa_len = lmrt->length;

  /* Clean FCB for sequential load (fresh cur_rec / dskmap; multi-extent
   * crossing is handled inside BDOS read + new_ext). */
  for (j = 0; j < 36; j++) loadfcb [j] = 0;

  loadfcb [0] = ((struct fcb *)infop)->drvcode;

  for (j = 0; j < 11; j++) loadfcb [1 + j] = ((BYTE *)infop) [1 + j];

  if (bdos(15, (LONG)loadfcb) > 3) {
    return 0xFFFE; /* cannot re-open for load read */
  }

  loadfcb [32] = 0; /* ensure sequential from record 0 */

  ctx.fcb = loadfcb;
  {
    UWORD rc = cpm386_load_from_reader(pgm_read_rec, &ctx,
                                       tpa_base, tpa_len, &entry);
    if (rc != 0) {
      /* Partial image may have overwritten the prior GO target */
      g_go_valid = 0;

      return rc;
    }
  }

  /* Ring-3 launch: user CS/DS base = TPA, EIP/ESP TPA-relative. */
  {
    unsigned long entry_off = (unsigned long)(entry - tpa_base);
    g_go_entry_off = entry_off;
    g_go_valid = 1;

    return pgm_enter(entry_off);
  }
}

/*
 * Re-run the last TPA image (CCP GO builtin).  Base page / FCBs / tail must
 * already be set by the CCP.  Returns 0xFFFF if nothing is loaded.
 */

UWORD pgm_go(void)
{
  if (!g_go_valid)
    return 0xFFFF;

  return pgm_enter(g_go_entry_off);
}

UBYTE *traphndl(void)
{
  return (UBYTE *)0;
}

void initexc(UBYTE **vecs)
{
  (void)vecs;
}  /* no 68k exceptions here! */

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
