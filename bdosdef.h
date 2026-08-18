/*
 * CP/M-386 - bdosdef.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 21f4f8da-82b4-11f1-96af-80ee73e9b8e7
 */

/*****************************************************************************/

/*********************************************************
 *                                                       *
 *               CP/M-386 header file                    *
 *               Derived from CP/M-68K                   *
 *                                                       *
 *    Structure definitions for BDOS globals             *
 *       and BDOS data structures                        *
 *                                                       *
 *       Desecrated 6-Aug-83 (sw) for type-ahead         *
 *       Again     17-Mar-84 (sw) for chaining           *
 *                                                       *
 *********************************************************/

/*****************************************************************************/

#ifndef BDOSDEF_H
# define BDOSDEF_H

/*****************************************************************************/

/**************************************************************************
 * The BDOS data structures, especially those relating to global variables,
 * are structured in a way that hopefully will enable this BDOS, in the future,
 * to easily become a re-entrant multi-tasking file system.  Consequently,
 * the BDOS global variables are divided into two classes.  Those that are
 * truly global, even in the case of multiple tasks using the file system
 * concurrently, are simply declared as global variables in bdosmain.c.
 * Only a few "globals" are really global in this sense.
 *
 * The majority of the "global" variables are actually state variables that
 * relate to the state of the task using the file system.  In CP/M-68K, these
 * are "global", since there's only one task, but in a multi-thread model
 * they're not.  This type of variables is put into a data structure, with the
 * intention that in the multi-task environment this structure will be based.
 *
 * The following declarations take this philosophy into account, and define
 * a simple structure for the single thread environment while leaving the
 * possibilities open for the multi-thread environment.
 ****************************************************************************/

/*****************************************************************************/

# define snglthrd TRUE
/* TRUE for single-thread environment */
/* FALSE to create based structure for re-entrant model */
# if snglthrd
#  define GBL gbls
/* In single thread case, GBL just names the structure */
#  define BSETUP EXTERN struct stvars gbls;
/* and BSETUP defines the extern structure */
# endif /* if snglthrd */

/*****************************************************************************/

# if !snglthrd
#  define GBL (*statep)
/* If multi-task, state vars are based */
#  define BSETUP              \
   REG struct stvars *statep; \
   statep = &gbls;
/* set up pointer to state variables */
/* This is intended as an example to show the intent */
# endif /* if !snglthrd */

/*****************************************************************************/

/*
 * Note that there are a few critical regions in the file system that must
 * execute without interruption.  They pertain mostly to the manipulation of
 * the allocation vector.  This isn't a problem in a single-thread model, but
 * must be provided for in a multi-tasking file system.  Consequently, the
 * primitives LOCK and UNLOCK are defined and used where necessary in the
 * file system.  For the single thread model, they are null routines.
 */

# define LOCK   /**/
# define UNLOCK /**/

/*****************************************************************************/

/*
 * Be sure LOCK and UNLOCK are implemented to allow recursive calls to LOCK.
 * That is, if a process that calls LOCK already owns the lock, let it proceed,
 * but remember that only the outer-most call to UNLOCK really releases the
 * file system.
 */

/*****************************************************************************/

# define VERSION 0x2022 /* BDOS version (func 12): type 0x20, BDOS 2.2 */

/*****************************************************************************/

/*
 * S_OSVER (func 163): user-visible product version.  High byte = machine
 * family (same convention as func 12); low byte = BCD major.minor.
 * CP/M-386 0.1 => 0x2001.
 */

# define OSVER_CPM386 0x2001

/*****************************************************************************/

/*
 * CP/M Plus-compatible date/time (see rtc.h).  days since 1978-01-01;
 * h/m/s are binary on CP/M-386 (CP/M Plus uses BCD).
 */

# define BDOS_SET_DATE_TIME 104 /* T_SET     - seasip BDOS 104              */
# define BDOS_GET_DATE_TIME 105 /* T_GET     - seasip BDOS 105 (binary hms) */
# define BDOS_P_CODE 108        /* P_CODE    - get/put program return code  */
# define BDOS_F_PARSE 152       /* F_PARSE   - parse ASCII filename to FCB  */
# define BDOS_T_SECONDS 155     /* T_SECONDS - MP/M get date/time, BCD hms  */
# define BDOS_S_OSVER 163       /* S_OSVER   - user-visible OS version      */

