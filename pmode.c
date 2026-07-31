/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 6d8219da-82b5-11f1-b027-80ee73e9b8e7
 */

/*****************************************************************************/

/* pmode.c - GDT / IDT / TSS setup and BDOS syscall C path for ring-3 */

/*****************************************************************************/

#include "bdosinc.h"
#include "platform.h"
#include "pmode.h"
#include "io.h"
#include "vidbios.h"
#include "vidmode.h"

/*****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/*****************************************************************************/

struct gdt_entry
{
  uint16_t limit_lo;
  uint16_t base_lo;
  uint8_t base_mid;
  uint8_t access;
  uint8_t gran; /* flags << 4 | limit_hi */
  uint8_t base_hi;
} __attribute__ ((packed));

/*****************************************************************************/

struct idt_entry
{
  uint16_t off_lo;
  uint16_t sel;
  uint8_t zero;
  uint8_t type;
  uint16_t off_hi;
} __attribute__ ((packed));

/*****************************************************************************/

struct dt_ptr
{
  uint16_t limit;
  uint32_t base;
} __attribute__ ((packed));

/*****************************************************************************/

/* 32-bit TSS */
struct tss32
{
  uint32_t prev_tss;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__ ((packed));

/*****************************************************************************/

/*
 * 0-6 as before; 7 is the ring-3 graphics framebuffer (built null until a
 * graphics mode is set), 8 and 9 are the 16-bit descriptors the real mode
 * thunk needs to step down through 16-bit protected mode.
 */

#define GDT_COUNT 10
#define IDT_COUNT 256

/*****************************************************************************/

static struct gdt_entry gdt [GDT_COUNT];
static struct idt_entry idt [IDT_COUNT];
static struct tss32 tss;
static struct dt_ptr gdtp, idtp;

/*****************************************************************************/

/*
 * Dedicated ring-0 stack for privilege transitions (int from ring 3).
 * Must be large enough for BDOS (dirscan/open/extent switch) on the
 * interrupt stack; overflow would smash alv/CCP globals below it.
 */

static uint8_t ring0_stack [32768] __attribute__ ((aligned (16)));

/*****************************************************************************/

static unsigned long g_tpa_base;
static unsigned long g_tpa_len;

/*****************************************************************************/

static int pmode_ready;

/*****************************************************************************/

extern void bdos_irq (void);
extern void return_to_kernel (void);
extern UWORD _bdos (WORD func, UWORD info, UBYTE *infop);

/*****************************************************************************/

unsigned long
pmode_tpa_base (void)
{
  return g_tpa_base;
}

/*****************************************************************************/

unsigned long
pmode_tpa_len (void)
{
  return g_tpa_len;
}

/*****************************************************************************/

int
pmode_active (void)
{
  return pmode_ready && g_tpa_len > 0;
}

/*****************************************************************************/

unsigned short
pmode_vga_selector (void)
{
#if CPM386_HAS_VGA_TEXT
  extern int bios_vga_present (void);

  /* No adapter fitted means no text plane to hand ring 3. */
  if (!pmode_ready || !bios_vga_present ())
    return 0;

  return (unsigned short)SEL_UVIDEO_RPL3;
#else
  return 0;
#endif
}

/*****************************************************************************/

unsigned long
pmode_vga_phys_base (void)
{
#if CPM386_HAS_VGA_TEXT
  return (unsigned long)CPM386_VGA_TEXT_BASE;
#else
  return 0;
#endif
}

/*****************************************************************************/

unsigned long
pmode_vga_map_size (void)
{
#if CPM386_HAS_VGA_TEXT
  return (unsigned long)CPM386_VGA_TEXT_SIZE;
#else
  return 0;
#endif
}

/*****************************************************************************/

/*
 * Ring-3 graphics framebuffer.  GDT index 7 is reprogrammed on every
 * graphics mode set and nulled on the way back to text, so a program that
 * hangs on to the selector across a mode change faults instead of writing
 * into whatever now lives at that address.
 *
 * Segments are the only mapping mechanism here - there is no paging - so a
 * linear framebuffer needs nothing more than a descriptor pointing at its
 * physical address.
 */

static unsigned long g_fb_base;
static unsigned long g_fb_size;

/*****************************************************************************/

unsigned short
pmode_fb_selector (void)
{
  if (!pmode_ready || g_fb_size == 0)
    return 0;

  return (unsigned short)SEL_UFB_RPL3;
}

/*****************************************************************************/

unsigned long
pmode_fb_base (void)
{
  return g_fb_base;
}

/*****************************************************************************/

unsigned long
pmode_fb_size (void)
{
  return g_fb_size;
}

/*****************************************************************************/

/* GDT helpers */

static void
gdt_set (int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
  /*
   * flags: typically 0xC = G=1 (4K), D/B=1 (32-bit). limit is byte length-1
   * or page count-1 depending on G.
   */

  gdt [idx].limit_lo = (uint16_t)(limit & 0xFFFF);
  gdt [idx].base_lo = (uint16_t)(base & 0xFFFF);
  gdt [idx].base_mid = (uint8_t)((base >> 16) & 0xFF);
  gdt [idx].access = access;
  gdt [idx].gran = (uint8_t)((flags & 0xF0) | ((limit >> 16) & 0x0F));
  gdt [idx].base_hi = (uint8_t)((base >> 24) & 0xFF);
}

/*****************************************************************************/

/*
 * Point the ring-3 framebuffer selector at base for size bytes, or null it
 * when size is 0.  Changing a descriptor already covered by the loaded GDT
 * takes effect on the next selector load, so no reload is needed.
 */

void
pmode_set_fb (unsigned long base, unsigned long size)
{
  if (!pmode_ready || size == 0)
    {
      g_fb_base = 0;
      g_fb_size = 0;
      gdt_set (7, 0, 0, 0, 0);

      return;
    }

  g_fb_base = base;
  g_fb_size = size;

  /*
   * Byte granularity caps a segment at 1MB, which is fine for the 64K
   * window at 0xA0000 but not for a linear framebuffer; switch to page
   * granularity once the region is large enough to need it.
   */

  if (size <= 0x100000UL)
    {
      gdt_set (7, (uint32_t)base, (uint32_t)(size - 1), 0xF2, 0x40);
    }
  else
    {
      uint32_t pages = (uint32_t)((size + 0xFFFUL) >> 12);

      gdt_set (7, (uint32_t)base, pages - 1, 0xF2, 0xC0);
    }
}

/*****************************************************************************/

static void
idt_set (int vec, void (*handler) (void), uint8_t type)
{
  uint32_t off = (uint32_t)(unsigned long)handler;

  idt [vec].off_lo = (uint16_t)(off & 0xFFFF);
  idt [vec].sel = SEL_KCODE;
  idt [vec].zero = 0;
  idt [vec].type = type;
  idt [vec].off_hi = (uint16_t)((off >> 16) & 0xFFFF);
}

/*****************************************************************************/

/* Fallback for IRQ vectors etc.: freeze (PIC is masked; should be rare). */
static void
fault_hang (void)
{
  for (;;)
    {
      __asm__ volatile ("cli; hlt");
    }
}

/*****************************************************************************/

/* Print via BIOS console (serial + VGA as configured). Safe from ring 0. */
extern void bios_conout (unsigned char c);

/*****************************************************************************/

/*
 * Stack image built by exc_common (pmode.S). Field order must match the
 * push sequence exactly (lowest address first).
 */

struct fault_frame
{
  uint32_t ebp, edi, esi, edx;
  uint32_t ecx, ebx, eax;
  uint32_t gs, fs, es, ds;
  uint32_t vec, err, eip, cs, eflags;
  /* Present only when the fault originated at CPL=3: */
  uint32_t uesp, uss;
};

/*****************************************************************************/

static void
fault_puts (const char *s)
{
  while (*s)
    {
      bios_conout ((unsigned char)*s++);
    }
}

/*****************************************************************************/

static void
fault_puthex (uint32_t v, int digits)
{
  static const char hex [] = "0123456789ABCDEF";
  int i;

  if (digits < 1)
    {
      digits = 1;
    }

  if (digits > 8)
    {
      digits = 8;
    }

  for (i = digits - 1; i >= 0; i--)
    {
      bios_conout ((unsigned char)hex [(v >> (i * 4)) & 0xF]);
    }
}

/*****************************************************************************/

static void
fault_puthex32 (uint32_t v)
{
  fault_puthex (v, 8);
}

/*****************************************************************************/

/* DOS/4GW-style exception names (vectors 0-19; rest numeric). */
static const char *
fault_name (uint32_t vec)
{
  static const char *const names [] = {
    "#DE divide error",
    "#DB debug",
    "NMI",
    "#BP breakpoint",
    "#OF overflow",
    "#BR bound range",
    "#UD invalid opcode",
    "#NM device not available",
    "#DF double fault",
    "coprocessor segment",
    "#TS invalid TSS",
    "#NP segment not present",
    "#SS stack fault",
    "#GP general protection",
    "#PF page fault",
    "reserved",
    "#MF x87 FPU",
    "#AC alignment check",
    "#MC machine check",
    "#XM SIMD FP",
  };

  if (vec < sizeof (names) / sizeof (names [0]))
    {
      return names [vec];
    }

  return "unknown";
}

/*****************************************************************************/

static void
fault_put_pair (const char *name, uint32_t v)
{
  fault_puts (name);
  fault_puthex32 (v);
}

/*****************************************************************************/

/*
 * DOS/4GW-inspired register dump for CPU exceptions.
 * Called from exc_common with a pointer to the saved frame on the ring-0
 * stack. Returns 1 to abort the ring-3 program (return_to_kernel -> CCP),
 * 0 to hang (kernel / unrecoverable fault).
 */

uint32_t
fault_handler_c (struct fault_frame *f)
{
  uint32_t cr0, cr2, cr3;
  int from_user = ((f->cs & 3u) == 3u);
  uint32_t esp_at_fault;
  uint32_t ss_at_fault;

  if (from_user)
    {
      esp_at_fault = f->uesp;
      ss_at_fault = f->uss;
    }
  else
    {
      /* Ring-0 fault: no SS:ESP pushed; approximate kernel ESP past eflags. */
      esp_at_fault = (uint32_t)(unsigned long)&f->eflags + 4u;
      ss_at_fault = SEL_KDATA;
    }

  __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
  __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
  __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

  /*
   * Get the display back into a readable text mode before printing a
   * single character, or a program that died in a graphics mode leaves the
   * dump as pixel noise.  Deliberately conservative:
   *
   *   - Nothing happens unless the mode actually differs from the console
   *     mode, so the overwhelmingly common case costs nothing and risks
   *     nothing.
   *   - Skipped outright if the fault came from inside the thunk itself,
   *     which would otherwise re-enter it from its own failure path.
   *
   * Serial output is unaffected by the video mode, so on a machine with a
   * serial console the dump survives regardless of what happens here.
   */

  {
    extern int bios_vga_present (void);

    if (bios_vga_present () && !vid_thunk_busy ())
      {
        vidmode_restore_console ();
      }
  }

  fault_puts ("\r\n");
  fault_puts ("CP/M-386 exception ");
  fault_puthex (f->vec, 2);
  fault_puts ("h (");
  fault_puts (fault_name (f->vec));
  fault_puts (")\r\n");

  /* CS:EIP and error code - DOS/4GW header style */
  fault_puts ("CS:EIP ");
  fault_puthex (f->cs, 4);
  fault_puts (":");
  fault_puthex32 (f->eip);
  fault_puts ("  Err ");
  fault_puthex32 (f->err);

  if (from_user && g_tpa_base)
    {
      fault_puts ("  linear ");
      fault_puthex32 ((uint32_t)g_tpa_base + f->eip);
    }

  fault_puts ("\r\n");

  /* General registers - four per line */
  fault_put_pair ("EAX=", f->eax);
  fault_put_pair (" EBX=", f->ebx);
  fault_put_pair (" ECX=", f->ecx);
  fault_put_pair (" EDX=", f->edx);
  fault_puts ("\r\n");
  fault_put_pair ("ESI=", f->esi);
  fault_put_pair (" EDI=", f->edi);
  fault_put_pair (" EBP=", f->ebp);
  fault_put_pair (" ESP=", esp_at_fault);
  fault_puts ("\r\n");

  /* Segment registers */
  fault_puts ("CS=");
  fault_puthex (f->cs, 4);
  fault_puts (" DS=");
  fault_puthex (f->ds, 4);
  fault_puts (" ES=");
  fault_puthex (f->es, 4);
  fault_puts (" FS=");
  fault_puthex (f->fs, 4);
  fault_puts (" GS=");
  fault_puthex (f->gs, 4);
  fault_puts (" SS=");
  fault_puthex (ss_at_fault, 4);
  fault_puts ("\r\n");

  fault_put_pair ("EFL=", f->eflags);
  fault_puts ("  (");

  if (f->eflags & (1u << 0))
    {
      fault_puts ("CF ");
    }

  if (f->eflags & (1u << 2))
    {
      fault_puts ("PF ");
    }

  if (f->eflags & (1u << 4))
    {
      fault_puts ("AF ");
    }

  if (f->eflags & (1u << 6))
    {
      fault_puts ("ZF ");
    }

  if (f->eflags & (1u << 7))
    {
      fault_puts ("SF ");
    }

  if (f->eflags & (1u << 8))
    {
      fault_puts ("TF ");
    }

  if (f->eflags & (1u << 9))
    {
      fault_puts ("IF ");
    }

  if (f->eflags & (1u << 10))
    {
      fault_puts ("DF ");
    }

  if (f->eflags & (1u << 11))
    {
      fault_puts ("OF ");
    }

  fault_puts ("IOPL=");
  fault_puthex ((f->eflags >> 12) & 3u, 1);

  if (f->eflags & (1u << 14))
    {
      fault_puts (" NT");
    }

  if (f->eflags & (1u << 16))
    {
      fault_puts (" RF");
    }

  if (f->eflags & (1u << 17))
    {
      fault_puts (" VM");
    }

  fault_puts (")\r\n");

  /* Control registers (CR2 is meaningful for #PF) */
  fault_put_pair ("CR0=", cr0);
  fault_put_pair (" CR2=", cr2);
  fault_put_pair (" CR3=", cr3);
  fault_puts ("\r\n");

  if (from_user)
    {
      fault_puts ("Ring-3 protection fault - program aborted.\r\n");
      return 1;
    }

  fault_puts ("Kernel fault - system halted.\r\n");

  return 0;
}

/*****************************************************************************/

/* Exception entry stubs (pmode.S); one pointer per vector 0..31 */
extern void (*exc_stub_table [32]) (void);

/*****************************************************************************/

/*
 * Bytes the BDOS will touch at the caller's pointer, for the calls whose
 * payload is a fixed-size object.  Returning 0 means "not known here" and
 * keeps the historical start-offset-only check for that call - correct for
 * genuinely variable-length payloads such as function 9's '$'-terminated
 * string, function 10's caller-sized line buffer, and function 27, whose
 * length depends on a DPB that has not been selected yet.
 *
 * Without this the only validation is that the *first* byte is inside the
 * TPA, so an object placed within sizeof(object)-1 bytes of the top lets
 * the BDOS write past the end of the ring-3 segment.
 */

#define ARGLEN_FCB 36     /* struct fcb                      */
#define ARGLEN_DMA 128    /* one CP/M record                 */
#define ARGLEN_DPB 16     /* struct dpb                      */
#define ARGLEN_TPA 12     /* struct set_tpa_struct           */
#define ARGLEN_SERIAL 6   /* S_SERIAL number                 */
#define ARGLEN_VGATEXT 16 /* struct cpm_vga_text             */
#define ARGLEN_TICKS 12   /* struct cpm_ticks                */
#define ARGLEN_MEMLAY 36  /* struct cpm_memlayout            */
#define ARGLEN_VIDMODE 32 /* struct cpm_vidmode              */
#define ARGLEN_VIDSET 8   /* struct cpm_vidset               */
#define ARGLEN_VIDFONT 12 /* struct cpm_vidfont              */
#define ARGLEN_VIDPAL 12  /* struct cpm_vidpal               */

static unsigned long
bdos_arg_len (WORD func)
{
  switch (func)
    {
    case 15: /* open           */
    case 16: /* close          */
    case 17: /* search first   */
    case 18: /* search next    */
    case 19: /* delete         */
    case 20: /* read seq       */
    case 21: /* write seq      */
    case 22: /* make           */
    case 23: /* rename         */
    case 30: /* set file attrs */
    case 33: /* read random    */
    case 34: /* write random   */
    case 35: /* file size      */
    case 36: /* set random rec */
    case 40: /* write ran zero */
    case 59: /* program load   */
    case 99: /* truncate       */
      return ARGLEN_FCB;

    case 26: /* set DMA */
      return ARGLEN_DMA;

    case 31: /* get DPB */
      return ARGLEN_DPB;

    case 63: /* get/set TPA */
      return ARGLEN_TPA;

    case 107: /* get serial number */
      return ARGLEN_SERIAL;

    case 224: /* BDOS_CON_VIDEO */
      return ARGLEN_VGATEXT;

    case 225: /* BDOS_GET_TICKS   */
    case 226: /* BDOS_SLEEP_UNTIL */
      return ARGLEN_TICKS;

    case 228: /* BDOS_MEM_LAYOUT */
      return ARGLEN_MEMLAY;

    case 229: /* BDOS_VID_QUERY */
    case 230: /* BDOS_VID_ENUM  */
      return ARGLEN_VIDMODE;

    case 231: /* BDOS_VID_SET */
      return ARGLEN_VIDSET;

    case 232: /* BDOS_VID_FONT */
      return ARGLEN_VIDFONT;

    case 233: /* BDOS_VID_PALETTE */
      return ARGLEN_VIDPAL;

    default:
      return 0;
    }
}

/*****************************************************************************/

/* pointer-arg BDOS functions (info is TPA-relative from ring 3!) */

static int
bdos_arg_is_ptr (WORD func)
{
  switch (func)
    {
    case 9:   /* print string */
    case 10:  /* read console buffer */
    case 15:  /* open */
    case 16:  /* close */
    case 17:  /* search first */
    case 18:  /* search next (sometimes DMA only; still ok) */
    case 19:  /* delete */
    case 20:  /* read seq */
    case 21:  /* write seq */
    case 22:  /* make */
    case 23:  /* rename */
    case 26:  /* set DMA */
    case 27:  /* get allocation vector (copied to caller buffer) */
    case 30:  /* set file attrs */
    case 31:  /* get DPB (CP/M-68K copies into caller buffer) */
    case 33:  /* read random */
    case 34:  /* write random */
    case 35:  /* compute file size */
    case 36:  /* set random record */
    case 40:  /* write random filled */
    case 59:  /* program load */
    case 99:  /* truncate file */
    case 63:  /* get/set TPA */
    case 104: /* set date/time */
    case 105: /* get date/time */
    case 200: /* P2DOS get time */
    case 201: /* P2DOS set time */
    case 107: /* get serial number */
    case 152: /* F_PARSE PFCB */
    case 155: /* T_SECONDS get date/time BCD */
    case 224: /* BDOS_CON_VIDEO - fill cpm_vga_text */
    case 225: /* BDOS_GET_TICKS - fill cpm_ticks */
    case 226: /* BDOS_SLEEP_UNTIL - read cpm_ticks target */
    case 228: /* BDOS_MEM_LAYOUT - fill cpm_memlayout */
    case 229: /* BDOS_VID_QUERY - fill cpm_vidmode    */
    case 230: /* BDOS_VID_ENUM  - fill cpm_vidmode    */
    case 231: /* BDOS_VID_SET   - read cpm_vidset     */
    case 232: /* BDOS_VID_FONT  - read cpm_vidfont    */
    case 233: /* BDOS_VID_PALETTE - read cpm_vidpal   */
      return 1;

    default:
      return 0;
    }
}

/*****************************************************************************/

/* C side of int 0x30. Called from bdos_irq with kernel DS. */
UWORD
bdos_syscall_c (WORD func, unsigned long info, unsigned long user_cs)
{
  unsigned long infop = info;
  UWORD r;

  /* keep 64-bit PIT extension accurate */
  {
    extern void pit_poll (void);
    pit_poll ();
  }

  /* Program exit from ring 3: resume kernel after enter_ring3 */
  if (func == 0 && (user_cs & 3) == 3)
    {
      return_to_kernel ();

      /* not reached */
      return 0;
    }

  if (bdos_arg_is_ptr (func) && pmode_ready)
    {
      unsigned long need = bdos_arg_len (func);

      /* user pointer is TPA-relative; reject out-of-range */
      if (info >= g_tpa_len)
        {
          return 0xFFFF;
        }

      /*
       * ...and reject an object that starts inside the TPA but would be
       * written past the end of it.  Calls with no fixed size keep the
       * start-only check.
       */

      if (need > 0 && need > g_tpa_len - info)
        {
          return 0xFFFF;
        }

      infop = g_tpa_base + info;
    }

  r = _bdos (func, (UWORD)info, (UBYTE *)(unsigned long)infop);

  return r;
}

/*****************************************************************************/

void
pmode_init (unsigned long tpa_base, unsigned long tpa_len)
{
  uint32_t ulim;
  int i;

  /* Cap so every user offset maps into real RAM (no 4G wrap, no MMIO). */
  {
    const unsigned long usable_top = 0xE0000000UL;
    if (tpa_base >= usable_top)
      {
        tpa_base = 0x100000;
        tpa_len = 0x10000;
      }
    else if (tpa_len > usable_top - tpa_base)
      {
        tpa_len = usable_top - tpa_base;
      }

    if (tpa_base + tpa_len < tpa_base)
      {
        tpa_len = (unsigned long)0 - tpa_base;
      }
  }

  g_tpa_base = tpa_base;
  g_tpa_len = tpa_len;

  /*
   * Mask all PIC IRQs - we poll console and have no IRQ handlers yet.
   * Leaving them unmasked with IF=1 in ring-3 would vector to fault_hang.
   */

  outb (0x21, 0xFF);
  outb (0xA1, 0xFF);

  /* PIT for BDOS 225/226 high-res ticks */
  {
    extern void pit_init (void);
    pit_init ();
  }

  /* Null */
  gdt_set (0, 0, 0, 0, 0);

  /* 0x08 kernel code: base 0, 4G, DPL0, RX, 32-bit, page gran */
  gdt_set (1, 0, 0xFFFFF, 0x9A, 0xC0);
  /* 0x10 kernel data: base 0, 4G, DPL0, RW */
  gdt_set (2, 0, 0xFFFFF, 0x92, 0xC0);

  /* User segments: base = TPA, limit in pages (round down to 4K) */
  if (tpa_len < 0x1000)
    {
      tpa_len = 0x1000;
    }

  ulim = (uint32_t)((tpa_len >> 12) - 1); /* pages - 1 */
  /* 0x18 user code DPL3 */
  gdt_set (3, (uint32_t)tpa_base, ulim, 0xFA, 0xC0);
  /* 0x20 user data DPL3 */
  gdt_set (4, (uint32_t)tpa_base, ulim, 0xF2, 0xC0);

  /* TSS descriptor 0x28 */
  {
    uint32_t tb = (uint32_t)(unsigned long)&tss;
    uint32_t tl = sizeof (tss) - 1;

    gdt_set (5, tb, tl, 0x89, 0x00); /* 32-bit TSS available, byte gran */
  }

  /*
   * 0x30 user VGA text: base/size from platform.h (default B8000 / 32K).
   * Byte granularity, 32-bit data, DPL=3.
   */

#if CPM386_HAS_VGA_TEXT
  {
    uint32_t vbase = (uint32_t)CPM386_VGA_TEXT_BASE;
    uint32_t vlim = (uint32_t)CPM386_VGA_TEXT_SIZE - 1u; /* byte gran */

    gdt_set (6, vbase, vlim, 0xF2, 0x40); /* DPL3 RW data, D=1, G=0 */
  }
#else
  gdt_set (6, 0, 0, 0, 0);
#endif

  /*
   * 0x38 ring-3 framebuffer: null until a graphics mode is set.
   */

  gdt_set (7, 0, 0, 0, 0);

  /*
   * 0x40 / 0x48 16-bit code and data, base 0, limit 64K-1, byte
   * granularity, D/B = 0.  Used only by the int 10h thunk to reach real
   * mode; the trampoline lives below 0x10000 because a 16-bit code segment
   * only has a 16-bit IP.
   */

  gdt_set (8, 0, 0xFFFF, 0x9A, 0x00);
  gdt_set (9, 0, 0xFFFF, 0x92, 0x00);

  /* TSS contents */
  for (i = 0; i < (int)sizeof (tss); i++)
    {
      ((uint8_t *)&tss) [i] = 0;
    }

  tss.ss0 = SEL_KDATA;
  tss.esp0 = (uint32_t)(unsigned long)(ring0_stack + sizeof (ring0_stack));
  tss.iomap_base = sizeof (tss);

  gdtp.limit = sizeof (gdt) - 1;
  gdtp.base = (uint32_t)(unsigned long)&gdt;

  /*
   * IDT: CPU exceptions 0-31 have recover-capable stubs; rest hang.
   * BDOS int 0x30 is DPL3 so ring-3 may call it.
   */

  for (i = 0; i < IDT_COUNT; i++)
    {
      idt_set (i, fault_hang, 0x8E); /* present, DPL0, 32-bit interrupt gate */
    }

  for (i = 0; i < 32; i++)
    {
      idt_set (i, (void (*) (void))exc_stub_table [i], 0x8E);
    }

  idt_set (BDOS_INT, bdos_irq, 0xEE); /* present, DPL3 */

  idtp.limit = sizeof (idt) - 1;
  idtp.base = (uint32_t)(unsigned long)&idt;

  __asm__ volatile ("lgdt %0\n"
                    "lidt %1\n"
                    /* reload kernel data segments (CS via far jump) */
                    "mov $0x10, %%ax\n"
                    "mov %%ax, %%ds\n"
                    "mov %%ax, %%es\n"
                    "mov %%ax, %%fs\n"
                    "mov %%ax, %%gs\n"
                    "mov %%ax, %%ss\n"
                    "ljmp $0x08, $1f\n"
                    "1:\n"
                    "mov $0x28, %%ax\n"
                    "ltr %%ax\n"
                    :
                    : "m"(gdtp), "m"(idtp)
                    : "eax", "memory");

  pmode_ready = 1;
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
