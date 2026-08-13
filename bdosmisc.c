/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 3cee445c-82b4-11f1-ac88-80ee73e9b8e7
 */

/*****************************************************************************/

/*****************************************************************
 *                                                               *
 *               CP/M-68K BDOS Miscellaneous Module              *
 *                                                               *
 *       This module contains miscellaneous loose ends for       *
 *       CP/M-68K.  Included are:                                *
 *                                                               *
 *               bdosinit()  - BDOS initialization routine       *
 *                             called from CCP for system init   *
 *               warmboot()  - BDOS warm boot exit routine       *
 *               error()     - BDOS error printing routine       *
 *               ro_err()    - BDOS read-only file error routine *
 *               setexc()    - BDOS set exception vector         *
 *               set_tpa()   - BDOS get/set TPA limits           *
 *               serial # and copyright notice, machine readable *
 *                                                               *
 *                                                               *
 *       Configured (originally) for Alcyon C on the VAX         *
 *                                                               *
 *      Modified 2/5/84 sw for ^C disk reset.                    *
 *      Again    3/17/84   for chain hack                        *
 *                                                               *
 *****************************************************************/

/*****************************************************************************/

#include "bdosinc.h" /* Standard I/O declarations */

/*****************************************************************************/

#include "bdosdef.h" /* Type and structure declarations for BDOS */

/*****************************************************************************/

#include "biosdef.h" /* BIOS definitions, needed for bios wboot */

/*****************************************************************************/

/* serial # and copyright notice */
char *copyrt = "CP/M-386 Version 0.1 (" BUILDDATE ")$";
char *copyr1 = "Copyright (c) 1982-1984 Digital Research, Inc.$";
char *copyr2 = "Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>$";
char *serial = "XXXX-0000-654321$";

/*****************************************************************************/

/*  Declare external functions (prototypes for modern gcc) */
EXTERN void conout (UBYTE);
EXTERN UBYTE conin (void);
EXTERN void prt_line (UBYTE *);
EXTERN UWORD _bdos (WORD, UWORD, UBYTE *);
EXTERN UBYTE *traphndl (void);
EXTERN void initexc (UBYTE **);

/*****************************************************************************/

/* dirscan */
EXTERN BOOLEAN set_attr (UBYTE *, UBYTE *, UWORD);
EXTERN UWORD dir_rd (WORD);

/*****************************************************************************/

/*  Declare external variables */
EXTERN UWORD log_dsk;  /* logged-on disk vector        */
EXTERN UWORD ro_dsk;   /* read-only disk vector        */
EXTERN UWORD crit_dsk; /* vector of critical disks     */
EXTERN BYTE *tpa_lt;   /* TPA lower limit (temporary)  */
EXTERN BYTE *tpa_lp;   /* TPA lower limit (permanent)  */
EXTERN BYTE *tpa_ht;   /* TPA upper limit (temporary)  */
EXTERN BYTE *tpa_hp;   /* TPA upper limit (permanent)  */
EXTERN BOOLEAN submit; /* external variables from CCP  */
EXTERN BOOLEAN morecmds;

/*****************************************************************************/

#define trap2v 34 /* trap 2 vector number */
#define ctrlc 3   /* control-c            */

/*****************************************************************************/

/********************************
 * bdos initialization routine  *
 ********************************/

bdosinit ()
/* Initialize the File System */
{
  REG struct
  {
    WORD nmbr;
    BYTE *low;
    LONG length;
  } *segp;
  BSETUP

  bsetvec (trap2v, &traphndl); /* set up trap vector                */
  GBL.kbchar = 0;              /* initialize the "global" variables */
  GBL.insptr = GBL.remptr = &(GBL.t_buff [0]);
  GBL.delim = '$';
  GBL.lstecho = FALSE;
  GBL.echodel = FALSE;
  chainp = NULL;             /*sw Used to be GBL.chainp             */
  _bdos (13, 0, 0);          /* reset disk system function          */
  segp = (void *)bgetseg (); /* get pointer to memory segment table */
  tpa_lt = tpa_lp = segp->low;
  tpa_ht = tpa_hp = tpa_lp + segp->length;
  initexc (&(GBL.excvec [0]));
}