/*****************************************************************************/

/*
 * CP/M-386 private BDOS calls!  Above the standard documented CP/M 3
 * MP/M / Concurrent range (also avoiding P2DOS 200/201, DOS+ 210+).
 */

# define BDOS_SYSTEM_REBOOT 220 /* machine reboot (REBOOT.386)        */
# define BDOS_CON_CLEAR 221     /* clear serial ANSI + VGA (CLS.386)  */
# define BDOS_CON_VGA 222       /* get/set VGA console enable         */
# define BDOS_CON_SER 223       /* get/set serial console enable      */
# define BDOS_CON_VIDEO 224     /* map info for direct VGA text (sel) */
# define BDOS_GET_TICKS 225     /* high-res 64-bit tick counter       */
# define BDOS_SLEEP_UNTIL 226   /* busy-wait until absolute tick      */
# define BDOS_RAMDISK_KB 227    /* RAM disk size in KB                */
# define BDOS_MEM_LAYOUT 228    /* physical memory layout             */
# define BDOS_VID_QUERY 229     /* describe the current video mode    */
# define BDOS_VID_ENUM 230      /* describe video mode [index]        */
# define BDOS_VID_SET 231       /* set / commit / revert a video mode */
# define BDOS_VID_FONT 232      /* load console glyphs / restore ROM  */
# define BDOS_VID_PALETTE 233   /* load DAC palette entries           */
# define BDOS_RNG_GET 253       /* get 16 bits of randomness from RNG */
# define BDOS_RNG_SEED 254      /* seed / reseed the RNG              */

/*****************************************************************************/

/*
 * Private numbers sit above standard CP/M 3 / MP/M / Concurrent calls and
 * avoid conflicts - see https://www.seasip.info/Cpm/bdos.html for a list.
 */

/*****************************************************************************/

/* Ring-3 video map result (BDOS 224; DE -> this, TPA-relative). */
struct cpm_vga_text
{
  UWORD sel;        /* selector|RPL3, or 0 if unavailable */
  UWORD cols;       /* text columns (e.g. 80)             */
  UWORD rows;       /* text rows (e.g. 25)                */
  UWORD cell_bytes; /* bytes per cell (2)                 */
  ULONG map_size;   /* mapped byte length (e.g. 32K)      */
  ULONG phys_base;  /* physical base (informational)      */
};

/*****************************************************************************/

/*
 * High-resolution tick block (BDOS 225/226; DE -> this, TPA-relative).
 * BDOS 225 (GET_TICKS): fills lo/hi/hz; returns 0, or 0xFFFF if DE bad.
 * BDOS 226 (SLEEP_UNTIL): waits until absolute tick >= (hi:lo); hz ignored;
 * returns 0, or 0xFFFF if DE bad.  Rate is PIT_HZ (1_193_182 Hz).
 */

struct cpm_ticks
{
  ULONG lo; /* low 32 bits of absolute tick              */
  ULONG hi; /* high 32 bits of absolute tick             */
  ULONG hz; /* tick frequency (GET fills; SLEEP ignores) */
};

/*****************************************************************************/

/*
 * Physical memory layout (BDOS 228; DE -> this, TPA-relative).
 * The kernel lives in conventional memory and the TPA above 1MB, with the
 * video/ROM hole in between, so the layout cannot be inferred from the TPA
 * limits alone.  Returns 0, or 0xFFFF if DE is bad.
 */

struct cpm_memlayout
{
  ULONG kernel_base;   /* load address of the kernel image        */
  ULONG kernel_end;    /* end of image, including the RAM disk    */
  ULONG ramdisk_base;  /* RAM disk within the image               */
  ULONG ramdisk_size;  /* RAM disk byte length                    */
  ULONG lowmem_top;    /* end of usable conventional memory       */
  ULONG tpa_base;      /* TPA (>= 1MB)                            */
  ULONG tpa_top;       /* end of the loadable region              */
  ULONG stack_top;     /* end of the ring-3 stack reserve         */
  ULONG mem_flags;     /* MEMF_* from memmap.h                    */
};

