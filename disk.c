/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 51ca46b0-9104-11f1-8e36-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * disk.c - CBIOS disk layer for the V86 int 13h server
 */

/*****************************************************************************/

#include "bdosinc.h"
#include "biosdef.h"
#include "bringup.h"
#include "disk.h"
#include "io.h"
#include "pit.h"
#include "pmode.h"

/*****************************************************************************/

/* The real-mode blob, linked into the kernel and copied to V86_CODE_ADDR. */

extern unsigned char disk_v86_start [];
extern unsigned char disk_v86_end [];

/*****************************************************************************/

/*
 * Low memory is addressed through these rather than plain pointers so the
 * compiler cannot decide a constant address is a null pointer and delete
 * the access.  All of them are non-zero by construction.
 */

#define LOW8(a) (*(volatile unsigned char *)(unsigned long)(a))
#define LOW16(a) (*(volatile unsigned short *)(unsigned long)(a))
#define LOW32(a) (*(volatile unsigned long *)(unsigned long)(a))

/*****************************************************************************/

#define PHYS_SECTOR 512 /* BIOS sector size we assume */
#define CPM_SECTOR 128 /* CP/M sector size */
#define CPM_PER_PHYS (PHYS_SECTOR / CPM_SECTOR)

/*****************************************************************************/

/* CP/M drive numbers (0 = A:) */
#define DRV_RAM 0
#define DRV_FLOPPY 1
#define DRV_HD0 2
#define DRV_HD1 3
#define DRV_COUNT 4

/*****************************************************************************/

struct bios_drive
{
  unsigned char bios_num; /* int 13h DL: 0x00 floppy, 0x80 first hard disk */
  signed char probed;     /* -1 not tried, 0 absent, 1 present             */
  unsigned char use_lba;  /* EDD packet calls available                    */
  unsigned short cyls;    /* physical geometry, only used without EDD      */
  unsigned short heads;
  unsigned short secs;
};

static struct bios_drive drives [DRV_COUNT] = {
  { 0x00,  0, 0, 0, 0, 0 }, /* A: is the RAM disk; never probed */
  { 0x00, -1, 0, 0, 0, 0 },
  { 0x80, -1, 0, 0, 0, 0 },
  { 0x81, -1, 0, 0, 0, 0 },
};

/*****************************************************************************/

/*
 * Disk parameter blocks.  These are fixed formats rather than something
 * derived from the probed geometry: the on-disk layout has to be agreed
 * with whatever wrote the image (cpmtools, say), so it cannot quietly
 * change with the medium.  See the diskdefs file in this directory.
 *
 * Both use 2 KiB blocks.  That is not arbitrary: over 255 blocks the
 * allocation map holds 16-bit block numbers, so eight of them fit in a
 * directory entry, and 8 x 2 KiB is exactly one 16 KiB logical extent.
 * One extent per entry is what EXM=0 means, and it is what the CCP's DIR
 * assumes when it lists only the entries whose extent byte is zero.
 *
 * Floppy - 1.44 MB, 512-byte physical sectors:
 *   72 CP/M sectors per track (18 x 512 / 128), 160 tracks, 720 blocks,
 *   256 directory entries in the first 4 blocks.
 *
 * Hard disk - the first 8 MiB of the medium:
 *   32 CP/M sectors per track (4 KiB), 2048 tracks, 4096 blocks, 512
 *   directory entries in the first 8 blocks.
 */

static UBYTE f_dirbuf [128];
static UBYTE f_csv [64];
static UBYTE f_alv [128];

static struct dpb dpb_floppy = {
  72,       /* spt: 18 physical sectors x 4                              */
  4, 15, 0, /* bsh blm exm: 2 KiB blocks, one extent per directory entry */
  0, 719,   /* dsm: 1440 KiB / 2 KiB - 1                                 */
  255,      /* drm: 256 directory entries                                */
  0x000F,   /* dir_al: first 4 blocks                                    */
  64,       /* cks: removable, so checksum the whole directory           */
  0         /* trk_off                                                   */
};

static struct dph dph_floppy
    = { 0, 0, 0, 0, f_dirbuf, &dpb_floppy, f_csv, f_alv };

