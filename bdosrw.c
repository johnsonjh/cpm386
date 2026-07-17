#ifdef RLI
# include "diverge.h"
#endif

/*****************************************************************
 *                                                               *
 *               CP/M-68K BDOS Disk Read/Write Module            *
 *                                                               *
 *       This module contains functions to perform sequential    *
 *       or random access read or write to the disk for CP/M-68K *
 *                                                               *
 *       It includes the following external functions:           *
 *                                                               *
 *               bdosrw()    - sequential and random disk I/O    *
 *                                                               *
 *                                                               *
 *       Compiled with Alcyon C on the VAX                       *
 *                                                               *
 *****************************************************************/

#include "bdosinc.h" /* Standard I/O declarations */

#include "bdosdef.h" /* Type and structure declarations for BDOS */

/* External function definitions */

EXTERN UWORD rdwrt (WORD, UBYTE *, WORD);
EXTERN WORD getaloc (WORD);
EXTERN WORD swap (WORD);

/* dirscan / openfile / create */
EXTERN BOOLEAN openfile (UBYTE *, UBYTE *, WORD);
EXTERN UWORD close_fi (UBYTE *);
EXTERN BOOLEAN create (UBYTE *, UBYTE *, WORD);
EXTERN UWORD ro_err (UBYTE *, WORD);

/* External variable definitions */

EXTERN UWORD ro_dsk; /* read-only disk vector */

/**********************************************************/
/*  First, some utility functions used by seqio and ranio */
/**********************************************************/

/******************************
 * FCB block number routines  *
 ******************************/

WORD
blkindx (fcbp)
/* return index into fcb disk map */
REG struct fcb *fcbp; /* pointer to fcb */
{
  REG struct dpb *dparmp; /* pointer to disk parameter block */
  REG WORD i;
  REG WORD blkshf;
  BSETUP

  dparmp = GBL.parmp;
  blkshf = dparmp->bsh;
  i = ((fcbp->extent) & dparmp->exm) << (7 - blkshf);
  return (i + (UBWORD (fcbp->cur_rec) >> blkshf));
}

UWORD
blknum (fcbp, index, wrdfcb)
/* return block number in fcb indicated by index */
REG struct fcb *fcbp; /* pointer to fcb                 */
REG WORD index;       /* index into disk map of fcb     */
WORD wrdfcb;          /* boolean, fcb disk map of words */
{
  if (wrdfcb)
    return (swap (fcbp->dskmap.big[index]));
  else
    return (UBWORD (fcbp->dskmap.small[index]));
}

setblk (fcbp, index, wrdfcb, block)
/* put block number into fcb */
REG struct fcb *fcbp; /* pointer to fcb                 */
REG WORD index;       /* index into disk map of fcb     */
WORD wrdfcb;          /* boolean, fcb disk map of words */
REG UWORD block;      /* block number                   */
{
  fcbp->s2 &= 0x7f; /* set file write flag */
  if (wrdfcb)
    fcbp->dskmap.big[index] = swap (block);
  else
    fcbp->dskmap.small[index] = (UBYTE)block;
}

/***************************
 * disk read/write routine *
 ***************************/

UWORD
do_io (block, rcrd, parm)
UWORD block;   /* block number    */
UBYTE rcrd;    /* record number   */
REG WORD parm; /* write parameter */
{
  REG LONG lsec;
  REG struct dpb *dparmp;
  BSETUP

  dparmp = GBL.parmp; /* init dpb pointer */
  lsec = ((LONG)block << (dparmp->bsh)) + (LONG)(rcrd & (dparmp->blm));
  return (rdwrt (lsec, GBL.dmaadr, parm));
}

/******************************************
 * routine for crossing extent boundaries *
 ******************************************/

WORD
new_ext (fcbp, reading, ran)
/* If sequential I/O, open the next extent                     */
/* If random I/O, compute new extent from random record field  */

REG struct fcb *fcbp; /* pointer to fcb  */
BOOLEAN reading;      /* read/write flag */
WORD ran;             /* random I/O flag */
{
  REG UBYTE mod;   /* module number   */
  REG UBYTE ext;   /* extent number   */
  REG UBYTE t_mod; /* temp mod number */
  REG UBYTE t_ext; /* temp extent     */
  BSETUP

  if (ran)
    {
      mod = ((fcbp->ran0) << 4) | ((fcbp->ran1) >> 4);
      ext = (((fcbp->ran1) & 0x0f) << 1);
      if ((fcbp->ran2) & 0x80)
        ext |= 1;
      /* the calculation of ext was coded this way because of a */
      /* compiler bug from Alcyon XXX */
    }
  else
    {
      mod = (fcbp->s2) & 0x3f;
      ext = (fcbp->extent) + 1; /* for sequential, incr extent  */
    }
  if (ext >= 32)
    {
      ext = 0;
      mod += 1;
    }
  if (mod >= 64)
    return (6); /* past maximum file size */
  if (mod == ((fcbp->s2) & 0x3f))
    if (!((ext ^ (fcbp->extent)) & ~((GBL.parmp)->exm) & 0x1f))
      { /* we're in same logical extent */
        fcbp->extent = ext;
        return (0);
      }
  /* Extent or Module numbers don't match */
  /* Close the old extent and open a  one */
  if (close_fi (fcbp) >= 255)
    return (3);
  /* can't close old extent */
  t_mod = fcbp->s2;
  t_ext = fcbp->extent;
  fcbp->s2 = mod;
  fcbp->extent = ext;
  if (dirscan (openfile, fcbp, 0) >= 255) /* open  extent */
    {
      if (reading)
        { /* reading unwritten extent */
          fcbp->s2 = t_mod;
          fcbp->extent = t_ext;
          return (4);
        }
      if (dirscan (create, fcbp, 8) >= 255)
        return (5); /* can't create new extent */
    }
  return (0);
}