/*****************************************************************************/

/*
 * Video mode description (BDOS 229/230; DE -> this, TPA-relative).
 *
 * BDOS 229 (VID_QUERY) fills for the mode the hardware is currently in.
 * BDOS 230 (VID_ENUM) reads .index and fills the rest for that entry.
 * Both return , or VIDR_* on failure: 0xFFFE when there is no usable adapter,
 *   0xFFFC when the enumeration index is past the end.
 *
 * The width/height/bpp/pitch/fb_* fields describe gfx modes,and are
 * zero for text modes.
 */

struct cpm_vidmode
{
  UWORD index;      /* IN for enum, OUT: table ordinal          */
  UWORD mode;       /* CP/M-386 mode id                         */
  UWORD flags;      /* VIDM_*                                   */
  UWORD cols;       /* text columns                             */
  UWORD rows;       /* text rows                                */
  UWORD cell_bytes; /* bytes per text cell (2)                  */
  UWORD width;      /* pixels across (graphics)                 */
  UWORD height;     /* pixels down (graphics)                   */
  UWORD bpp;        /* bits per pixel (graphics)                */
  UWORD sel;        /* ring-3 selector|RPL3 for the plane, or 0 */
  ULONG pitch;      /* bytes per scan line (graphics)           */
  ULONG fb_size;    /* mapped bytes                             */
  ULONG fb_phys;    /* physical base (informational)            */
};

/*****************************************************************************/

/*
 * Video mode request (BDOS 231; DE -> this, TPA-relative).
 *
 * action is VIDA_TRANSIENT (a program mode, dropped when the program
 * exits), VIDA_CONSOLE (a console mode, provisional until committed),
 * VIDA_COMMIT (make the provisional mode stick) or VIDA_REVERT (go back to
 * the committed console mode).  mode is ignored for COMMIT and REVERT.
 *
 * A console mode that is never committed is undone when the program
 * leaves, so a crash or a hang cannot strand the console in a mode the
 * user cannot read.
 */

struct cpm_vidset
{
  UWORD mode;
  UWORD action;
  UWORD flags; /* reserved, must be 0 */
  UWORD pad;
};

/*****************************************************************************/

/*
 * Console font (BDOS 232; DE -> this, TPA-relative).
 *
 * data is itself a TPA-relative pointer to count glyphs of height bytes
 * each, or 0 to put the ROM font back.  height must match the cell height
 * of the current mode - pick the cell height with a text mode first, then
 * load glyphs to match, so loading a font never has to reprogram the CRTC.
 *
 * Returns 0, or VIDR_* (see vidmode.h); 0xFFFA when the geometry does not
 * fit the current mode.
 */

struct cpm_vidfont
{
  ULONG data;   /* TPA-relative glyph data, or 0 for the ROM font */
  UWORD height; /* scan lines per glyph                           */
  UWORD count;  /* number of glyphs                               */
  UWORD first;  /* first glyph index                              */
  UWORD flags;  /* reserved, must be 0                            */
};

/*****************************************************************************/

/*
 * Palette (BDOS 233; DE -> this, TPA-relative).
 *
 * Ring 3 cannot reach the DAC ports, so palette changes come through here.
 * data is a TPA-relative pointer to three bytes per entry - red, green,
 * blue - each in the VGA's native range of 0 to 63.
 *
 * Returns 0, or VIDR_* (see vidmode.h).
 */

struct cpm_vidpal
{
  ULONG data;  /* TPA-relative rgb triples */
  UWORD first; /* first DAC index          */
  UWORD count; /* number of entries        */
  UWORD flags; /* reserved, must be 0      */
  UWORD pad;
};

/*****************************************************************************/

/*
 * Console cursor (BDOS 234; DE -> this, TPA-relative).
 *
 * flags for which of shape/visible/blinking to apply; the rest of the
 * block is ignored and filled with the resulting state, so 0 is query.
 *
 * A full steady cursor cannot be done with pure VGA hardwarem, so
 * non-blinking is drawn into the text plane directly.
 *
 * returns 0, VIDR_NOHW when no adapter is fitted, or VIDR_BADPTR.
 */

# define VIDC_SET_SHAPE 0x0001
# define VIDC_SET_VISIBLE 0x0002
# define VIDC_SET_BLINK 0x0004