static UBYTE h_dirbuf [2] [128];
static UBYTE h_alv [2] [512];

static struct dpb dpb_hd = {
  32,       /* spt: 16 physical sectors, 4 KiB per track                 */
  4, 15, 0, /* bsh blm exm: 2 KiB blocks, one extent per directory entry */
  0, 4095,  /* dsm: 8 MiB / 2 KiB - 1                                    */
  511,      /* drm: 512 directory entries                                */
  0x00FF,   /* dir_al: first 8 blocks                                    */
  0,        /* cks: fixed medium, no checksumming                        */
  0         /* trk_off                                                   */
};

static struct dph dph_hd [2] = {
  { 0, 0, 0, 0, h_dirbuf [0], &dpb_hd, 0, h_alv [0] },
  { 0, 0, 0, 0, h_dirbuf [1], &dpb_hd, 0, h_alv [1] },
};

/*****************************************************************************/

/* Selected-drive state, set by the CBIOS calls BDOS makes before a transfer */

static unsigned short cur_trk;
static unsigned short cur_sec;
static void *cur_dma;
static int cur_drive = DRV_RAM;

/*****************************************************************************/

/*
 * One-sector cache: BDOS walks 128-byte sectors, so without this every
 * physical sector would be fetched four times over. Write-through, so
 * there is never anything dirty to flush.
 */

static unsigned long cache_lba;
static int cache_drive = -1;

/*****************************************************************************/

static int v86_live;

/*****************************************************************************/

/*
 * Floppy motor shutdown.
 *
 * Every PC BIOS turns the drive motor off from its timer tick: int 08h
 * decrements the motor countdown in the BIOS data area and drops the
 * controller's motor lines when it reaches zero.  CP/M-386 has no timer
 * interrupt, so on real hardware the motor would simply keep spinning after
 * the last access - unpleasant, and hard on the drive.
 *
 * Rather than reach into the BIOS's own state to stop it, which means
 * knowing where that BIOS keeps its data, the kernel hands the ROM the
 * ticks it is owed: after a floppy access, disk_poll() runs the real int 08h
 * handler at 18.2 Hz until the BIOS has had long enough to time the motor
 * out by itself.  Nothing here needs to know how long that is - a count
 * comfortably past the ~2 seconds every BIOS uses is enough, and each new
 * access restarts it.
 *
 * The ticks are delivered from the console-poll and BDOS-entry paths, which
 * between them cover everything except a program that neither touches the
 * console nor calls the BDOS.  Such a program delays the motor stopping; it
 * cannot prevent it.
 */

#define FLOPPY_TICK_HZ 18 /* the classic 18.2 Hz tick */
#define FLOPPY_TICK_PERIOD (PIT_HZ / FLOPPY_TICK_HZ)
#define FLOPPY_SPINDOWN_TICKS 64 /* ~3.5s; BIOSes use ~2s */

static unsigned int spindown_left;
static unsigned long next_tick_lo, next_tick_hi;
static int in_poll;

/*****************************************************************************/

static void
spindown_arm (void)
{
  unsigned long lo, hi;

  pit_read (&lo, &hi);

  next_tick_lo = lo + FLOPPY_TICK_PERIOD;
  next_tick_hi = hi + (next_tick_lo < lo);

  spindown_left = FLOPPY_SPINDOWN_TICKS;
}

/*****************************************************************************/

/*
 * IRQs the V86 task is allowed to see, as a PIC master mask (0 = enabled).
 * Only IRQ 6: the ROM's floppy code waits on it and will otherwise time out
 * with status 80h.  The ATA path polls and needs nothing.  IRQ 0 is
 * deliberately left masked - the ROM's timer tick would fire at whatever
 * rate pit.c has the PIT set to, and its floppy motor-off handling has no
 * business running in the middle of a transfer.
 */

#define V86_PIC1_MASK ((unsigned char)~(1u << 6))

/*****************************************************************************/

/*
 * Run the V86 task with port access and those IRQs live, and take both away
 * again the moment it stops.  Interrupts reach it only here: the kernel
 * proper runs with the PIC fully masked, and every entry back into the
 * kernel is through an interrupt gate, so IF is clear by the time this
 * returns however it returns - fault included.
 */

