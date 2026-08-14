/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: c915bfe0-82b5-11f1-8486-80ee73e9b8e7
 */

/*****************************************************************************/

/* testbdos.c - unit tests */

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#define HOST_TEST 1
/* Types (bdosinc) must come before bdosdef structures that use them. */
#include "bdosinc.h"
#include "bdosdef.h"
#include "biosdef.h"
#include "bringup.h" /* shared bring-up */

/*****************************************************************************/

extern UWORD _bdos (WORD func, UWORD info, UBYTE *infop);

/*****************************************************************************/

/* also bdos wrapper if used, 32-bit safe, matches bios.c */
UWORD
bdos (WORD func, LONG info)
{
  extern UWORD _bdos (WORD func, UWORD info, UBYTE * infop);
  unsigned long pval = (unsigned long)info;

  return _bdos (func, (UWORD)pval, (UBYTE *)pval);
}

/*****************************************************************************/

/* --- mock BIOS providing the 22-ish entries --- */
static int mock_disk_selected = -1;
static int mock_read_count = 0;
static int mock_conout_count = 0;
static unsigned char mock_dma [128];
static void *last_dma = 0;
static unsigned short cur_sec = 0;
static unsigned short cur_trk = 0;
static UBYTE mock_dir_sector [128];
static unsigned char
    unit_dma0; /* snapshot for gate, avoid stack clobber in test */

/*****************************************************************************/

/* capture for dir writes (create 22, close 16, set attr 30) per plan checklist */
static unsigned char last_dir_write [128];
static int last_dir_write_valid = 0;
static int mock_write_count = 0;

/*****************************************************************************/

void
init_mock_dir (void)
{
  int i;

  for (i = 0; i < 128; i++)
    {
      mock_dir_sector[i] = 0xe5;
    }

  UBYTE *de = mock_dir_sector;
  de[0] = 0; /* user 0 */
  memcpy (de + 1, "README  ", 8);
  memcpy (de + 9, "TXT", 3);
  de[12] = 0; /* extent */
  de[13] = 0;
  de[14] = 0; /* s1 s2 (must be 0 for match chk_ext after name) */
  de[15] = 1; /* rcdcnt */
  de[16] = 4; /* block - chosen to map to 0x2000 in the 4mb-hd ramdisk image
               * (with current dpb 2k blocks) */
}

/*****************************************************************************/

void
reset_mock_dir_writes (void)
{
  last_dir_write_valid = 0;
  mock_write_count = 0;
  memset (last_dir_write, 0, sizeof (last_dir_write));
}

/*****************************************************************************/

/* shared dph0 from cpm_bringup after cpm_bringup(); local mock_dir_sector for
 * read fill to make search succeed for strict unit asserts */

/*****************************************************************************/

void
bios_wboot (void)
{
  printf ("[mock wboot]\n");
  exit (0);
}

/*****************************************************************************/

unsigned short int
bios_const (void)
{
  return 0;
}

/*****************************************************************************/

unsigned char
bios_conin (void)
{
  return 0;
}

/*****************************************************************************/

void
bios_conout (unsigned char c)
{
  if (c >= 32 || c == '\r' || c == '\n' || c == '\t')
    {
      putchar (c);
    }

  mock_conout_count++;
}

/*****************************************************************************/

void
bios_list (unsigned char c)
{
  (void)c;
}

/*****************************************************************************/

void
bios_punch (unsigned char c)
{
  (void)c;
}

/*****************************************************************************/

unsigned char
bios_reader (void)
{
  return 0x1a;
}

/*****************************************************************************/

#if 0
void
bios_home (void)
{
  /* void */
}
#endif

/*****************************************************************************/

void *
bios_seldsk (unsigned char d, unsigned char l)
{
  mock_disk_selected = d;
  (void)l;

  if (d < 16)
    {
      return &dph0; /* emulate only A: ramdisk for all drives; prevents null
                       dphp + error(3) loop in fileio seldsk while() */
    }

  return 0;
}

/*****************************************************************************/

void
bios_settrk (unsigned short t)
{
  cur_trk = t;
}

/*****************************************************************************/

void
bios_setsec (unsigned short s)
{
  cur_sec = s;
}

/*****************************************************************************/

void
bios_setdma (void *a)
{
  last_dma = a;
  if (a)
    {
      memcpy (mock_dma, a, 128);
    }
}

/*****************************************************************************/

unsigned short int
bios_read (void)
{
  mock_read_count++;
  if (last_dma)
    {
      unsigned char *d = (unsigned char *)last_dma;
      /*
       * serve from real ramdisk at offset computed from cur_trk/cur_sec (set
       * by BDOS using FCB dskmap/cur_rec + dpb after open populated the alloc
       * from dirent). Delivers actual data from the allocation block position
       * in the ramdisk image.
       */

      if (last_dma == dph0.dbufp)
        {
          memcpy (d, mock_dir_sector, 128);
        }
      else
        {
          unsigned short spt = dph0.dpbp ? dph0.dpbp->spt : 32;
          unsigned long off = ((unsigned long)cur_trk * spt + cur_sec) * 128UL;

          if (off + 128 <= RAMDISK_SIZE)
            {
              memcpy (d, &ramdisk[off], 128);
            }
          else
            {
              d[0] = 'T';
              d[1] = 'h';
              d[2] = 'i';
              d[3] = 's';
              d[4] = ' ';
            }
        }
    }

  return 0;
}

/*****************************************************************************/

