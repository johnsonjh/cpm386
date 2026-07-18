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

#include "bdosinc.h"     /* UBYTE/WORD etc */
#include "bdosdef.h"     /* for CPM386_HDR / loader core */
#include "biosdef.h"     /* for bios_* decls/macros */
#include "cpm_bringup.h" /* shared: ramdisk, dph0, cpm_bringup(), struct dpb/dph defs */
#include "pmode.h"       /* ring-3 GDT/IDT/TSS + enter_ring3 */
#include "absaddr.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t; /* rough */

/* --- console: 8250/16550 (COM1) 0x3f8 --- */

#define COM1_PORT 0x3f8

static int com_present;

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static void vga_init(void); /* forward */
static void vga_update_cursor(void); /* forward for calls in clear/scroll */

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

  vga_init(); /* VGA text always; serial only if present */
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
 * --- Video (VGA 80x25 0xb8000) + AT keyboard ---
 * Input: prefer keyboard if ready else serial
 */

#define VGA_BASE 0xb8000UL
#define VGA_W 80
#define VGA_H 25

static volatile uint16_t *vga_mem = (volatile uint16_t *)VGA_BASE;
static int vrow = 0, vcol = 0;
static int have_kbd_input = 0; /* once we see PS/2 input, ignore serial input */

static void vga_clear(void) {
  int i;

  for (i = 0; i < VGA_W * VGA_H; i++)
    vga_mem[i] = (uint16_t)(' ' | (0x07 << 8));

  vrow = vcol = 0;
  vga_update_cursor();
}

static void vga_scroll(void) {
  int y, x, pos;

  for (y = 0; y < VGA_H-1; y++) {
    for (x = 0; x < VGA_W; x++) {
      pos = y * VGA_W + x;
      vga_mem[pos] = vga_mem[pos + VGA_W];
    }
  }

  for (x = 0; x < VGA_W; x++) {
      vga_mem[(VGA_H-1)*VGA_W + x] = (uint16_t)(' ' | (0x07 << 8));
  }

  vga_update_cursor();
}

/* hardware cursor via CRTC (0x3D4/0x3D5). */
static void vga_update_cursor(void) {
  uint16_t pos = (uint16_t)(vrow * VGA_W + vcol);
  outb(0x3D4, 0x0E); /* cursor location high */
  outb(0x3D5, (pos >> 8) & 0xFF);
  outb(0x3D4, 0x0F); /* cursor location low */
  outb(0x3D5, pos & 0xFF);
}

static void vga_putc(unsigned char c) {
  int pos;

  if (c == '\r') {
    vcol = 0;
    vga_update_cursor();

    return;
  }

  if (c == '\n') {
    vcol = 0;

    if (++vrow >= VGA_H) {
      vrow = VGA_H-1;
      vga_scroll();
    }

    vga_update_cursor();

    return;
  }

  if (c == '\b') {
    if (vcol > 0) vcol--;
    vga_update_cursor();

    return;
  }

  if (c == '\t') {
    vcol = (vcol + 8) & ~7;

    if (vcol >= VGA_W) {
      vcol = 0;

      if (++vrow >= VGA_H) {
        vrow = VGA_H-1;
        vga_scroll();
      }
    }

    vga_update_cursor();

    return;
  }

  /* printable or control we treat as-is */
  if (vcol >= VGA_W) {
    vcol = 0;

    if (++vrow >= VGA_H) {
      vrow = VGA_H-1;
      vga_scroll();
    }
  }

  pos = vrow * VGA_W + vcol;
  vga_mem[pos] = (uint16_t)(c | (0x07 << 8));
  vcol++;
  vga_update_cursor();
}

static void vga_init(void) {
  vga_clear();
  /* reset CRTC start address (regs 0x0C/0x0D) to 0 */
  outb(0x3D4, 0x0C);
  outb(0x3D5, 0);
  outb(0x3D4, 0x0D);
  outb(0x3D5, 0);
  vga_update_cursor();
}

/* Basic PS/2 scancode set1 to ASCII (no numpad yet, basic shifts) */

static int kbd_shift = 0;
static int kbd_ctrl = 0;

static unsigned char kbd_map[128] = {
  0, 0x1b, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
  'z','x','c','v','b','n','m',',','.','/', 0, 0, 0, ' ', 0,
  /* rest 0 for F1 etc */
};

static unsigned char kbd_map_shift[128] = {
  0, 0x1b, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
  '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
  'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|',
  'Z','X','C','V','B','N','M','<','>','?', 0, 0, 0, ' ', 0,
};