static void
v86_run (void)
{
  unsigned char mask = inb (0x21);

  pmode_v86_io (1);
  outb (0x21, (unsigned char)(mask & V86_PIC1_MASK));

  v86_resume ();

  outb (0x21, mask);
  pmode_v86_io (0);
}

/*****************************************************************************/

static void
disk_puts (const char *s)
{
  while (*s)
    {
      bios_conout ((unsigned char)*s++);
    }
}

/*****************************************************************************/

static void
disk_putu (unsigned long v)
{
  char buf [12];
  int n = 0;

  if (!v)
    {
      bios_conout ('0');

      return;
    }

  while (v && n < (int)sizeof (buf))
    {
      buf [n++] = (char)('0' + (v % 10));
      v /= 10;
    }

  while (n)
    {
      bios_conout ((unsigned char)buf [--n]);
    }
}

/*****************************************************************************/

/*
 * Start (or restart) the V86 task.  Runs it as far as its first yield, at
 * which point it is parked inside disk_v86.s waiting for a call to be set
 * up in v86_state.
 */

static int
v86_boot (void)
{
  unsigned long len = (unsigned long)(disk_v86_end - disk_v86_start);
  unsigned long i;

  v86_live = 0;

  for (i = 0; i < len; i++)
    {
      LOW8 (V86_CODE_ADDR + i) = disk_v86_start [i];
    }

  for (i = 0; i < sizeof (v86_state) / sizeof (unsigned long); i++)
    {
      ((unsigned long *)&v86_state) [i] = 0;
    }

  v86_state.cs = V86_CODE_ADDR >> 4;
  v86_state.eip = 0;
  v86_state.ss = 0;
  v86_state.esp = V86_STACK_TOP;
  v86_state.eflags = V86_EFLAGS;

  v86_clear_fault ();
  v86_run ();

  if (v86_faulted ())
    {
      disk_puts ("disk: V86 server failed to start\r\n");

      return -1;
    }

  v86_live = 1;

  return 0;
}

/*****************************************************************************/

/*
 * Run one real-mode interrupt handler in the V86 task.  The registers go in
 * and come back through v86_state; only the vector's IVT entry has to be
 * handed over separately, because the task reaches it with a far CALL
 * rather than an INT (see disk_v86.s).
 */

static int
v86_call (unsigned int vec)
{
  LOW32 (V86_FARPTR_ADDR) = LOW32 (vec * 4u);

  v86_clear_fault ();
  v86_run ();

  if (v86_faulted ())
    {
      /*
       * The task's saved state is whatever it was before the fault, so it
       * cannot be resumed; force a rebuild on the next call.
       */

      v86_live = 0;

      return -1;
    }

  return 0;
}

/*****************************************************************************/

/*
 * One int 13h call.  Returns 0 on success, the BIOS status byte (always
 * non-zero) on a reported failure, or -1 if the server itself died.
 */

static int
int13 (unsigned long ax, unsigned long bx, unsigned long cx, unsigned long dx,
       unsigned long es, unsigned long si)
{
  int status;

  if (!v86_live && v86_boot () != 0)
    {
      return -1;
    }

  v86_state.eax = ax;
  v86_state.ebx = bx;
  v86_state.ecx = cx;
  v86_state.edx = dx;
  v86_state.esi = si;
  v86_state.edi = 0;
  v86_state.ebp = 0;
  v86_state.ds = 0;
  v86_state.es = es;
  v86_state.fs = 0;
  v86_state.gs = 0;
  v86_state.eflags = V86_EFLAGS;
  v86_state.esp = V86_STACK_TOP;

  if (v86_call (0x13) != 0)
    {
      return -1;
    }

  /* Any floppy access restarts the motor countdown; see disk_poll. */
  if ((dx & 0xFFu) < 0x80u)
    {
      spindown_arm ();
    }

  if (!(v86_state.eflags & 1u)) /* CF clear: success */
    {
      return 0;
    }

  status = (int)((v86_state.eax >> 8) & 0xFFu);

  return status ? status : 0xFF;
}

/*****************************************************************************/

/*
 * Hand the ROM one timer tick, from wherever the kernel next passes through
 * an idle point, until the floppy motor has had time to stop.  Cheap to call
 * and does nothing at all unless a floppy has been touched.
 */