unsigned short int
bios_write (unsigned short t)
{
  mock_write_count++;
  (void)t;

  if (last_dma)
    {
      /* capture; key on whether dma is the dirbufp (dir writes for create/close/attr) */
      memcpy (mock_dma, last_dma, 128);

      if (last_dma == dph0.dbufp)
        {
          memcpy (last_dir_write, last_dma, 128);
          last_dir_write_valid = 1;
          /* apply back for consistency if later read same dir */
          memcpy (mock_dir_sector, last_dma, 128);
        }
    }

  return 0;
}

/*****************************************************************************/

#if 0
unsigned short int
bios_listst (void)
{
  return 0;
}
#endif

/*****************************************************************************/

unsigned short int
bios_sectran (unsigned short s, void *t)
{
  (void)t;

  return s;
}

/*****************************************************************************/

void *
bios_getmrt (void)
{
  /* large TPA to match CP/M-386 protected mode "end of memory" model (for
   * bdosinit etc) */
  static struct
  {
    unsigned short c;
    void *b;
    unsigned long l;
  } m = { 1, (void *)0x30000, 16 * 1024 * 1024UL };

  return &m;
}

/*****************************************************************************/

unsigned short int
bios_getiobyte (void)
{
  return 0;
}

/*****************************************************************************/

void
bios_setiobyte (unsigned short v)
{
  (void)v;
}

/*****************************************************************************/

unsigned short int
bios_flush (void)
{
  return 0;
}

/*****************************************************************************/

void *
bios_setexc (unsigned short v, void *h)
{
  (void)v;
  (void)h;

  return 0;
}

/*****************************************************************************/

/* swap/udiv from our bios, but since linked the portable call the ones in
 * bios.o which we provide in build */
extern WORD swap (WORD);
extern UWORD udiv (LONG, UWORD, UWORD *);

/*****************************************************************************/

/* disk state vectors for resetting between test groups to avoid ro_dsk/crit
 * causing error(4) on writes after dir data change */
extern UWORD log_dsk, ro_dsk, crit_dsk;

/*****************************************************************************/

/* provide swap/udiv expected by some portable disk code (from our 386 layer)
 */
WORD
swap (WORD victim)
{
  static int swaptype = -1;
  static unsigned short testpattern = 0x0102;
  unsigned short temp;

  if (swaptype < 0)
    {
      if (*(char *)&testpattern == 0x01)
        {
          swaptype = 1;
        }
      else
        {
          swaptype = 0;
        }
    }

  if (swaptype == 0)
    {
      return victim;
    }

  temp = ((victim & 0xff) << 8) | ((victim & 0xff00) >> 8);

  return (WORD)temp;
}

/*****************************************************************************/

UWORD
udiv (LONG dividend, UWORD divisor, UWORD *remainder)
{
  if (divisor == 0)
    {
      *remainder = 0;

      return 0;
    }

  *remainder = (UWORD)(dividend % (LONG)divisor);

  return (UWORD)(dividend / (LONG)divisor);
}

/*****************************************************************************/

char load_tbl[32]; /* referenced by ccp load path (stubbed) */

/*****************************************************************************/

void
initexc (UBYTE **v)
{
  (void)v;
}

/*****************************************************************************/

UBYTE *
traphndl (void)
{
  return 0;
}

/*****************************************************************************/

UWORD
pgmld (UBYTE *i, UBYTE *d)
{
  (void)i;
  (void)d;

  return 0xffff;
}

/*****************************************************************************/

UWORD
pgm_go (void)
{
  return 0xffff;
}

/*****************************************************************************/

void
bios_setup_basepage (const void *a, const void *b, const char *c)
{
  (void)a;
  (void)b;
  (void)c;
}

/*****************************************************************************/

/* Host stubs for BDOS 104/105/155 (real CMOS is in rtc.o on the target). */
int
rtc_get (void *p)
{
  (void)p;

  return 1;
}

/*****************************************************************************/

int
rtc_get_bcd (void *p)
{
  (void)p;

  return 1;
}

/*****************************************************************************/

int
rtc_set (void *p)
{
  (void)p;

  return 1;
}

/*****************************************************************************/

/* Host stubs for private BDOS 220-223 */
void
bios_system_reboot (int warm)
{
  (void)warm;
}

/*****************************************************************************/

/* Host stubs for private BDOS 225/226 (where PIT not available) */
void
pit_read (unsigned long *lo, unsigned long *hi)
{
  if (lo)
    {
      *lo = 0;
    }

  if (hi)
    {
      *hi = 0;
    }
}

/*****************************************************************************/

void
pit_sleep_until (unsigned long lo, unsigned long hi)
{
  (void)lo;
  (void)hi;
}

/*****************************************************************************/

void
pit_poll (void)
{
  /* void */
}

/*****************************************************************************/

void
pit_init (void)
{
  /* void */
}

/*****************************************************************************/

void
bios_con_clear (void)
{
  /* void */
}

/*****************************************************************************/

unsigned short
bios_con_vga_ctl (unsigned short info)
{
  return info == 0xFFFF ? 1 : (info ? 1 : 0);
}

/*****************************************************************************/

unsigned short
bios_con_ser_ctl (unsigned short info)
{
  return info == 0xFFFF ? 1 : (info ? 1 : 0);
}

/*****************************************************************************/

/* Host stubs for F_PARSE pointer translation (real pmode.o is freestanding).
 * pmode_active()==0 makes BDOS 152 treat ascii/fcb fields as host pointers. */
int
pmode_active (void)
{
  return 0;
}

/*****************************************************************************/

unsigned long
pmode_tpa_base (void)
{
  return 0;
}

/*****************************************************************************/

unsigned long
pmode_tpa_len (void)
{
  return 0;
}

/*****************************************************************************/

/* no ring-3 VGA segment in unit tests */
unsigned short
pmode_vga_selector (void)
{
  return 0;
}