/*****************************************************************************/

/************************
 * warmboot entry point *
 ************************/

warmboot (parm)
    /* Warm Boot the system */
    WORD parm; /* 1 to reset submit flag */
{
  BSETUP

  /* CP/M 3 P_CODE: Control-C termination stores 0xFFFE */
  if (parm == 2)
    (void)_bdos (108, (UWORD)0xFFFE, (UBYTE *)0);

  if (parm != 2)        /*sw Not ^C                          */
    log_dsk &= ~ro_dsk; /* log off any disk marked read-only */
  else
    log_dsk &= (1 << GBL.curdsk); /*sw Log off all but current drive, as */
                                  /*   per manual.  (^C only)            */

  /*
   * Note that this code is specifically for a single-thread system.
   * It won't work in a multi-tasking system.
   */

  /*sw The above is still very much true */
  ro_dsk = 0;
  crit_dsk = 0;
  if (parm)
    submit = morecmds = FALSE;
  GBL.curdsk = 0xff; /* set current disk to "unknown" */
  GBL.delim = '$';   /* reset custom delimiter */
  tpa_lt = tpa_lp;
  tpa_ht = tpa_hp;
  initexc (&(GBL.excvec [0]));
  bwboot ();
}

/*****************************************************************************/

/*************************/
/*  disk error handlers  */
/*************************/

prt_err (p)
    /*  print the error message  */

    BYTE *p;
{
  BSETUP

  prt_line (p);
  prt_line (" error on drive $");
  conout (GBL.curdsk + 'A');
}

abrt_err (p)
    /*  print the error message and always abort */

    BYTE *p;
{
  prt_err (p);
  warmboot (1);
}

char *warning = "\r\nWARNING -- Do not attempt to change disks$";
ext_err (cont, p)
/* print the error message,  allow for retry, abort, or ignore */

REG BOOLEAN cont; /* Boolean for whether continuing is allowed */
BYTE *p;          /* pointer to error message                  */
{
  REG UBYTE ch;

  prt_err (p);
  prt_line (warning);
  do
    {
      prt_line ("\n\rDo you want to:  Abort (A),  Retry (R)$");

      if (cont)
        prt_line (", or Continue with bad data (C)$");
      prt_line ("? $");
      ch = conin () & 0x5f;
      prt_line ("\r\n$");

      switch (ch)
        {
        case ctrlc:

        case 'A':
          warmboot (1); /* does not return */

          break;

        case 'C':
          if (cont)
            return (1);

          break;

        case 'R':
          return (0);
        }
    }

  while (TRUE);
}

/*****************************************************************************/

/********************************/
/* Read-only File Error Routine */
/********************************/

UWORD ro_err (fcbp, dirindx) /*  File R/O error  */
REG struct fcb *fcbp;
WORD dirindx;
{
  REG BYTE *p;
  REG UWORD i;
  REG UBYTE ch;

  p = (BYTE *)fcbp;
  prt_line ("CP/M Disk file error: $");
  i = 8;

  do
    conout (*++p & 0x7f);
  while (--i);

  conout ('.');
  i = 3;

  do
    conout (*++p & 0x7f);
  while (--i);

  prt_line (" is read-only.$");
  prt_line (warning);

  do
    {
      prt_line (
          "\r\nDo you want to: Change it to read/write (C), or Abort (A)? $");
      ch = conin () & 0x5f;
      prt_line ("\r\n$");

      switch (ch)
        {
        case ctrlc:

        case 'A':
          warmboot (1); /* does not return */

          break;

        case 'C':
          fcbp->ftype [robit] &= 0x7f;
          dirscan (set_attr, fcbp, 2);

          return (dir_rd (dirindx >> 2));

        } /* Reset the directory buffer !!!! */
    }

  while (TRUE);
}