void
disk_poll (void)
{
  unsigned long lo, hi, prev;

  if (!spindown_left || in_poll || !v86_live)
    {
      return;
    }

  pit_read (&lo, &hi);

  if (hi < next_tick_hi || (hi == next_tick_hi && lo < next_tick_lo))
    {
      return;
    }

  in_poll = 1;

  v86_state.eax = 0;
  v86_state.ebx = 0;
  v86_state.ecx = 0;
  v86_state.edx = 0;
  v86_state.esi = 0;
  v86_state.edi = 0;
  v86_state.ebp = 0;
  v86_state.ds = 0;
  v86_state.es = 0;
  v86_state.fs = 0;
  v86_state.gs = 0;
  v86_state.eflags = V86_EFLAGS;
  v86_state.esp = V86_STACK_TOP;

  (void)v86_call (0x08);

  spindown_left--;


  prev = next_tick_lo;
  next_tick_lo = prev + FLOPPY_TICK_PERIOD;

  if (next_tick_lo < prev)
    {
      next_tick_hi++;
    }

  in_poll = 0;
}

/*****************************************************************************/

/* int 13h AH=08h: physical geometry, needed only when EDD is unavailable. */

static int
int13_geometry (struct bios_drive *d)
{
  unsigned long cx, dx;

  if (int13 (0x0800, 0, 0, d->bios_num, 0, 0) != 0)
    {
      return -1;
    }

  cx = v86_state.ecx;
  dx = v86_state.edx;

  d->secs = (unsigned short)(cx & 0x3Fu);
  d->cyls = (unsigned short)((((cx >> 8) & 0xFFu) | ((cx & 0xC0u) << 2)) + 1u);
  d->heads = (unsigned short)(((dx >> 8) & 0xFFu) + 1u);

  return (d->secs && d->heads) ? 0 : -1;
}

/*****************************************************************************/

/* int 13h AH=41h: are the EDD packet calls (42h/43h) available? */

static int
int13_has_edd (struct bios_drive *d)
{
  if (int13 (0x4100, 0x55AA, 0, d->bios_num, 0, 0) != 0)
    {
      return 0;
    }

  return ((v86_state.ebx & 0xFFFFu) == 0xAA55u)
         && ((v86_state.ecx & 1u) != 0);
}

/*****************************************************************************/

static int
drive_probe (int drv)
{
  struct bios_drive *d = &drives [drv];

  if (d->probed >= 0)
    {
      return d->probed;
    }

  d->probed = 0;

  if (!v86_live && v86_boot () != 0)
    {
      return 0;
    }

  /*
   * Reset first: on a cold machine the floppy controller has not been
   * touched since POST, and AH=08h on an absent drive is happier for it.
   */

  (void)int13 (0x0000, 0, 0, d->bios_num, 0, 0);

  if (d->bios_num >= 0x80)
    {
      if (int13 (0x0800, 0, 0, d->bios_num, 0, 0) != 0)
        {
          return 0;
        }

      if ((d->bios_num - 0x80) >= (v86_state.edx & 0xFFu))
        {
          return 0;
        }
    }

  d->use_lba = (unsigned char)int13_has_edd (d);

  if (int13_geometry (d) != 0)
    {
      /* No CHS geometry is fatal for a floppy but not for an EDD disk. */
      if (!d->use_lba)
        {
          return 0;
        }

      d->cyls = d->heads = d->secs = 0;
    }

  d->probed = 1;

  return 1;
}

/*****************************************************************************/

/*
 * Transfer one physical sector between V86_BUF_ADDR and the medium.
 * LBA when the BIOS offers it, CHS otherwise.
 */