# define VIDC_SHAPE_KEEP 0
# define VIDC_SHAPE_BLOCK 1
# define VIDC_SHAPE_UNDERLINE 2
# define VIDC_SHAPE_HALF 3
# define VIDC_SHAPE_EXPLICIT 4 /* use .start and .end verbatim */

struct cpm_vidcursor
{
  UWORD flags;   /* VIDC_SET_*                                    */
  UWORD shape;   /* VIDC_SHAPE_*                                  */
  UWORD start;   /* first scan line of the cursor within the cell */
  UWORD end;     /* last scan line                                */
  UWORD visible; /* 0 hidden, 1 shown                             */
  UWORD blink;   /* 0 steady, 1 blinking                          */
  UWORD cell_h;  /* out: cell height in scan lines                */
  UWORD pad;
};

/*****************************************************************************/

/*
 * Keyboard locks (BDOS 236; DE -> this, TPA-relative).
 *
 * flags for which of locks/leds to apply, 0 is a query.
 *
 * Returns 0, or 0xFFFF on a bad pointer.
 */

# define KBDL_SET_LOCKS 0x0001
# define KBDL_SET_LEDS 0x0002
# define KBDL_SCROLL 0x0001
# define KBDL_NUM 0x0002
# define KBDL_CAPS 0x0004

struct cpm_kbdlock
{
  UWORD flags; /* KBDL_SET_*                                  */
  UWORD locks; /* KBDL_SCROLL / KBDL_NUM / KBDL_CAPS, set = on */
  UWORD leds;  /* the same bits: which lamps may illuminate    */
  UWORD pad;
};

/*****************************************************************************/

/*
 * CSPRNG seed block (BDOS 254; DE -> this, TPA-relative).
 *
 * data is a TPA-relative pointer to len bytes of seed material
 * (1..64).  The material is XORed into the existing CSPRNG pool and
 * then diffused via a Salsa20/20 permutation, so it always folds innew
 * entropy in rather than replacing the pool state.
 *
 * Returns 0, or 0xFFFF if DE or the data pointer is out of range.
 */

struct cpm_rng_seed
{
  ULONG data; /* TPA-relative pointer to seed bytes */
  ULONG len;  /* byte count, 1..64                  */
};

/*****************************************************************************/

# define robit 0    /* read-only bit in file type field of fcb */
# define arbit 2    /* archive bit in file type field of fcb   */
# define SECLEN 128 /* length of a CP/M sector                 */

/*****************************************************************************/

/* File Control Block definition */
struct fcb
{
  UBYTE drvcode;   /* 0 = default drive, 1..16 are drives A..P */
  UBYTE fname [8]; /* File name (ASCII)                        */
  UBYTE ftype [3]; /* File type (ASCII)                        */
  UBYTE extent;    /* Extent number (bits 0..4 used)           */
  /* s1 = Last Record Byte Count (LRBC) on CP/M Plus / DOS Plus /
   * CP/M-386.  DOS-PLUS interpretation (this OS):
   *   0  = last record is full (128 data bytes used), or empty file
   *   1..127 = number of data bytes used in the last record
   * Exact size = (records-1)*128 + lrbc  when lrbc!=0 && records>0
   *            = records*128             when lrbc==0
   * (ISX uses "unused bytes in last record"; we do NOT use ISX.)
   * See https://www.seasip.info/Cpm/bytelen.html
   * FCB+32 (cur_rec)=0xFF on open returns LRBC there; F6'+func30 sets it. */
  UBYTE s1;     /* Last Record Byte Count (DOS-PLUS)    */
  UBYTE s2;     /* Module field (bits 0..5), write flag (7) */
  UBYTE rcdcnt; /* Nmbr records used in this extent, 0..128 */
  union
  {
    UBYTE small [16]; /* 16 block numbers of 1 byte           */
    WORD big [8];     /* or 8 block numbers of 1 word         */
  } dskmap;
  UBYTE cur_rec; /* current record field (FCB+32); also
                  * LRBC in/out for open(0xFF)/set_attr  */
  UBYTE ran0;    /* random record field (3 bytes)        */
  UBYTE ran1;
  UBYTE ran2;
};