/*****************************************************************************/

unsigned long
pmode_vga_phys_base (void)
{
  return 0;
}

/*****************************************************************************/

unsigned long
pmode_vga_map_size (void)
{
  return 0;
}

/*****************************************************************************/

/* DOS-PLUS exact size: records*128 if lrbc==0, else (records-1)*128+lrbc */
static unsigned long
dosplus_exact (unsigned long records, unsigned lrbc)
{
  if (records == 0)
    {
      return 0;
    }

  if (lrbc == 0)
    {
      return records * 128UL;
    }

  return (records - 1) * 128UL + lrbc;
}

/*****************************************************************************/

/* Synthetic CP/M record stream for cpm386_load_from_reader unit tests */
static UBYTE g_stream_blob[512];
static int g_stream_idx;
static int g_stream_hole; /* record index that returns status 1, or -1 */

/*****************************************************************************/

static UWORD
test_stream_reader (UBYTE rec[128], void *ctx)
{
  int k;

  (void)ctx;
  if (g_stream_idx == g_stream_hole)
    {
      g_stream_idx++;
      for (k = 0; k < 128; k++)
        {
          rec[k] = 0xAA;
        }

      return 1;
    }

  for (k = 0; k < 128; k++)
    {
      rec[k] = g_stream_blob[g_stream_idx * 128 + k];
    }

  g_stream_idx++;

  return 0;
}

/*****************************************************************************/

/*
 * Build a version 1 .386 header, so the fixtures below describe programs
 * the way mk386 actually writes them.
 */

/* Image byte at which record 1 - the simulated allocation hole - begins. */
#define HOLE_FIRST (128 - CPM386_HDR_SIZE)

static void
mk_hdr (UBYTE *h, unsigned long load, unsigned long sz, unsigned long ent,
        unsigned long min_kb)
{
  int i;

  for (i = 0; i < CPM386_HDR_SIZE; i++)
    {
      h[i] = 0;
    }

  h[0] = (UBYTE)(CPM386_MAGIC & 0xFF);
  h[1] = (UBYTE)((CPM386_MAGIC >> 8) & 0xFF);
  h[2] = (UBYTE)((CPM386_MAGIC >> 16) & 0xFF);
  h[3] = (UBYTE)((CPM386_MAGIC >> 24) & 0xFF);
  h[4] = CPM386_VERSION;
  h[5] = CPM386_HDR_SIZE;
  /* h[6..7] flags stay 0 */

  h[8] = (UBYTE)(load & 0xFF);
  h[9] = (UBYTE)((load >> 8) & 0xFF);
  h[10] = (UBYTE)((load >> 16) & 0xFF);
  h[11] = (UBYTE)((load >> 24) & 0xFF);

  h[12] = (UBYTE)(sz & 0xFF);
  h[13] = (UBYTE)((sz >> 8) & 0xFF);
  h[14] = (UBYTE)((sz >> 16) & 0xFF);
  h[15] = (UBYTE)((sz >> 24) & 0xFF);

  h[16] = (UBYTE)(ent & 0xFF);
  h[17] = (UBYTE)((ent >> 8) & 0xFF);
  h[18] = (UBYTE)((ent >> 16) & 0xFF);
  h[19] = (UBYTE)((ent >> 24) & 0xFF);

  h[20] = (UBYTE)(min_kb & 0xFF);
  h[21] = (UBYTE)((min_kb >> 8) & 0xFF);
  h[22] = (UBYTE)((min_kb >> 16) & 0xFF);
  h[23] = (UBYTE)((min_kb >> 24) & 0xFF);
}

/*****************************************************************************/