static int
phys_xfer (int drv, unsigned long lba, int write)
{
  struct bios_drive *d = &drives [drv];
  unsigned long cyl, head, sec, cx;

  if (d->use_lba)
    {
      /*
       * EDD disk address packet: size, reserved, count, buffer off:seg,
       * then the 64-bit LBA.
       */

      LOW16 (V86_DAP_ADDR + 0) = 0x0010;
      LOW16 (V86_DAP_ADDR + 2) = 1;
      LOW16 (V86_DAP_ADDR + 4) = 0;
      LOW16 (V86_DAP_ADDR + 6) = (unsigned short)(V86_BUF_ADDR >> 4);
      LOW32 (V86_DAP_ADDR + 8) = lba;
      LOW32 (V86_DAP_ADDR + 12) = 0;

      /* DS is always 0 in the server, so SI is the flat address. */
      return int13 (write ? 0x4300 : 0x4200, 0, 0, d->bios_num, 0,
                    V86_DAP_ADDR);
    }

  if (!d->secs || !d->heads)
    {
      return -1;
    }

  sec = (lba % d->secs) + 1u;
  head = (lba / d->secs) % d->heads;
  cyl = lba / ((unsigned long)d->secs * d->heads);

  if (d->cyls && cyl >= d->cyls)
    {
      return 0x04; /* sector not found */
    }

  cx = ((cyl & 0xFFu) << 8) | ((cyl & 0x300u) >> 2) | sec;

  return int13 ((write ? 0x0301u : 0x0201u), 0, cx,
                (head << 8) | d->bios_num, V86_BUF_ADDR >> 4, 0);
}

/*****************************************************************************/

/*
 * Make sure physical sector lba of drv is sitting in the transfer buffer.
 * Retries once through a controller reset, which is what the BIOS expects
 * of a caller and what makes a cold floppy work on the second go.
 */

static int
phys_read (int drv, unsigned long lba)
{
  int rc;

  if (cache_drive == drv && cache_lba == lba)
    {
      return 0;
    }

  cache_drive = -1;

  rc = phys_xfer (drv, lba, 0);

  if (rc > 0)
    {
      (void)int13 (0x0000, 0, 0, drives [drv].bios_num, 0, 0);
      rc = phys_xfer (drv, lba, 0);
    }

  if (rc != 0)
    {
      return rc;
    }

  cache_drive = drv;
  cache_lba = lba;

  return 0;
}

/*****************************************************************************/

static int
phys_write (int drv, unsigned long lba)
{
  int rc = phys_xfer (drv, lba, 1);

  if (rc > 0)
    {
      (void)int13 (0x0000, 0, 0, drives [drv].bios_num, 0, 0);
      rc = phys_xfer (drv, lba, 1);
    }

  if (rc != 0)
    {
      cache_drive = -1;

      return rc;
    }

  cache_drive = drv;
  cache_lba = lba;

  return 0;
}

/*****************************************************************************/

int
disk_init (void)
{
  if (v86_live)
    {
      return 0;
    }

  return v86_boot ();
}

/*****************************************************************************/

static void
print_drive_name (unsigned bios_num)
{
  if (bios_num <= 3)
    {
      disk_puts("FD");
      disk_putu(bios_num);
    }
  else if (bios_num >= 128 && bios_num <= 255)
    {
      disk_puts("HD");
      disk_putu(bios_num - 128);
    }
  else
    {
      disk_puts("drive ");
      disk_putu(bios_num);
    }
}

/*****************************************************************************/

void
disk_report (void)
{
  static const char *const names [DRV_COUNT] = { "A", "B", "C", "D" };
  int i, rc;

  disk_puts ("\r\n");

  for (i = DRV_FLOPPY; i < DRV_COUNT; i++)
    {
      struct bios_drive *d = &drives [i];

      if (!drive_probe (i))
        {
          continue;
        }

      disk_puts (names [i]);
      disk_puts (": BIOS");
      disk_puts (d->use_lba ? " LBA " : " CHS ");
      print_drive_name(d->bios_num);

      if (d->cyls)
        {
          disk_puts (", ");
          disk_putu (d->cyls);
          disk_puts ("/");
          disk_putu (d->heads);
          disk_puts ("/");
          disk_putu (d->secs);
          disk_puts (" (");
          disk_putu ((unsigned long)d->cyls * d->heads * d->secs);
          disk_puts (" sectors), ");
          disk_putu (
            (((unsigned long)d->cyls * d->heads * d->secs) * 512UL) / 1024UL);
          disk_puts ("K");
        }

      /*
       * A drive the BIOS admits to is not necessarily one it can read
       * from - an empty floppy drive is still a drive - so say so here
       * rather than leaving it to the first BDOS Err.
       */

      rc = phys_read (i, 0);

      if (rc > 0)
        {
          disk_puts (" (not ready, status ");
          disk_putu ((unsigned long)rc);
          disk_puts (")");
        }
      else if (rc < 0)
        {
          disk_puts (" (V86 server fault!)");
        }

      disk_puts ("\r\n");
    }
}