/*****************************************************************************/

/* Declaration of directory entry */
struct dirent
{
  UBYTE entry;     /* 0 - 15 for user numbers, E5 for empty    */
                   /* the rest are reserved                    */
  UBYTE fname [8]; /* File name (ASCII)                        */
  UBYTE ftype [3]; /* File type (ASCII)                        */
  UBYTE extent;    /* Extent number (bits 0..4 used)           */
  UBYTE s1;        /* Last Record Byte Count (DOS-PLUS)        */
  UBYTE s2;        /* Module field (bits 0..5), write flag (7) */
  UBYTE rcdcnt;    /* Nmbr records used in this extent, 0..128 */
  union
  {
    UBYTE small [16]; /* 16 block numbers of 1 byte   */
    WORD big [8];     /* or 8 block numbers of 1 word */
  } dskmap;
};

/*****************************************************************************/

/* Declaration of disk parameter tables */
struct dpb /* disk parameter table */
{
  UWORD spt;     /* sectors per track            */
  UBYTE bsh;     /* block shift factor           */
  UBYTE blm;     /* block mask                   */
  UBYTE exm;     /* extent mask                  */
  UBYTE dpbdum;  /* dummy byte for fill          */
  UWORD dsm;     /* max disk size in blocks      */
  UWORD drm;     /* max directory entries        */
  UWORD dir_al;  /* initial allocation for dir   */
  UWORD cks;     /* number dir sectors to chksum */
  UWORD trk_off; /* track offset                 */
};

/*****************************************************************************/

struct dph /* disk parameter header */
{
  UBYTE *xlt;       /* pointer to sector translate table    */
  UWORD hiwater;    /* high water mark for this disk        */
  UWORD dum1;       /* dummy (unused)                       */
  UWORD dum2;       /* dummy (unused)                       */
  UBYTE *dbufp;     /* pointer to 128 byte directory buffer */
  struct dpb *dpbp; /* pointer to disk parameter block      */
  UBYTE *csv;       /* pointer to check vector              */
  UBYTE *alv;       /* pointer to allocation vector         */
};

/*****************************************************************************/

/* Declaration of structure containing "global" state variables */
# define TBUFSIZ 126 /*sw # typed-ahead characters */
struct stvars
{
  UBYTE kbchar;           /* keyboard type-ahead buffer count        */
  UBYTE delim;            /* Delimiter for function 9                */
  BOOLEAN lstecho;        /* True if echoing console output to lst:  */
  BOOLEAN echodel;        /* Echo char when getting <del> ?          */
  UWORD column;           /* CRT column number for expanding tabs    */
  UBYTE curdsk;           /* Currently selected disk                 */
  UBYTE dfltdsk;          /* Default disk (last selected by fcn 14)  */
  UBYTE user;             /* Current user number                     */
  struct dph *dphp;       /* pointer to disk parm hdr for cur disk   */
  struct dirent *dirbufp; /* pointer for directory buff for process  */
                          /* stored here so that each process can    */
                          /* have a separate dirbuf.                 */
  struct dpb *parmp;      /* pointer to disk parameter block for cur */
                          /* disk. Stored here to save ref calc      */
  UWORD srchpos;          /* position in directory for search next   */
  UBYTE *dmaadr;          /* Disk dma address                        */
  struct fcb *srchp;      /* Pointer to search FCB for function 17   */
  UBYTE *excvec [18];     /* Array of exception vectors              */
  UBYTE *insptr;          /* sw Insertion pointer for typeahead      */
  UBYTE *remptr;          /* sw Removal pointer for typeahead        */
  UBYTE t_buff [TBUFSIZ]; /* sw Type-ahead buffer itself             */
};
/*sw removed next line from structure */
extern UBYTE *chainp; /* Used for chain to program call */

/*****************************************************************************/

/* Console buffer structure declaration */

struct conbuf
{
  UBYTE maxlen;   /* Maximum length from calling routine */
  UBYTE retlen;   /* Length actually found by BDOS       */
  UBYTE cbuf [0]; /* Console data                        */
};

/*****************************************************************************/