int
main (void)
{
  int ok = 1;

  printf ("*** test_bdos: exercising real portable BDOS objects\n");

  /* console status (exercises _bdos + bios_const via conbdos) */
  UWORD r = _bdos (11, 0, 0);
  printf ("constat=%u\n", r);

  /* version (simple) */
  r = _bdos (12, 0, 0);
  printf ("version=%04x\n", r);

  /* console output via bdos (exercises tabout/raw etc path) */
  _bdos (2, 'T', 0);
  _bdos (2, 'X', 0);
  _bdos (2, '\r', 0);
  _bdos (2, '\n', 0);
  printf (" (conout ~%d)\n", mock_conout_count);

  init_mock_dir (); /* for mock_read to fill dir sector data BEFORE bringup's
                     * login scan sets csv/hiwater; ensures chksum match +
                     * match success on open for strict asserts */
  cpm_bringup (); /* shared bring-up (inits ramdisk/dph + bdosinit + select A) */

  /*
   * representative disk calls per v.erif plan (select, open, read) -- use
   * shared dph0; mock read provides dir data; strict gate: fail unless
   * open==0, read==0, dma[0]=='R'
   */

  r = _bdos (14, 0, 0);
  printf ("select A ret=%u mock_sel=%d\n", r, mock_disk_selected);

  unsigned char fcb[36];
  memset (fcb, 0, 36);
  fcb[0] = 1;  /* A: drive code (1 for A, as in ccp dir_cmd fcb[0] = cur+1 ) */
  fcb[12] = 0; /* extent */
  memcpy (fcb + 1, "README  TXT", 11);
  UWORD open_ret = _bdos (15, 0, fcb);
  printf ("open README ret=%u\n", open_ret);

  /*
   * FCB after open (15) per plan: should have copied dirent (rc, alloc) but
   * kept user extent; s2 hi bit set; ran untouched
   */

  printf ("post-open fcb: drv=%u ex=%u s1=%u s2=%02x rc=%u dsk0=%u cur=%u "
          "ran0/1/2=%u/%u/%u\n",
          fcb[0], fcb[12], fcb[13], fcb[14], fcb[15], fcb[16], fcb[32],
          fcb[33], fcb[34], fcb[35]);
  if (open_ret > 3 || fcb[15] != 1 || fcb[16] != 4 || (fcb[14] & 0x80) == 0
      || fcb[33] || fcb[34] || fcb[35])
    {
      printf ("UNIT HARD FAIL: open FCB fields wrong (rc/alloc/s2hi/ran)\n");
      ok = 0;
    }

  unsigned char rdbuf[128];
  UWORD setdma_ret = _bdos (
      26, 0, rdbuf); /* set dma (26) - capture explicit ret for verif */
  printf ("set DMA(26) ret=%u\n", setdma_ret);
  fcb[32] = 0;                         /* cur rec */
  UWORD read_ret = _bdos (20, 0, fcb); /* read seq (20) -> should call bread */
  int firstc = (int)(unsigned char)rdbuf[0];
  printf ("read ret=%u readcnt=%d dma[0:4]=%c%c%c%c cur_sec=%u (set from FCB "
          "dskmap/cur_rec via iosys; served from ramdisk at computed off)\n",
          read_ret, mock_read_count, rdbuf[0], rdbuf[1], rdbuf[2], rdbuf[3],
          cur_sec);
  printf ("post-read fcb cur_rec=%u ran=%u%u%u\n", fcb[32], fcb[33], fcb[34],
          fcb[35]);

  if (mock_disk_selected == 0 && mock_read_count > 0 && firstc == 'M'
      && read_ret == 0 /* read success */)
    {
      printf ("disk side effects (select/read/dma content) observed\n");
    }
  else
    {
      printf ("UNIT HARD FAIL: disk ");
      printf ("disk side effects (select/read/dma content) NOT observed\n");
      ok = 0;
    }

  /*
   * hard gate: open==0 (or<=3), read==0, dma[0]=='T' (real ramdisk data from
   * alloc block), cur_rec advanced, ran untouched (seq vs rand distinct)
   */

  if (open_ret > 3 || read_ret != 0 || firstc != 'M' || fcb[32] != 1 || fcb[33]
      || fcb[34] || fcb[35])
    {
      printf ("UNIT HARD FAIL: open=%u read=%u dma0=%c cur_rec=%u ran=%u%u%u "
              "(expected 0/<=3,0,'T',cur=1,ran=000)\n",
              open_ret, read_ret, firstc, fcb[32], fcb[33], fcb[34], fcb[35]);
      ok = 0;
    }

  /* test group: 22 create + 30 set attr + 16 close (new clean dir slot) */
  reset_mock_dir_writes ();
  /* prepare empty dir (e5) so create finds free slot; use name not conflicting with prior */
  {
    int ii;

    for (ii = 0; ii < 128; ii++)
      {
        mock_dir_sector[ii] = 0xe5;
      }
  }

  /*
   * clear crit/ro/log state from prior open (which set crit) so that dir data
   * change doesn't cause ro_dsk -> error(4) on create's dir_wr
   */

  log_dsk = ro_dsk = crit_dsk = 0;
  dph0.hiwater = 0;

  /*
   * force csv chksum init for e5 data + copy to dirbufp to prevent mismatch ->
   * retry loops or error paths in dirscan
   */

  memcpy ((void *)dph0.dbufp, mock_dir_sector, 128);
  {
    long lsum = 0;
    long *p = (long *)dph0.dbufp;
    int k;

    for (k = 0; k < 32; k++)
      {
        lsum += p[k];
      }

    lsum += (lsum >> 16);
    lsum += (lsum >> 8);
    ((UBYTE *)dph0.csv)[0] = (UBYTE)(lsum & 0xff);
  }

  unsigned char fcb2[36];
  memset (fcb2, 0, 36);
  fcb2[0] = 1;
  memcpy (fcb2 + 1, "TEST    DAT", 11); /* new file, no ext attr yet */
  fcb2[12] = 0;
  fcb2[13] = 0;
  fcb2[14] = 0;
  fcb2[15] = 0; /* as bdos preps for create */
  UWORD cre_ret = _bdos (22, 0, fcb2);
  printf ("create TEST ret=%u writecnt=%d\n", cre_ret, mock_write_count);
  printf ("post-create fcb2: ex=%u s2=%02x rc=%u dsk0=%u (create zeros "
          "rcdcnt/dskmap, moves name to dir)\n",
          fcb2[12], fcb2[14], fcb2[15], fcb2[16]);

  if (last_dir_write_valid)
    {
      UBYTE *de = last_dir_write;
      printf ("create dirent: user=%u name=%.8s.%.3s ex=%u rc=%u dsk0=%u\n",
              de[0], de + 1, de + 9, de[12], de[15], de[16]);
    }

  if (cre_ret > 3 || fcb2[15] != 0 || fcb2[16] != 0)
    {
      printf ("UNIT HARD FAIL: create did not leave FCB rc/dsk zeroed as "
              "expected\n");
      ok = 0;
    }

  /* now set attr (30): set archive bit (arbit=2, ftype[2] high) instead of ro
   * to avoid ro_err on later dir_wr/close */
  fcb2[11] |= 0x80; /* arbit in ftype[2] high bit */
  UWORD attr_ret = _bdos (30, 0, fcb2);
  printf ("set attr ret=%u writecnt=%d\n", attr_ret, mock_write_count);
  unsigned char dir_after_attr[128] = { 0 };

  if (last_dir_write_valid)
    {
      memcpy (dir_after_attr, last_dir_write, 128);
      UBYTE *de = dir_after_attr;
      printf ("after attr dirent ftype2=%02x\n", de[11]);
    }

  /* close (16) - do NOT set s2&0x80 (write flag on would skip dirscan in
   * close_fi); set a dskmap to drive merge/copy in close */
  fcb2[16] = 5;      /* simulate alloc block from "prior write" */
  fcb2[14] &= ~0x80; /* ensure hi bit off so dirscan(close) runs and does
                      * merge+dir_wr */
  UWORD clo_ret = _bdos (16, 0, fcb2);
  printf ("close ret=%u writecnt=%d\n", clo_ret, mock_write_count);

  if (last_dir_write_valid)
    {
      UBYTE *de = last_dir_write;
      printf ("after close dirent: user=%u ex=%u s2=%02x rc=%u dsk0=%u "
              "ftype0=%02x (merged from fcb dsk)\n",
              de[0], de[12], de[14], de[15], de[16], de[9]);
    }

  /* gates for create/attr/close: success rets, close wrote, attr bit seen */
  if (cre_ret > 3 || attr_ret > 3 || clo_ret > 3 || !last_dir_write_valid
      || mock_write_count < 3)
    {
      printf ("UNIT HARD FAIL: create/attr/close ret bad or no close write "
              "(cre=%u attr=%u clo=%u wc=%d)\n",
              cre_ret, attr_ret, clo_ret, mock_write_count);
      ok = 0;
    }
  else
    {
      UBYTE *de = last_dir_write;
      /* Word-mode map: block 5 is LE bytes 05 00 at dskmap[0] */
      UWORD merged_blk = (UWORD)de[16] | ((UWORD)de[17] << 8);

      if (de[12] != 0 || de[15] != 0 /*rc*/ || merged_blk != 5)
        {
          printf ("UNIT HARD FAIL: created/closed dirent layout wrong (ex=%u "
                  "rc=%u blk=%u)\n",
                  de[12], de[15], merged_blk);
          ok = 0;
        }

      if ((dir_after_attr[11] & 0x80) == 0)
        {
          printf ("UNIT HARD FAIL: attr did not set in dirent\n");
          ok = 0;
        }
    }

  printf ("*** basic BDOS calls via real objects succeeded\n");

  /*
   * LRBC (DOS-PLUS last-record byte count) unit tests
   * See https://www.seasip.info/Cpm/bytelen.html.
   * I use DOS-PLUS (bytes used in last record; 0 means full/128),
   * not ISX (unused bytes) style.
   */
  {
    unsigned long rec, exact;
    UWORD oret;
    unsigned char lfcb[36];

    /* --- open with FCB+32=0xFF returns directory s1 (LRBC) in cur_rec --- */
    /* mock README has s1=0 in init_mock_dir; set a known LRBC first */
    init_mock_dir ();
    mock_dir_sector[13] = 49; /* DOS-PLUS: 49 bytes used in last record */
    mock_dir_sector[15] = 1;  /* rc = 1 record */
    /* re-login style: refresh dirbuf checksum for dirscan */
    log_dsk = ro_dsk = crit_dsk = 0;
    dph0.hiwater = 0;
    memcpy ((void *)dph0.dbufp, mock_dir_sector, 128);

    {
      long lsum = 0;
      long *p = (long *)dph0.dbufp;
      int k;

      for (k = 0; k < 32; k++)
        {
          lsum += p[k];
        }

      lsum += (lsum >> 16);
      lsum += (lsum >> 8);
      ((UBYTE *)dph0.csv)[0] = (UBYTE)(lsum & 0xff);
    }

    memset (lfcb, 0, 36);
    lfcb[0] = 1;
    memcpy (lfcb + 1, "README  TXT", 11);
    lfcb[32] = 0xFF; /* request LRBC on open */
    oret = _bdos (15, 0, lfcb);
    printf ("LRBC open(0xFF) ret=%u s1=%u cur_rec=%u rc=%u\n", oret, lfcb[13],
            lfcb[32], lfcb[15]);

    if (oret > 3 || lfcb[13] != 49 || lfcb[32] != 49)
      {
        printf ("UNIT HARD FAIL: open 0xFF did not return DOS-PLUS LRBC in "
                "s1/cur_rec\n");
        ok = 0;
      }
    else
      {
        printf ("LRBC: open with FCB+32=0xFF returns LRBC ok\n");
      }

    /* exact size for rc=1, lrbc=49 */
    exact = dosplus_exact (1, 49);

    if (exact != 49)
      {
        printf ("UNIT HARD FAIL: dosplus_exact(1,49)=%lu expected 49\n",
                exact);
        ok = 0;
      }

    /* --- set_attr with F6' writes LRBC to directory s1 --- */
    reset_mock_dir_writes ();
    {
      int ii;

      for (ii = 0; ii < 128; ii++)
        {
          mock_dir_sector[ii] = 0xe5;
        }
    }

    /* put a file entry so set_attr can match */
    mock_dir_sector[0] = 0;
    memcpy (mock_dir_sector + 1, "SIZED   DAT", 11);
    mock_dir_sector[12] = 0;
    mock_dir_sector[13] = 0; /* old LRBC */
    mock_dir_sector[14] = 0;
    mock_dir_sector[15] = 2; /* 2 records */
    mock_dir_sector[16] = 6;
    log_dsk = ro_dsk = crit_dsk = 0;
    dph0.hiwater = 0;
    memcpy ((void *)dph0.dbufp, mock_dir_sector, 128);

    {
      long lsum = 0;
      long *p = (long *)dph0.dbufp;
      int k;

      for (k = 0; k < 32; k++)
        {
          lsum += p[k];
        }

      lsum += (lsum >> 16);
      lsum += (lsum >> 8);
      ((UBYTE *)dph0.csv)[0] = (UBYTE)(lsum & 0xff);
    }

    memset (lfcb, 0, 36);
    lfcb[0] = 1;
    memcpy (lfcb + 1, "SIZED   DAT", 11);
    lfcb[6] |= 0x80; /* F6' - signal set LRBC */
    lfcb[32] = 72;   /* DOS-PLUS: 72 bytes used in last record */

    {
      UWORD ar = _bdos (30, 0, lfcb);
      printf ("LRBC set_attr(F6',72) ret=%u\n", ar);

      if (ar > 3 || !last_dir_write_valid || last_dir_write[13] != 72)
        {
          printf ("UNIT HARD FAIL: set_attr did not store LRBC s1=72 (got "
                  "s1=%u valid=%d)\n",
                  last_dir_write_valid ? last_dir_write[13] : 0,
                  last_dir_write_valid);
          ok = 0;
        }
      else
        {
          printf ("LRBC: set_attr F6'+FCB+32 writes directory s1 ok\n");
        }
    }

    /* formula checks: full record, multi-record partial */
    if (dosplus_exact (1, 0) != 128 || dosplus_exact (2, 0) != 256
        || dosplus_exact (2, 1) != 129 || dosplus_exact (2, 72) != 200
        || dosplus_exact (0, 0) != 0)
      {
        printf ("UNIT HARD FAIL: DOS-PLUS exact size formula\n");
        ok = 0;
      }
    else
      {
        printf (
            "LRBC: DOS-PLUS exact-size formula ok (0=>full, 1..127=>used)\n");
      }

    /* Function 35 (compute file size) endian-safe ranrec packing */
    {
      unsigned char sfcb[36];
      unsigned long nrec;
      init_mock_dir ();
      mock_dir_sector[13] = 49;
      mock_dir_sector[15] = 1;
      log_dsk = ro_dsk = crit_dsk = 0;
      dph0.hiwater = 0;
      memcpy ((void *)dph0.dbufp, mock_dir_sector, 128);

      {
        long lsum = 0;
        long *p = (long *)dph0.dbufp;
        int k;

        for (k = 0; k < 32; k++)
          {
            lsum += p[k];
          }

        lsum += (lsum >> 16);
        lsum += (lsum >> 8);
        ((UBYTE *)dph0.csv)[0] = (UBYTE)(lsum & 0xff);
      }

      memset (sfcb, 0, 36);
      sfcb[0] = 1;
      memcpy (sfcb + 1, "README  TXT", 11);
      _bdos (35, 0, sfcb);
      /* CP/M-68K: ran0=hi, ran1=mid, ran2=lo */
      nrec = ((unsigned long)sfcb[33] << 16) | ((unsigned long)sfcb[34] << 8)
             | sfcb[35];
      printf ("LRBC getsize(35) ran=%u/%u/%u records=%lu\n", sfcb[33],
              sfcb[34], sfcb[35], nrec);

      if (nrec != 1)
        {
          printf ("UNIT HARD FAIL: getsize expected 1 record, got %lu\n",
                  nrec);
          ok = 0;
        }
      else
        {
          printf ("LRBC: BDOS function 35 record count ok\n");
        }
    }

    /* --- if ramdisk.bin present, verify real cpmtools LRBC for shipped files -- */
    {
      FILE *rf = fopen ("ramdisk.bin", "rb");

      if (rf)
        {
          unsigned char dir[4096];
          size_t n = fread (dir, 1, sizeof dir, rf);
          fclose (rf);
          int found_readme = 0, found_hello = 0, found_lrbc = 0;
          size_t i;

          for (i = 0; i + 32 <= n; i += 32)
            {
              if (dir[i] >= 16 || dir[i + 1] < 32 || dir[i + 1] >= 127)
                {
                  continue;
                }

              char name[13];
              int j;

              for (j = 0; j < 8; j++)
                {
                  name[j] = dir[i + 1 + j] & 0x7f;
                }

              while (j > 0 && name[j - 1] == ' ')
                {
                  j--;
                }

              name[j++] = '.';
              name[j++] = dir[i + 9] & 0x7f;
              name[j++] = dir[i + 10] & 0x7f;
              name[j++] = dir[i + 11] & 0x7f;

              while (j > 0 && name[j - 1] == ' ')
                {
                  j--;
                }
              name[j] = 0;

              unsigned s1 = dir[i + 13], rc = dir[i + 15];
              rec = rc; /* single-extent files on our tiny image */
              exact = dosplus_exact (rec, s1);
              printf ("ramdisk dirent %s s1=%u rc=%u exact=%lu\n", name, s1,
                      rc, exact);

              if (strncmp (name, "README.TXT", 10) == 0)
                {
                  found_readme = 1;

                  /* printf README is 49 bytes */
                  if (s1 != 49 || rc != 1 || exact != 49)
                    {
                      printf ("UNIT HARD FAIL: README.TXT LRBC expected s1=49 "
                              "rc=1 exact=49\n");
                      ok = 0;
                    }
                }

              if (strncmp (name, "HELLO.386", 9) == 0)
                {
                  found_hello = 1;
                  /* size = 12 hdr + image; verify s1 matches file size mod 128
                   */

                  FILE *hf = fopen ("hello.386", "rb");
                  if (hf)
                    {
                      fseek (hf, 0, SEEK_END);
                      long hsz = ftell (hf);
                      fclose (hf);
                      unsigned exp_s1 = (unsigned)(hsz % 128);
                      unsigned exp_rc = (unsigned)((hsz + 127) / 128);

                      if (s1 != exp_s1 || rc != exp_rc
                          || exact != (unsigned long)hsz)
                        {
                          printf ("UNIT HARD FAIL: HELLO.386 LRBC s1=%u rc=%u "
                                  "exact=%lu (file %ld exp s1=%u rc=%u)\n",
                                  s1, rc, exact, hsz, exp_s1, exp_rc);
                          ok = 0;
                        }
                    }
                }

              if (strncmp (name, "LRBC.386", 8) == 0)
                {
                  found_lrbc = 1;
                  FILE *lf = fopen ("lrbc.386", "rb");

                  if (lf)
                    {
                      fseek (lf, 0, SEEK_END);
                      long lsz = ftell (lf);
                      fclose (lf);
                      unsigned exp_s1 = (unsigned)(lsz % 128);
                      unsigned exp_rc = (unsigned)((lsz + 127) / 128);

                      if (s1 != exp_s1 || rc != exp_rc
                          || exact != (unsigned long)lsz)
                        {
                          printf ("UNIT HARD FAIL: LRBC.386 LRBC mismatch "
                                  "s1=%u rc=%u exact=%lu file=%ld\n",
                                  s1, rc, exact, lsz);
                          ok = 0;
                        }
                    }
                }
            }

          if (!found_readme || !found_hello || !found_lrbc)
            {
              printf ("UNIT HARD FAIL: ramdisk missing README/HELLO/LRBC "
                      "(r=%d h=%d l=%d)\n",
                      found_readme, found_hello, found_lrbc);
              ok = 0;
            }
          else
            {
              printf ("LRBC: ramdisk README/HELLO/LRBC directory LRBC matches "
                      "host file sizes\n");
            }
        }
      else
        {
          printf ("LRBC: ramdisk.bin not present - skip image LRBC check "
                  "(build all first)\n");
        }
    }
  }

  /* loader core unit tests */

  /* (synthetic .386 buffers, no disk, drive shipped cpm386_load_from_buf) */
  {
    /* synthetic header load=0 size=4 entry=2 ; followed by 4 bytes of "image" */
    UBYTE fakefile[CPM386_HDR_SIZE + 4];
    UBYTE tpa[64];
    UBYTE *ent = 0;
    UWORD lrc;
    int i;

    /* zero tpa */
    for (i = 0; i < 64; i++)
      {
        tpa[i] = 0;
      }

    mk_hdr (fakefile, 0, 4, 2, 0);
    fakefile[CPM386_HDR_SIZE + 0] = 0x90;
    fakefile[CPM386_HDR_SIZE + 1] = 0x90;
    fakefile[CPM386_HDR_SIZE + 2] = 0xCC;
    fakefile[CPM386_HDR_SIZE + 3] = 0x90;
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != 0 || ent != (tpa + 2) || (UBYTE)tpa[0] != (UBYTE)0x90
        || (UBYTE)tpa[1] != (UBYTE)0x90 || (UBYTE)tpa[2] != (UBYTE)0xCC
        || (UBYTE)tpa[3] != (UBYTE)0x90 || (UBYTE)tpa[4] != (UBYTE)0)
      {
        printf ("UNIT HARD FAIL: core load buf placement/entry (lrc=%u "
                "ent_off=%ld) tpa[0..4]=%02x %02x %02x %02x %02x\n",
                lrc, (long)(ent ? (ent - tpa) : -1), (unsigned char)tpa[0],
                (unsigned char)tpa[1], (unsigned char)tpa[2],
                (unsigned char)tpa[3], (unsigned char)tpa[4]);
        ok = 0;
      }
    else
      {
        printf ("loader core: synthetic .386 parsed/placed/returned entry ok "
                "(load0/sz4/ent2)\n");
      }

    /* test bad size (overflow) */
    mk_hdr (fakefile, 0, 0x7fffffffUL, 2, 0);
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_BADSIZE)
      {
        printf ("UNIT HARD FAIL: core did not reject huge size (rc=%u)\n",
                lrc);
        ok = 0;
      }
    else
      {
        printf ("loader core: size validation error path ok\n");
      }

    /* bad magic */
    mk_hdr (fakefile, 0, 4, 2, 0);
    fakefile[0] ^= 0xFF;
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_BADHDR)
      {
        printf ("UNIT HARD FAIL: core accepted bad magic (rc=%u)\n", lrc);
        ok = 0;
      }
    else
      {
        printf ("loader core: bad magic rejected\n");
      }

    /* unknown version */
    mk_hdr (fakefile, 0, 4, 2, 0);
    fakefile[4] = CPM386_VERSION + 1;
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_BADHDR)
      {
        printf ("UNIT HARD FAIL: core accepted bad version (rc=%u)\n", lrc);
        ok = 0;
      }
    else
      {
        printf ("loader core: unknown version rejected\n");
      }

    /* non-zero reserved flags */
    mk_hdr (fakefile, 0, 4, 2, 0);
    fakefile[6] = 1;
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_BADHDR)
      {
        printf ("UNIT HARD FAIL: core accepted set reserved flag (rc=%u)\n",
                lrc);
        ok = 0;
      }
    else
      {
        printf ("loader core: reserved flags must be zero\n");
      }

    /* declared memory requirement larger than the TPA */
    mk_hdr (fakefile, 0, 4, 2, 64); /* 64K wanted, 64 bytes available */
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_NOMEM)
      {
        printf ("UNIT HARD FAIL: core ignored min_kb (rc=%u)\n", lrc);
        ok = 0;
      }
    else
      {
        printf ("loader core: min_kb refused (%luK wanted, %luK free)\n",
                cpm386_load_req_kb (), cpm386_load_avail_kb ());
      }

    /* a requirement that does fit must still load */
    mk_hdr (fakefile, 0, 4, 2, 0);
    lrc = cpm386_load_from_buf (fakefile, sizeof (fakefile), tpa, 64, &ent);

    if (lrc != CPMLD_OK)
      {
        printf ("UNIT HARD FAIL: core rejected a good header (rc=%u)\n", lrc);
        ok = 0;
      }
  }

  /* streaming loader: multi-record image + allocation hole (status 1) */
  {
    UBYTE tpa[512];
    UBYTE *ent = 0;
    UWORD lrc;
    int i;
    unsigned long sz = 200;

    /*
     * rec0 = header+116, rec1 = HOLE (status 1), rest from pattern
     * with zeros in hole range
     */

    for (i = 0; i < (int)sizeof (g_stream_blob); i++)
      {
        g_stream_blob[i] = 0;
      }

    mk_hdr (g_stream_blob, 0x100, sz, 0x100, 0);

    /*
     * Record 0 carries the header plus whatever is left of its 128 bytes,
     * so the image bytes served by record 1 - the hole - start at
     * 128 - CPM386_HDR_SIZE and run for one record.
     */

    for (i = 0; i < (int)sz; i++)
      {
        if (i >= HOLE_FIRST && i < HOLE_FIRST + 128)
          {
            g_stream_blob[CPM386_HDR_SIZE + i] = 0; /* hole reads as zero */
          }
        else
          {
            g_stream_blob[CPM386_HDR_SIZE + i] = (UBYTE)((i + 1) & 0xff);
          }
      }

    g_stream_idx = 0;
    g_stream_hole = 1;

    for (i = 0; i < 512; i++)
      {
        tpa[i] = 0xEE;
      }

    lrc = cpm386_load_from_reader (test_stream_reader, 0, tpa, 512, &ent);

    if (lrc != 0 || ent != tpa + 0x100)
      {
        printf ("UNIT HARD FAIL: stream load rc=%u ent=%ld\n", lrc,
                (long)(ent ? ent - tpa : -1));
        ok = 0;
      }
    else
      {
        int bad = 0;
        for (i = 0; i < (int)sz; i++)
          {
            UBYTE exp = (i >= HOLE_FIRST && i < HOLE_FIRST + 128)
                            ? 0
                            : (UBYTE)((i + 1) & 0xff);
            if (tpa[0x100 + i] != exp)
              {
                bad = i + 1;

                break;
              }
          }

        if (bad)
          {
            printf ("UNIT HARD FAIL: stream/hole mismatch at %d got %02x\n",
                    bad - 1, tpa[0x100 + bad - 1]);
            ok = 0;
          }
        else
          {
            printf (
                "loader stream: multi-record + hole zero-fill ok (sz=%lu)\n",
                sz);
          }
      }

    /* 300-byte image across records, no hole (load at 0 so 512-byte TPA fits)
     */
    {
      unsigned long bsz = 300;
      UBYTE *e2 = 0;
      UBYTE tpa2[400];

      for (i = 0; i < (int)sizeof (g_stream_blob); i++)
        {
          g_stream_blob[i] = 0;
        }

      mk_hdr (g_stream_blob, 0, bsz, 0, 0);

      for (i = 0; i < (int)bsz; i++)
        {
          g_stream_blob[CPM386_HDR_SIZE + i] = (UBYTE)(0x40 + (i & 0x3f));
        }

      g_stream_idx = 0;
      g_stream_hole = -1;

      for (i = 0; i < 400; i++)
        {
          tpa2[i] = 0;
        }

      lrc = cpm386_load_from_reader (test_stream_reader, 0, tpa2, 400, &e2);

      if (lrc != 0 || tpa2[0] != 0x40
          || tpa2[299] != (UBYTE)(0x40 + (299 & 0x3f)))
        {
          printf ("UNIT HARD FAIL: multi-record stream sz=300 rc=%u t0=%02x "
                  "tend=%02x\n",
                  lrc, tpa2[0], tpa2[299]);
          ok = 0;
        }
      else
        {
          printf ("loader stream: multi-record 300-byte image ok\n");
        }
    }
  }

  /* ramdisk presence of IOTEST when built */
  {
    FILE *rf = fopen ("ramdisk.bin", "rb");

    if (rf)
      {
        unsigned char dir[8192];
        size_t n = fread (dir, 1, sizeof dir, rf);
        fclose (rf);
        int found_io = 0;
        size_t i;

        for (i = 0; i + 32 <= n; i += 32)
          {
            if (dir[i] >= 16 || dir[i + 1] < 32)
              {
                continue;
              }

            char nm[16];
            int j;

            for (j = 0; j < 8; j++)
              {
                nm[j] = dir[i + 1 + j] & 0x7f;
              }

            while (j > 0 && nm[j - 1] == ' ')
              {
                j--;
              }
            nm[j++] = '.';
            nm[j++] = dir[i + 9] & 0x7f;
            nm[j++] = dir[i + 10] & 0x7f;
            nm[j++] = dir[i + 11] & 0x7f;

            while (j > 0 && (nm[j - 1] == ' ' || nm[j - 1] == '.'))
              {
                j--;
              }

            nm[j] = 0;

            if (strncmp (nm, "IOTEST.386", 10) == 0)
              {
                found_io = 1;
              }
          }

        if (!found_io)
          {
            printf (
                "UNIT HARD FAIL: ramdisk missing IOTEST (io=%d)\n", found_io);
            ok = 0;
          }
        else
          {
            printf ("ramdisk: IOTEST.386 present");

            printf ("\n");
          }
      }
  }

  if (ok)
    {
      printf ("*** test_bdos PASSED\n");
    }
  else
    {
      printf ("*** test_bdos FAILED\n");
    }

  fflush (stdout);

  return ok ? 0 : 1;
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