/*****************************************************************************/

/************************
 *  error entry point    *
 ************************/

UWORD error (errnum)
    /* Print error message, do appropriate response */

    UWORD errnum; /* error number */
{
  BSETUP

  prt_line ("\r\nCP/M Disk $");

  switch (errnum)
    {
    case 0:
      return (ext_err (TRUE, "read$"));

    case 1:
      return (ext_err (TRUE, "write$"));

    case 2:
      abrt_err ("select$"); /* does not return */

      break;

    case 3:
      return (ext_err (FALSE, "select$"));

    case 4:
      abrt_err ("change$"); /* does not return */

      break;
    }
}

/*****************************************************************************/

/*****************************
 *  set exception entry point *
 *****************************/

struct setexc_struct
{
  WORD vecnum;
  BYTE *newvec;
  BYTE *oldvec;
};

UWORD setexc (epbp)
    /* Set Exception Vector */
    REG struct setexc_struct *epbp;

{
  REG WORD i;
  BSETUP

  i = epbp->vecnum - 2;

  if (i == 32 || i == 33)
    return (-1);

  if ((30 <= i) && (i <= 37))
    i -= 20;
  else if ((i < 0) || (i > 9))
    return (255);

  epbp->oldvec = GBL.excvec [i];
  GBL.excvec [i] = epbp->newvec;

  return (0);
}

/*****************************************************************************/

/*****************************
 *  get/set TPA entry point   *
 *****************************/

struct set_tpa_struct
{
  UWORD parms;
  BYTE *low;
  BYTE *high;
};

set_tpa (p)
    /* Get/Set TPA Limits */
    REG struct set_tpa_struct *p;

#define set 1
#define sticky 2

{
  if (p->parms & set)
    {
      tpa_lt = p->low;
      tpa_ht = p->high;

      if (p->parms & sticky)
        {
          tpa_lp = tpa_lt;
          tpa_hp = tpa_ht;
        }
    }
  else
    {
      p->low = tpa_lt;
      p->high = tpa_ht;
    }
}

/*****************************************************************************/

static unsigned long
le32 (const UBYTE *p)
{
  return (unsigned long)(unsigned char)p [0]
      | ((unsigned long)(unsigned char)p [1] << 8)
      | ((unsigned long)(unsigned char)p [2] << 16)
      | ((unsigned long)(unsigned char)p [3] << 24);
}

/*****************************************************************************/

/*
 * Recorded when a load is refused for want of memory, so the CCP can
 * report both numbers rather than a bare "insufficient memory".
 */

static unsigned long ld_req_kb;
static unsigned long ld_avail_kb;

/*****************************************************************************/

unsigned long
cpm386_load_req_kb (void)
{
  return ld_req_kb;
}

/*****************************************************************************/

unsigned long
cpm386_load_avail_kb (void)
{
  return ld_avail_kb;
}

/*****************************************************************************/

/*
 * Validate a header and unpack it.  Shared by both loaders so the two can
 * never disagree about what a valid program looks like.
 */

static UWORD
cpm386_parse_hdr (const UBYTE *h, unsigned long tpa_len, CPM386_HDR *out)
{
  unsigned long need;

  out->magic = le32 (h);
  out->version = h [4];
  out->hdr_size = h [5];
  out->flags = (unsigned short)(h [6] | (h [7] << 8));
  out->load_off = le32 (h + 8);
  out->img_size = le32 (h + 12);
  out->entry_off = le32 (h + 16);
  out->min_kb = le32 (h + 20);

  if (out->magic != CPM386_MAGIC)
    return CPMLD_BADHDR;

  if (out->version != CPM386_VERSION)
    return CPMLD_BADHDR;

  if (out->hdr_size != CPM386_HDR_SIZE)
    return CPMLD_BADHDR;

  if (out->flags != 0)
    return CPMLD_BADHDR;

  /* Reject wrap and images that do not fit the TPA. */
  if (out->img_size == 0 || out->load_off > tpa_len
      || out->img_size > tpa_len - out->load_off)
    return CPMLD_BADSIZE;

  /* Entry must land inside the image, not one past its end. */
  if (out->entry_off < out->load_off
      || out->entry_off >= out->load_off + out->img_size)
    return CPMLD_BADENTRY;

  /*
   * A stated requirement is the total TPA the program needs, image
   * included.  Zero means the program has not said, so nothing is checked.
   */

  if (out->min_kb != 0)
    {
      need = out->min_kb;
      ld_req_kb = need;
      ld_avail_kb = tpa_len / 1024UL;

      /* Compare in KB; the multiply could overflow on a big request. */
      if (need > ld_avail_kb)
        return CPMLD_NOMEM;
    }

  return CPMLD_OK;
}