/*
 * -1 = no ASCII pending.  Scancodes that are not mapped (breaks, shift, …)
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

  ch = kbd_shift ? kbd_map_shift[sc] : kbd_map[sc];
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
 * Console output enables (BDOS 222/223). VGA on by default.
 * Serial is enabled in cpm386_init only if COM1 probe succeeds.
 */

static int con_vga_en = 1;
static int con_ser_en = 0;

void bios_wboot(void) {
  /* for now, just loop or jump to ccp restart */
  extern void ccp(void);
  ccp();
}

unsigned short int bios_const(void) {
  /*
   * Only report ready when a real character is available.
   * Input stays available even if that device's *output* is disabled
   * (so SERON can still be typed after SEROFF on a serial-only box).
   */

  if (kbd_stat())
    return 0x00FF;

  if (com_stat())
    return 0x00FF;

  return 0;
}

unsigned char bios_conin(void) {
  for (;;) {
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
    vga_putc(c);
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
  static const char ansi[] = "\033[2J\033[H";
  const char *p;

  if (con_ser_en) {
    for (p = ansi; *p; p++)
      com_out_raw((unsigned char)*p);
  }

  if (con_vga_en)
      vga_clear();
}

/*
 * BDOS 222: VGA console enable.
 * info 0 = off, 1 = on, 0xFFFF = query.
 * Returns 0/1 status, or 0xFF if refusing to
 * disable the last remaining output path.
 */

unsigned short bios_con_vga_ctl(unsigned short info)
{
  if (info == 0xFFFF)
      return (unsigned short)(con_vga_en ? 1 : 0);

  if (info == 0) {
    if (!con_ser_en)
      return 0xFF; /* would leave no console output */

    con_vga_en = 0;
  } else {
    con_vga_en = 1;
  }

  return (unsigned short)(con_vga_en ? 1 : 0);
}

/* BDOS 223: serial console enable */
unsigned short bios_con_ser_ctl(unsigned short info)
{
  if (info == 0xFFFF)
    return (unsigned short)(con_ser_en ? 1 : 0);

  if (info == 0) {
    if (!con_vga_en)
      return 0xFF;

    con_ser_en = 0;
  } else {
    con_ser_en = 1;
  }

  return (unsigned short)(con_ser_en ? 1 : 0);
}

void bios_list(unsigned char c) { (void)c; }
void bios_punch(unsigned char c) { (void)c; }
unsigned char bios_reader(void) { return 0x1A; } /* ^Z eof */

void bios_home(void) { /* no-op for ram */ }

static unsigned short cur_trk = 0, cur_sec = 0;
static void *cur_dma_ptr = 0;
static void *current_dph = 0;
/* track last selected for spt etc, avoid direct dpb0 use */

void *bios_seldsk(unsigned char drive, unsigned char logged) {
  (void)logged;
  cur_trk = cur_sec = 0;

  if (drive < 16) {
    /*
     * always return our ram dph for drives 0-15;
     * prevents fileio seldsk's while(!error(3)) path
     * on null dphp for unsupported drives
     * (single-drive ramdisk emulates A:)
     * CCP normal flow stays clean
     */

    current_dph = &dph0;

    return &dph0;
  }

  current_dph = 0;

  return 0;
}

void bios_settrk(unsigned short int track) { cur_trk = track; }
void bios_setsec(unsigned short int sector) { cur_sec = sector; }
void bios_setdma(void *dmaaddress) { cur_dma_ptr = dmaaddress; }

unsigned short int bios_read(void) {
  unsigned short spt = 32; /* fallback (match 4mb-hd) */

  if (current_dph) {
    struct dph *hdr = (struct dph *)current_dph;
    if (hdr->dpbp)
      spt = hdr->dpbp->spt;
  }

  unsigned long off = ((unsigned long)cur_trk * spt + cur_sec) * 128UL;

  if (off + 128 > RAMDISK_SIZE)
    return 1; /* error */

  if (cur_dma_ptr) {
    int i;

    unsigned char *src = &ramdisk[off];
    unsigned char *dst = (unsigned char *)cur_dma_ptr;

    for (i = 0; i < 128; i++)
      dst[i] = src[i];
  }

  return 0; /* ok */
}

unsigned short int bios_write(unsigned short int typecode) {
  (void)typecode;

  unsigned short spt = 32; /* fallback (match 4mb-hd) */

  if (current_dph) {
    struct dph *hdr = (struct dph *)current_dph;

    if (hdr->dpbp) spt = hdr->dpbp->spt;
  }

  unsigned long off = ((unsigned long)cur_trk * spt + cur_sec) * 128UL;

  if (off + 128 > RAMDISK_SIZE)
    return 1;

  if (cur_dma_ptr) {
    int i;
    unsigned char *src = (unsigned char *)cur_dma_ptr;
    unsigned char *dst = &ramdisk[off];

    for (i = 0; i < 128; i++)
      dst[i] = src[i];
  }

  return 0;
}

unsigned short int bios_listst(void)
{
  return 0;
}

unsigned short int bios_sectran(unsigned short int sec, void *table)
{
  (void)table; return sec; /* no xlt */
}

/*
 * Classic CP/M base page in the TPA (offsets relative to TPA base / user DS).
 * Programs load at TPA+0x100 so 0x00..0xFF remain available for this layout.
 */

void bios_setup_basepage(const void *fcb1, const void *fcb2, const char *tail)
{
  mrt_t *mrt = bios_getmrt();
  UBYTE *tpa = (UBYTE *)mrt->base;
  unsigned long tpa_len = mrt->length;
  int i;
  unsigned tlen = 0;

  if (!tpa || tpa_len < 0x100)
    return;

  /* Clear base page */
  for (i = 0; i < 0x100; i++)
    tpa[i] = 0;

  /* Default FCB at 0x5C (first arg), second at 0x6C */
  if (fcb1) {
    for (i = 0; i < 36; i++)
      tpa[0x5C + i] = ((const UBYTE *)fcb1)[i];
  }

  if (fcb2) {
    for (i = 0; i < 16; i++) /* only 16 bytes fit before 0x7C..0x7F */
      tpa[0x6C + i] = ((const UBYTE *)fcb2)[i];
  }

  /*
   * Command tail at 0x80: length byte + up to 126 chars + trailing NUL.
   * CCP's tail pointer usually starts at the blank after the command.
   */

  if (tail) {
    while (*tail == ' ' || *tail == '\t')
      tail++;

    tlen = 0;

    while (tail[tlen] && tlen < 126)
      tlen++;

    tpa[0x80] = (UBYTE)tlen;

    for (i = 0; i < (int)tlen; i++)
      tpa[0x81 + i] = (UBYTE)tail[i];

    tpa[0x81 + tlen] = 0;
  }
}

void *bios_getmrt(void) {
  if (mrt.count == 0) {
    uint32_t tpa_base = *ABS_U32(0x604);
    uint32_t top = *ABS_U32(0x600);
    extern char __kernel_end[];
    uint32_t k_end = (uint32_t)__kernel_end;

    /*
     * Keep TPA below typical PCI MMIO (~3.5G).
     * Offsets near 4G-1 with nonzero base land in non-RAM,
     * and ring-3 stack/data would silently fail
     */

    const uint32_t usable_top = 0xE0000000u;

    if (top == 0 || top > usable_top)
      top = usable_top;

    if (tpa_base == 0 || top <= tpa_base || tpa_base < k_end) {
      /* safe adaptive for low-mem (<1MB) or bad detect */
      uint32_t stack_res = 0x4000;
      tpa_base = k_end + stack_res + 0x1000;

      if (top <= tpa_base)
        top = k_end + 0x20000; /* fallback room */
    }

    uint32_t tpa_len = (top > tpa_base) ? (top - tpa_base) : 0x10000;

    /* Ensure base+len never wraps past 4G */
    if (tpa_base + tpa_len < tpa_base)
      tpa_len = (uint32_t)0u - tpa_base;

    mrt.count = 1;
    mrt.base = (void *)tpa_base;
    mrt.length = tpa_len;
  }

  return &mrt;
}

unsigned short int bios_getiobyte(void) { return 0; }
void bios_setiobyte(unsigned short int v) { (void)v; }

unsigned short int bios_flush(void) { return 0; }

void *bios_setexc(unsigned short int vec, void *h) { (void)vec; (void)h; return 0; }

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

/* cold boot entry from asm */
void cpm386_init(void) {
  com_init();
  /* Dual console only when hardware is there; VGA-only is fine. */
  con_ser_en = com_present ? 1 : 0;

  /* Install user TPA segments, TSS, and int 0x30 BDOS gate before any load. */
  {
    mrt_t *mrt = bios_getmrt();
    pmode_init((unsigned long)mrt->base, (unsigned long)mrt->length);
  }

  cpm_bringup(); /* shared bring-up (ramdisk/dph + bdosinit + select A) */
  extern void bdosinit(void);
  bdosinit(); /* reinit to ensure GBL.delim and state for clean bdos(9) banner */

  /* ensure CP/M-68K banner (portable string); $ terminator */
  extern char *copyrt;
  extern char *copyr1;
  extern char *copyr2;

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
    char tmp[16];
    int i = 0;

    if (kb == 0) {
      tmp[i++] = '0';
    } else {
      while (kb > 0 && i < 15) {
        tmp[i++] = '0' + (kb % 10);
        kb /= 10;
      }
    }

    char buf[20];
    char *p = buf;

    while (i > 0)
      *p++ = tmp[--i];

    *p++ = 'K'; *p++ = ' '; *p++ = 'T'; *p++ = 'P'; *p++ = 'A';
    *p++ = '\r'; *p++ = '\n'; *p++ = '$'; *p = 0;
    bdos(9, (LONG)buf);
  }

  ccp();
}