/*****************************************************************************/
/* CBIOS entry points                                                        */
/*****************************************************************************/

void *
bios_seldsk (unsigned char drive, unsigned char logged)
{
  cur_trk = cur_sec = 0;
  cur_drive = drive;

  /*
   * A fresh login is the one moment the BDOS tells us the medium may not be
   * the one we last saw, so it is the moment to drop the sector cache.
   */

  if (!logged)
    {
      cache_drive = -1;
    }

  if (drive == DRV_RAM)
    {
      return &dph0;
    }

  if (drive >= DRV_COUNT || !drive_probe (drive))
    {
      return 0;
    }

  if (drive == DRV_FLOPPY)
    {
      return &dph_floppy;
    }

  return &dph_hd [drive - DRV_HD0];
}

/*****************************************************************************/

void
bios_settrk (unsigned short int track)
{
  cur_trk = track;
}

/*****************************************************************************/

void
bios_setsec (unsigned short int sector)
{
  cur_sec = sector;
}

/*****************************************************************************/

void
bios_setdma (void *dmaaddress)
{
  cur_dma = dmaaddress;
}

/*****************************************************************************/

unsigned short int
bios_sectran (unsigned short int sec, void *table)
{
  (void)table;

  return sec; /* no skew */
}

/*****************************************************************************/

/* A: - the RAM disk, unchanged from the original CBIOS. */

static unsigned short int
ram_rw (int write)
{
  unsigned long off
      = ((unsigned long)cur_trk * dpb0.spt + cur_sec) * CPM_SECTOR;
  unsigned char *dma = (unsigned char *)cur_dma;
  int i;

  if (off + CPM_SECTOR > RAMDISK_SIZE)
    {
      return 1;
    }

  if (!dma)
    {
      return 0;
    }

  for (i = 0; i < CPM_SECTOR; i++)
    {
      if (write)
        {
          ramdisk [off + i] = dma [i];
        }
      else
        {
          dma [i] = ramdisk [off + i];
        }
    }

  return 0;
}

/*****************************************************************************/

/*
 * B:/C:/D: - one 128-byte CP/M sector inside a 512-byte physical one.
 * A write is a read-modify-write; the one-sector cache means the read is
 * usually free, since BDOS works through the four sub-sectors in order.
 */

static unsigned short int
bios_rw (int write)
{
  const struct dpb *p = (cur_drive == DRV_FLOPPY) ? &dpb_floppy : &dpb_hd;
  unsigned long lin, lba, off;
  unsigned char *dma = (unsigned char *)cur_dma;
  int i;

  if (cur_drive >= DRV_COUNT || !drives [cur_drive].probed)
    {
      return 1;
    }

  if (!dma)
    {
      return 1;
    }

  lin = (unsigned long)cur_trk * p->spt + cur_sec;
  lba = lin / CPM_PER_PHYS;
  off = (lin % CPM_PER_PHYS) * CPM_SECTOR;

  if (phys_read (cur_drive, lba) != 0)
    {
      return 1;
    }

  if (!write)
    {
      for (i = 0; i < CPM_SECTOR; i++)
        {
          dma [i] = LOW8 (V86_BUF_ADDR + off + i);
        }

      return 0;
    }

  for (i = 0; i < CPM_SECTOR; i++)
    {
      LOW8 (V86_BUF_ADDR + off + i) = dma [i];
    }

  return phys_write (cur_drive, lba) ? 1 : 0;
}

/*****************************************************************************/

unsigned short int
bios_read (void)
{
  if (cur_drive == DRV_RAM)
    {
      return ram_rw (0);
    }

  return bios_rw (0);
}

/*****************************************************************************/

unsigned short int
bios_write (unsigned short int typecode)
{
  (void)typecode;

  if (cur_drive == DRV_RAM)
    {
      return ram_rw (1);
    }

  return bios_rw (1);
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
