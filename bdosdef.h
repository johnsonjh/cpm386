/*********************************************************
 *                                                       *
 *               CP/M-68K header file                    *
 *    Copyright (c) 1982 by Digital Research, Inc.       *
 *    Structure definitions for BDOS globals             *
 *       and BDOS data structures                        *
 *                                                       *
 *       Desecrated 6-Aug-83 (sw) for type-ahead         *
 *       Again     17-Mar-84 (sw) for chaining           *
 *                                                       *
 *********************************************************/

#ifndef BDOSDEF_H
#define BDOSDEF_H

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

#define snglthrd TRUE
/* TRUE for single-thread environment */
/* FALSE to create based structure for re-entrant model */
#if snglthrd
# define GBL gbls
/* In single thread case, GBL just names the structure */
# define BSETUP EXTERN struct stvars gbls;
/* and BSETUP defines the extern structure */
#endif /* if snglthrd */

#if !snglthrd
# define GBL (*statep)
/* If multi-task, state vars are based */
# define BSETUP              \
  REG struct stvars *statep; \
  statep = &gbls;
/* set up pointer to state variables */
/* This is intended as an example to show the intent */
#endif /* if !snglthrd */

/*
 * Note that there are a few critical regions in the file system that must
 * execute without interruption.  They pertain mostly to the manipulation of
 * the allocation vector.  This isn't a problem in a single-thread model, but
 * must be provided for in a multi-tasking file system.  Consequently, the
 * primitives LOCK and UNLOCK are defined and used where necessary in the
 * file system.  For the single thread model, they are null routines.
 */

#define LOCK   /**/
#define UNLOCK /**/

/*
 * Be sure LOCK and UNLOCK are implemented to allow recursive calls to LOCK.
 * That is, if a process that calls LOCK already owns the lock, let it proceed,
 * but remember that only the outer-most call to UNLOCK really releases the
 * file system.
 */

#define VERSION 0x2022 /* BDOS version (func 12): type 0x20, BDOS 2.2 */

/*
 * S_OSVER (func 163): user-visible product version.  High byte = machine
 * family (same convention as func 12); low byte = BCD major.minor.
 * CP/M-386 0.1 => 0x2001.
 */

#define OSVER_CPM386 0x2001

/*
 * CP/M Plus-compatible date/time (see rtc.h).  days since 1978-01-01;
 * h/m/s are binary on CP/M-386 (CP/M Plus uses BCD).
 */

#define BDOS_SET_DATE_TIME 104 /* T_SET     - seasip BDOS 104              */
#define BDOS_GET_DATE_TIME 105 /* T_GET     - seasip BDOS 105 (binary hms) */
#define BDOS_P_CODE 108        /* P_CODE    - get/put program return code  */
#define BDOS_F_PARSE 152       /* F_PARSE   - parse ASCII filename to FCB  */
#define BDOS_T_SECONDS 155     /* T_SECONDS - MP/M get date/time, BCD hms  */
#define BDOS_S_OSVER 163       /* S_OSVER   - user-visible OS version      */

/*
 * CP/M-386 private BDOS calls!  Above the standard documented CP/M 3
 * MP/M / Concurrent range (also avoiding P2DOS 200/201, DOS+ 210+).
 */

#define BDOS_SYSTEM_REBOOT 220 /* machine reboot (REBOOT.386)       */
#define BDOS_CON_CLEAR 221     /* clear serial ANSI + VGA (CLS.386) */
#define BDOS_CON_VGA 222       /* get/set VGA console enable        */
#define BDOS_CON_SER 223       /* get/set serial console enable     */

#define robit 0    /* read-only bit in file type field of fcb */
#define arbit 2    /* archive bit in file type field of fcb   */
#define SECLEN 128 /* length of a CP/M sector                 */