/* Provide other externs expected by bdos sources that are BIOS mapped or internal */
/* (removed warmboot/conin/conout to avoid multiple def with bdosmisc/conbdos) */

#ifndef HOST_TEST
/*
 * _start is now provided by mbentry.S (Multiboot header + dual-path entry).
 * It lives in .text.start and is placed immediately after the multiboot header.
 */

int main(void)
{
  return 0; /* not used */
}
#endif

/* --- stubs and helpers for link (swap/udiv from prior art, load stubs) --- */

WORD swap(WORD victim) {
  static int swaptype = -1;
  static unsigned short testpattern = 0x0102;
  unsigned short temp;
  if (swaptype < 0) {
    if (*(char *)&testpattern == 0x01) swaptype = 1; else swaptype = 0;
  }
  if (swaptype == 0) return victim;
  temp = ((victim & 0xff) << 8) | ((victim & 0xff00) >> 8);
  return (WORD)temp;
}

UWORD udiv(LONG dividend, UWORD divisor, UWORD *remainder) {
  if (divisor == 0) { *remainder = 0; return 0; }
  *remainder = (UWORD)(dividend % (LONG)divisor);
  return (UWORD)(dividend / (LONG)divisor);
}

UWORD load68k(BYTE *info) { (void)info; return 3; /* load error */ }

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