/************************************
 * Routine to calculate the maximum *
 * extent number of an FCB in a     *
 * extent-folded environment        *
 ************************************/

UWORD
calcext (fcbp)
REG struct fcb *fcbp;
{
  REG UWORD i;
  REG BYTE *p;
  BSETUP

  i = 15;
  p = &(fcbp->dskmap.small[16]);
  do
    {
      if (*--p)
        break;
      i -= 1;
    }
  while (i);
  /* Now i contains the index of the last non-zero block in the FCB */
  if ((GBL.parmp)->dsm > 255)
    i >>= 1;
  i >>= 7 - ((GBL.parmp)->bsh);
  return (((fcbp->extent) & ~((GBL.parmp)->exm) & 0x1f) | i);
}

/**********************************
 * Routine to get the actual      *
 * record count of the currently  *
 * active logical extent of a FCB *
 **********************************/

UWORD
get_rc (fcbp)
REG struct fcb *fcbp;
{
  REG UWORD ext;

  ext = calcext (fcbp); /* find last active extent in fcb */
  if (ext == fcbp->extent)
    return (UBWORD (fcbp->rcdcnt));
  /* if this is the last active fcb, return fcb's rc */
  else if (ext > fcbp->extent)
    return (128);
  /* if the fcb has more extents past this one, then */
  /* the current one is logically full */
  else
    return (0);
  /* if we seeked past the last active extent, rc = 0 */
}

/************************
 * bdosrw entry point   *
 ************************/

UWORD
bdosrw (fcbp, reading, random)
REG struct fcb *fcbp; /* fcbp is a pointer to a fcb               */
REG BOOLEAN reading;  /* boolean to tell whether to read or write */
WORD random;          /* 0 = sequential, 1 = random (normal),     */
                      /* 2 = random with zero fill                */
{
  REG UWORD block;   /* block number from fcb        */
  REG WORD index;    /* index into disk map of fcb   */
  REG BYTE *old_dma; /* temp holding spot for dmaadr */
  REG WORD parm;     /* parameter to do-io           */
  REG WORD bigfile;  /* file system is in word mode  */
  REG UWORD rtn;     /* return parameter             */
  REG UBYTE rc;      /* temp storage for rcdcnt      */
  BSETUP

  bigfile = ((GBL.parmp)->dsm) & ~0xff;
  if ((!reading) && (fcbp->ftype[robit] & 0x80))
    ro_err (fcbp, (((GBL.dphp)->dpbp))->drm); /*** rli ***/
                                              /* check for read-only file */
  if (random)
    {
      if ((rtn = new_ext (fcbp, reading, TRUE)))
        return (rtn);
      /* open new extent if necessary, return if error */
      fcbp->cur_rec = (fcbp->ran2) & 0x7f;
    }
  else /* sequential */
    if (fcbp->cur_rec == (UBYTE)128)
      { /* time to try next extent */
        if (new_ext (fcbp, reading, FALSE))
          return (1);      /* if can't open new extent, error */
        fcbp->cur_rec = 0; /* opened new extent, zero cur_rec */
      }

  /* record is now in active fcb */
  rc = fcbp->rcdcnt;
  if (UBWORD (fcbp->cur_rec) >= get_rc (fcbp))
    {
      if (reading)
        return (1);     /* reading unwritten data */
      fcbp->s2 &= 0x7f; /* set file write flag    */
      rc = fcbp->cur_rec + 1;
    }
  index = blkindx (fcbp); /* get index into fcb disk map */
  block = blknum (fcbp, index, bigfile);
  if (block)
    parm = (reading ? 0 : 1);
  else /* if allocated block, parm is just read or write */
    { /* unallocated block    */
      if (reading)
        return (1); /* reading unwritten data */

      /* Writing to new block */
      /* The parm passed to getaloc is the previously allocated block */
      /*      or 0, if the previous block is not allocated            */

      block = getaloc (blknum (fcbp, (index ? (index - 1) : 0), bigfile));
      if (block == (UWORD)~0)
        return (2); /* out of space */
      setblk (fcbp, index, bigfile, block);
      parm = 3;
      if (random == 2)
        { /* Write random with zero fill  */
          old_dma = GBL.dmaadr;
          GBL.dmaadr = (void *)GBL.dirbufp; /* Do DMA from dir_buf */
          index = SECLEN;
          do
            GBL.dmaadr[--index] = 0;
          while (index); /* zero the dma buffer */
          for (index = 0; index <= ((GBL.parmp)->blm); index++)
            {
              do_io (block, (UBYTE)index, parm);
              /* write zeros to the block */
              parm = 1; /* next write is not to new block */
            }
          GBL.dmaadr = old_dma; /* restore dma address  */
        }
    }
  rtn = do_io (block, fcbp->cur_rec, parm);
  if (rtn == 0)
    {
      fcbp->rcdcnt = rc;
      if (!random)
        fcbp->cur_rec += 1;
    }
  return (rtn);
}