/* File Control Block definition */
struct fcb
{
  UBYTE drvcode;  /* 0 = default drive, 1..16 are drives A..P */
  UBYTE fname[8]; /* File name (ASCII)                    */
  UBYTE ftype[3]; /* File type (ASCII)                    */
  UBYTE extent;   /* Extent number (bits 0..4 used)       */
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
    UBYTE small[16]; /* 16 block numbers of 1 byte           */
    WORD big[8];     /* or 8 block numbers of 1 word         */
  } dskmap;
  UBYTE cur_rec; /* current record field (FCB+32); also
                  * LRBC in/out for open(0xFF)/set_attr  */
  UBYTE ran0;    /* random record field (3 bytes)        */
  UBYTE ran1;
  UBYTE ran2;
};

/* Declaration of directory entry */
struct dirent
{
  UBYTE entry;    /* 0 - 15 for user numbers, E5 for empty    */
                  /* the rest are reserved                    */
  UBYTE fname[8]; /* File name (ASCII)                        */
  UBYTE ftype[3]; /* File type (ASCII)                        */
  UBYTE extent;   /* Extent number (bits 0..4 used)           */
  UBYTE s1;       /* Last Record Byte Count (DOS-PLUS)        */
  UBYTE s2;       /* Module field (bits 0..5), write flag (7) */
  UBYTE rcdcnt;   /* Nmbr records used in this extent, 0..128 */
  union
  {
    UBYTE small[16]; /* 16 block numbers of 1 byte   */
    WORD big[8];     /* or 8 block numbers of 1 word */
  } dskmap;
};

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

/* Declaration of structure containing "global" state variables */
#define TBUFSIZ 126 /*sw # typed-ahead characters */
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
  UBYTE *excvec[18];      /* Array of exception vectors              */
  UBYTE *insptr;          /* sw Insertion pointer for typeahead      */
  UBYTE *remptr;          /* sw Removal pointer for typeahead        */
  UBYTE t_buff[TBUFSIZ];  /* sw Type-ahead buffer itself             */
};
/*sw removed next line from structure */
extern UBYTE *chainp; /* Used for chain to program call */

/* Console buffer structure declaration */

struct conbuf
{
  UBYTE maxlen;  /* Maximum length from calling routine */
  UBYTE retlen;  /* Length actually found by BDOS       */
  UBYTE cbuf[0]; /* Console data                        */
};

/* .386 absolute executable format (minimal, inspired by CP/M-68K absolute
 * files, adapted to flat 32-bit protected mode, using relative offsets from
 * TPA base): Offset 0:  32-bit little-endian load offset (image at TPA_base +
 * load_off) Offset 4:  32-bit little-endian image size in bytes Offset 8:
 * 32-bit little-endian entry offset (transfer to TPA_base + entry_off) Offset
 * 12+: raw image bytes (exactly img_size bytes, verbatim memory content)
 * Convention: load/entry = 0x100 (classic CP/M program origin); TPA[0..0xFF]
 * is the base page (default FCBs, command tail).  Header values are relative
 * to the TPA allocation base (not physical 0000:0100).
 */

typedef struct
{
  unsigned long load_off;
  unsigned long img_size;
  unsigned long entry_off;
} CPM386_HDR;

/*
 * Pure loader: parse header from in-memory buffer (hdr+img), validate
 * against provided TPA base/len, copy raw image bytes to (tpa_base +
 * load_off), return new entry addr via *entry_out or error code. No
 * disk or global mutation so fully testable with mocked buffers.
 */

UWORD cpm386_load_from_buf (const UBYTE *filebuf, unsigned long buflen,
                            UBYTE *tpa_base, unsigned long tpa_len,
                            UBYTE **entry_out);

/*
 * Streaming loader: pull back to back 128-byte CP/M records via callback.
 * Reader returns: 0 = record data valid, 1 = EOF/unwritten (zero-fill if still
 * within image), other = hard error. Handles multi-extent files (caller's
 * sequential BDOS read crosses extents) and allocation holes (status 1
 * mid-image → zero-filled record). No size cap beyond TPA.
 */

typedef UWORD (*cpm386_rec_reader) (UBYTE rec[128], void *ctx);
UWORD cpm386_load_from_reader (cpm386_rec_reader reader, void *ctx,
                               UBYTE *tpa_base, unsigned long tpa_len,
                               UBYTE **entry_out);

#endif /* BDOSDEF_H */