struct _filetyps load_tbl[] = {
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
  UBYTE recbuf[128];
};

/*
 * One sequential record via BDOS 20. Status 1 (EOF/unwritten/hole) is returned
 * to the streaming core, which zero-fills mid-image holes. When status is 1 and
 * cur_rec did not advance, bump it so the next call progresses (holes / sparse).
 */

static UWORD pgm_read_rec(UBYTE rec[128], void *vctx)
{
  struct pgm_read_ctx *c = (struct pgm_read_ctx *)vctx;
  UWORD r;
  UBYTE prev_cr = c->fcb[32];
  int i;

  bdos(26, (LONG)(unsigned long)c->recbuf);
  r = bdos(20, (LONG)(unsigned long)c->fcb);

  for (i = 0; i < 128; i++)
    rec[i] = c->recbuf[i];

  if (r == 1 && c->fcb[32] == prev_cr) {
    /*
     * Unwritten/hole: BDOS left cur_rec unchanged - advance like a skip.
     * cur_rec is UBYTE 0..128; 128 triggers new_ext on the next read.
     */

    if ((unsigned)c->fcb[32] < 128u)
      c->fcb[32] = (UBYTE)(c->fcb[32] + 1);
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
  static UBYTE kdma[128];

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
  unsigned long ulen = pmode_tpa_len();
  unsigned long user_esp;
  const unsigned long stack_room = 0x100000UL;

  if (ulen < 0x2000)
    return 0xFFFD;

  user_esp = (ulen > stack_room) ? stack_room : (ulen & ~0xFFFUL);
  user_esp -= 16;
  enter_ring3(entry_off, user_esp);
  pgm_after_exit();

  return 0;
}

UWORD pgmld(UBYTE *infop, UBYTE *dmaadr) {
  (void)dmaadr;
  mrt_t *mrt;
  UBYTE *tpa_base;
  unsigned long tpa_len;
  UBYTE *entry = 0;
  struct pgm_read_ctx ctx;
  BYTE loadfcb[36];
  int j;

  /* get TPA from BIOS mrt (same as bgetseg) */
  mrt = bios_getmrt();
  tpa_base = (UBYTE *)mrt->base;
  tpa_len = mrt->length;

  /* Clean FCB for sequential load (fresh cur_rec / dskmap; multi-extent
   * crossing is handled inside BDOS read + new_ext). */
  for (j = 0; j < 36; j++) loadfcb[j] = 0;

  loadfcb[0] = ((struct fcb *)infop)->drvcode;

  for (j = 0; j < 11; j++) loadfcb[1 + j] = ((BYTE *)infop)[1 + j];

  if (bdos(15, (LONG)loadfcb) > 3) {
    return 0xFFFE; /* cannot re-open for load read */
  }

  loadfcb[32] = 0; /* ensure sequential from record 0 */

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

UBYTE *traphndl(void) { return (UBYTE *)0; }

void initexc(UBYTE **vecs) { (void)vecs; }  /* no 68k exceptions here! */

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