/*
 * .386 absolute executable format.  Inspired by CP/M-68K absolute files,
 * adapted to flat 32-bit protected mode; all offsets are relative to the
 * TPA allocation base, NOT to physical 0000:0100.
 *
 *   0x00  4  magic      "M386" (CPM386_MAGIC), so a program is identifiable
 *   0x04  1  version    format version, currently 1
 *   0x05  1  hdr_size   bytes of header before the image (32 for version 1)
 *   0x06  2  flags      reserved, must be 0
 *   0x08  4  load_off   image is placed at tpa_base + load_off
 *   0x0C  4  img_size   bytes of image following the header
 *   0x10  4  entry_off  control transfers to tpa_base + entry_off
 *   0x14  4  min_kb     total TPA required, in KB; 0 means do not check
 *   0x18  8  reserved   must be 0
 *
 * followed by exactly img_size bytes of image, verbatim memory content.
 *
 * min_kb lets a program state what it needs beyond its own image - a small
 * image with a large BSS or heap - so the loader can refuse it up front
 * with a useful message instead of letting it fault later.
 *
 * Convention: load/entry = 0x100, the classic CP/M program origin;
 * TPA[0..0xFF] is the base page holding the default FCBs and command tail.
 */

# define CPM386_MAGIC 0x3638334DUL /* "M386" little-endian */
# define CPM386_VERSION 1
# define CPM386_HDR_SIZE 32

typedef struct
{
  unsigned long magic;
  unsigned char version;
  unsigned char hdr_size;
  unsigned short flags;
  unsigned long load_off;
  unsigned long img_size;
  unsigned long entry_off;
  unsigned long min_kb;
} CPM386_HDR;

/*****************************************************************************/

/* Loader result codes, returned by BDOS 59 and the cpm386_load_* helpers. */

# define CPMLD_OK 0x0000
# define CPMLD_NOMEM 0xFFFA    /* declared requirement exceeds the TPA   */
# define CPMLD_TRUNC 0xFFFB    /* image data ends early                  */
# define CPMLD_BADENTRY 0xFFFC /* entry point outside the image          */
# define CPMLD_BADSIZE 0xFFFD  /* load/size does not fit the TPA         */
# define CPMLD_BADHDR 0xFFFE   /* bad magic, version, or header fields   */

/*****************************************************************************/

/*
 * Set by the loaders when a load is refused for want of memory, so the CCP
 * can say how much was wanted and how much there was.  Both are in KB.
 */

unsigned long cpm386_load_req_kb (void);
unsigned long cpm386_load_avail_kb (void);

/*****************************************************************************/

/*
 * Pure loader: parse header from in-memory buffer (hdr+img), validate
 * against provided TPA base/len, copy raw image bytes to (tpa_base +
 * load_off), return new entry addr via *entry_out or error code. No
 * disk or global mutation so fully testable with mocked buffers.
 */

UWORD cpm386_load_from_buf (const UBYTE *filebuf, unsigned long buflen,
                            UBYTE *tpa_base, unsigned long tpa_len,
                            UBYTE **entry_out);

/*****************************************************************************/

/*
 * Streaming loader: pull back to back 128-byte CP/M records via callback.
 * Reader returns: 0 = record data valid, 1 = EOF/unwritten (zero-fill if still
 * within image), other = hard error. Handles multi-extent files (caller's
 * sequential BDOS read crosses extents) and allocation holes (status 1
 * mid-image -> zero-filled record). No size cap beyond TPA.
 */

typedef UWORD (*cpm386_rec_reader) (UBYTE rec [128], void *ctx);
UWORD cpm386_load_from_reader (cpm386_rec_reader reader, void *ctx,
                               UBYTE *tpa_base, unsigned long tpa_len,
                               UBYTE **entry_out);

/*****************************************************************************/

/*
 * dirscan() callback: invoked for each directory entry.
 * Return non-zero to report a match (see dskutil.c parms bits).
 * Matches the K&R definitions of openfile/create/delete/ (fcbp, dirp, index).
 */
typedef BOOLEAN (*DIRSCAN_FN) (UBYTE *fcbp, UBYTE *dirp, WORD dirindx);

UWORD dirscan (DIRSCAN_FN funcp, UBYTE *fcbp, UWORD parms);

/*****************************************************************************/

#endif

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