/*****************************************************************************/

UWORD
cpm386_load_from_buf (const UBYTE *filebuf, unsigned long buflen,
                      UBYTE *tpa_base, unsigned long tpa_len,
                      UBYTE **entry_out)
{
  CPM386_HDR h;
  const UBYTE *img;
  unsigned long i;
  UWORD rc;

  if (buflen < CPM386_HDR_SIZE || !filebuf || !tpa_base || !entry_out)
    return CPMLD_BADHDR;

  rc = cpm386_parse_hdr (filebuf, tpa_len, &h);

  if (rc != CPMLD_OK)
    return rc;

  img = filebuf + CPM386_HDR_SIZE;

  if (buflen - CPM386_HDR_SIZE < h.img_size)
    return CPMLD_TRUNC;

  /* place verbatim */
  for (i = 0; i < h.img_size; i++)
    tpa_base [h.load_off + i] = img [i];

  *entry_out = tpa_base + h.entry_off;

  return CPMLD_OK;
}

/*****************************************************************************/

UWORD
cpm386_load_from_reader (cpm386_rec_reader reader, void *ctx, UBYTE *tpa_base,
                         unsigned long tpa_len, UBYTE **entry_out)
{
  UBYTE rec [128];
  CPM386_HDR h;
  unsigned long sz, placed, i, n;
  UWORD r;
  UBYTE *dst;

  if (!reader || !tpa_base || !entry_out)
    return CPMLD_BADHDR;

  /* Record 0: the header, followed by the first of the image. */
  r = reader (rec, ctx);

  if (r != 0 && r != 1)
    return CPMLD_BADHDR;

  if (r == 1)
    {
      /* completely empty / missing first record */
      for (i = 0; i < 128; i++)
        rec [i] = 0;
    }

  r = cpm386_parse_hdr (rec, tpa_len, &h);

  if (r != CPMLD_OK)
    return r;

  sz = h.img_size;

  /* Whatever is left of the first record after the header. */
  n = 128 - CPM386_HDR_SIZE;

  if (n > sz)
    n = sz;
  dst = tpa_base + h.load_off;

  for (i = 0; i < n; i++)
    dst [i] = rec [CPM386_HDR_SIZE + i];

  placed = n;

  /*
   * Remaining image as full 128-byte records (last may be partial).
   * reader status 1 mid-image = allocation hole or short last record:
   * zero-fill that record and continue until img_size is satisfied.
   */

  while (placed < sz)
    {
      r = reader (rec, ctx);

      if (r != 0 && r != 1)
        return CPMLD_BADHDR;

      n = sz - placed;

      if (n > 128)
        n = 128;

      if (r == 1)
        {
          /* hole/unwritten/EOF: treat as zeros for absolute image */
          for (i = 0; i < n; i++)
            dst [placed + i] = 0;
        }
      else
        {
          for (i = 0; i < n; i++)
            dst [placed + i] = rec [i];
        }
      placed += n;

      /*
       * True EOF before image complete: only an error if still
       * need data and got a short last successful read is OK;
       * status 1 after all placed is fine (loop ends).
       */
    }

  *entry_out = tpa_base + h.entry_off;

  return CPMLD_OK;
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
