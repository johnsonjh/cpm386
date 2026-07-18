/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: aba1b7e4-82b4-11f1-93ac-80ee73e9b8e7
 */

/*****************************************************************************/

#ifdef RLI
#include "diverge.h"
#endif

/*************************************************************
 *                                                           *
 *         CP/M-68K BDOS File I/O Module                     *
 *                                                           *
 * This module contains all file handling BDOS functions     *
 * except for read and write for CP/M-68K.  Included are:    *
 *                                                           *
 *         seldsk()      - select disk                       *
 *         openfile()    - open file                         *
 *         close_fi()    - close file                        *
 *         search()      - search for first/next file match  *
 *         create()      - create file                       *
 *         delete()      - delete file                       *
 *         rename()      - rename file                       *
 *         set_attr()    - set file attributes               *
 *         getsize()     - get file size                     *
 *         setran()      - set random record field           *
 *         truncate_fi() - truncate file (BDOS 99)           *
 *         f_parse_fn()  - parse ASCII filename (BDOS 152)   *
 *         free_sp()     - get disk free space               *
 *         move()        - general purpose byte mover        *
 *                                                           *
 *                                                           *
 * Compiled with Alcyon C on the VAX                         *
 *                                                           *
 * Modified 2/5/84 sw Allow odd DMA on get free space        *
 *                                                           *
 *************************************************************/

#include "bdosinc.h"            /* Standard I/O declarations */

#include "bdosdef.h"            /* Type and structure declarations for BDOS */

#include "pktio.h"              /* Packet I/O definitions */

/* declare external fucntions (updated for gcc arg checking) */

/* dirscan: bdosdef.h: UWORD dirscan(DIRSCAN_FN, UBYTE *, UWORD) */
EXTERN UWORD    error(WORD);
EXTERN UWORD    ro_err(UBYTE *, WORD);
EXTERN UWORD    do_phio(void *);
EXTERN void     clraloc(WORD);
EXTERN void     setaloc(WORD);
EXTERN WORD     swap(WORD);
EXTERN UWORD    dir_wr(WORD);
EXTERN void     tmp_sel(UBYTE *);
EXTERN UWORD    calcext(UBYTE *);
EXTERN UWORD    udiv(LONG, UWORD, UWORD *);

/* declare external variables */
EXTERN UWORD    log_dsk;        /* logged-on disk vector        */
EXTERN UWORD    ro_dsk;         /* read-only disk vector        */
EXTERN UWORD    crit_dsk;       /* vector of disks in critical state    */


/************************************
*  This function passed to dirscan  *
*       from seldsk (below)         *
************************************/

BOOLEAN alloc(fcbp, dirp, dirindx)
/* Set up allocation vector for directory entry pointed to by dirp */

struct fcb      *fcbp;          /* not used in this function    */
REG struct dirent *dirp;        /* pointer to directory entry   */
WORD            dirindx;        /* index into directory for *dirp */
{
    REG WORD    i;              /* loop counter */
    BSETUP

    if ( UBWORD(dirp->entry) < 0x10 )   /* skip MP/M 2.x and CP/M 3.x XFCBs */
    {
        (GBL.dphp)->hiwater = dirindx;  /* set up high water mark for disk */
        i = 0;
        if ((GBL.parmp)->dsm < 256)
        {
            do setaloc( UBWORD(dirp->dskmap.small[i++]) );
                while (i <= 15);
        }
        else
        {
            do setaloc(swap(dirp->dskmap.big[i++]));
                while (i <= 7);
        }
    }
}


/************************
*  seldsk entry point   *
************************/

seldsk(dsknum)

REG UBYTE dsknum;               /* disk number to select */

{
    struct iopb selpkt;
    REG WORD    i;
    UWORD       j;
    REG UBYTE   logflag;
    BSETUP

    logflag = ~(log_dsk >> dsknum) & 1;
    if ((GBL.curdsk != dsknum) || logflag)
    {                           /* if not last used disk or not logged on */
        selpkt.iofcn = sel_info;
        GBL.curdsk = (selpkt.devnum = dsknum);
        if (UBWORD(dsknum) > 15) error(2);
        selpkt.ioflags = logflag ^ 1;
        do
        {
            do_phio(&selpkt);   /* actually do the disk select  */
            if ( (void *)(GBL.dphp = selpkt.infop) != NULL ) break;
        } while ( ! error(3) );

        GBL.dirbufp = (void *)((GBL.dphp)->dbufp);
                        /* set up GBL copies of dir_buf and dpb ptrs */
        GBL.parmp = (GBL.dphp)->dpbp;
    }
    if (logflag)
    {           /* if disk not previously logged on, do it now */
        LOCK    /* must lock the file system while messing with alloc vec */
        i = (GBL.parmp)->dsm;
        do clraloc(i); while (i--);     /* clear the allocation vector */
        i = udiv( (LONG)(((GBL.parmp)->drm) + 1),
                  4 * (((GBL.parmp)->blm) + 1), &j);
                                        /* calculate nmbr of directory blks */
        if (j) i++;                     /* round up */
        do setaloc(--i); while (i);     /* alloc directory blocks */
        dirscan(alloc, NULL, 0x0e);     /* do directory scan & alloc blocks */
        log_dsk |= 1 << dsknum;         /* mark disk as logged in       */
    }
}


/*******************************
*  General purpose byte mover  *
*******************************/

move(p1, p2, i)

REG BYTE *p1;
REG BYTE *p2;
REG WORD  i;
{
    while (i--)
        *p2++ = *p1++;
}


/*************************************
*  General purpose filename matcher  *
*************************************/

BOOLEAN match(p1, p2, chk_ext)

REG UBYTE *p1;
REG UBYTE *p2;
BOOLEAN  chk_ext;
{
    REG WORD    i;
    REG UBYTE temp;
    BSETUP

    i = 12;
    do
    {
        temp = (*p1 ^ '?');
        if ( ((*p1++ ^ *p2++) & 0x7f) && temp )
            return(FALSE);
        i -= 1;
    } while (i);
    if (chk_ext)
    {
        if ( (*p1 != '?') && ((*p1 ^ *p2) & ~((GBL.parmp)->exm)) )
            return(FALSE);
        p1 += 2;
        p2 += 2;
        if ((*p1 ^ *p2) & 0x3f) return(FALSE);
    }
    return(TRUE);
}


/************************
*  openfile entry point *
************************/

BOOLEAN openfile(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to open */
struct dirent  *dirp;           /* pointer to directory entry   */
WORD    dirindx;

{
    REG UBYTE fcb_ext;          /* extent field from fcb        */
    REG UBYTE want_lrbc;        /* FCB+32 was 0xFF: return LRBC */
    REG BOOLEAN rtn;
    BSETUP

    if ( (rtn = match(fcbp, dirp, TRUE)) )
    {
        /* CP/M Plus / DOS Plus: if cur_rec (FCB+32) is 0xFF on entry,
         * return the Last Record Byte Count (dir s1) in that byte.
         * Caller must zero cur_rec before sequential I/O.
         * For multi-extent files the real LRBC lives on the *last*
         * extent; open always matches extent 0 first, so openfile only
         * seeds cur_rec/s1 - file_last_lrbc() (called from BDOS 15)
         * then rewrites them from the highest extent. */
        want_lrbc = (fcbp->cur_rec == (UBYTE)0xFF);
        fcb_ext = fcbp->extent;  /* save extent number from user's fcb */
        move(dirp, fcbp, sizeof *dirp);
                                /* copy dir entry into user's fcb  */
        fcbp->extent = fcb_ext;
        fcbp->s2 |= 0x80;        /* set hi bit of S2 (write flag)       */
        if (want_lrbc)
            fcbp->cur_rec = fcbp->s1; /* provisional; fixed by file_last_lrbc */
        crit_dsk |= 1 << (GBL.curdsk);
    }
   return(rtn);
}


/* State for file_last_lrbc directory scan (not re-entrant). */
static UBYTE lrbc_best_s1;
static UBYTE lrbc_best_ex;
static UBYTE lrbc_best_s2;
static UBYTE lrbc_found;

/* dirscan callback: track s1 of the highest extent for this filename. */
static BOOLEAN lrbc_scan(fcbp, dirp, dirindx)
UBYTE *fcbp;
UBYTE *dirp;
WORD dirindx;
{
    REG struct fcb *f = (struct fcb *)fcbp;
    REG struct dirent *d = (struct dirent *)dirp;
    UBYTE ex, s2;

    (void)dirindx;
    if (UBWORD(d->entry) >= 0x10)
        return(FALSE);
    if (!match(fcbp, dirp, FALSE))
        return(FALSE); /* name only; all extents */

    ex = (UBYTE)(d->extent & 0x1f);
    s2 = (UBYTE)(d->s2 & 0x3f);

    if (!lrbc_found
        || s2 > lrbc_best_s2
        || (s2 == lrbc_best_s2 && ex >= lrbc_best_ex))
    {
        lrbc_found = 1;
        lrbc_best_ex = ex;
        lrbc_best_s2 = s2;
        lrbc_best_s1 = d->s1;
    }

    (void)f;
    return(FALSE); /* never stop - need every extent */
}


/******************************
*  file_last_lrbc entry point *
******************************/
/* After a successful open with FCB+32=0xFF, set s1 and cur_rec to the
 * DOS-PLUS Last Record Byte Count from the file's last directory extent.
 * Does not alter the open extent's allocation map (extent 0 remains open). */

void file_last_lrbc(fcbp)
REG struct fcb *fcbp;
{
    lrbc_found = 0;
    lrbc_best_s1 = fcbp->s1;
    lrbc_best_ex = 0;
    lrbc_best_s2 = 0;
    /* Scan whole directory; callback always returns false so parms=0 is fine */
    (void)dirscan(lrbc_scan, (UBYTE *)fcbp, 0);
    if (lrbc_found) {
        fcbp->s1 = lrbc_best_s1;
        fcbp->cur_rec = lrbc_best_s1;
    }
}


/*************************/
/* flush buffers routine */
/*************************/

UWORD flushit()
{
    REG UWORD   rtn;            /* return code from flush buffers call */
    struct iopb flushpkt;       /* I/O packet for flush buffers call */

    flushpkt.iofcn = flush;
    while ( (rtn = do_phio(&flushpkt)) )
        if ( error(1) ) break;
    return(rtn);
}


/*********************************
* file close routine for dirscan *
*********************************/

BOOLEAN close(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb */
REG struct dirent *dirp;        /* pointer to directory entry */
WORD    dirindx;                /* index into directory */

{
    REG WORD  i;
    REG UBYTE *fp;
    REG UBYTE *dp;
    REG UWORD fcb_ext;
    REG UWORD dir_ext;
    BSETUP

    if ( match(fcbp, dirp, TRUE) )
    {                   /* Note that FCB merging is done here as a final
                           confirmation that disks haven't been swapped */
        LOCK
        fp = &(fcbp->dskmap.small[0]);
        dp = &(dirp->dskmap.small[0]);
        if ((GBL.parmp)->dsm < 256)
        {               /* Small disk map merge routine  */
            i = 16;
            do
            {
                if (*dp)
                {
                    if (*fp)
                    {
                        if (*dp != *fp) goto badmerge;
                    }
                    else *fp = *dp;
                }
                else *dp = *fp;
                fp += 1;
                dp += 1;
                i -= 1;
            } while (i);
        }
        else
        {               /* Large disk map merge routine (word block numbers).
                         * Advance pointers by 2 on byte-addressed hosts; the
                         * original Alcyon 68K source had a broken *fp+=1. */
            i = 8;
            do
            {
                if (*(UWORD *)dp)
                {
                    if (*(UWORD *)fp)
                    {
                        if (*(UWORD *)dp != *(UWORD *)fp) goto badmerge;
                    }
                    else *(UWORD *)fp = *(UWORD *)dp;
                }
                else *(UWORD *)dp = *(UWORD *)fp;
                fp += 2;
                dp += 2;
                i -= 1;
            } while (i);
        }
        /* Disk map merging complete */
        fcb_ext = calcext(fcbp);        /* calc max extent for fcb */
        dir_ext = (UWORD)(dirp->extent) & 0x1f;
        if ( (fcb_ext > dir_ext) ||
            ((fcb_ext == dir_ext) &&
                (UBWORD(fcbp->rcdcnt) > UBWORD(dirp->rcdcnt))) )
                        /* if fcb points to larger file than dirp */
        {
            dirp->rcdcnt = fcbp->rcdcnt;        /* set up rc, ext from fcb */
            dirp->extent = (BYTE)fcb_ext;
        }
        dirp->s1 = fcbp->s1;
        if ( (dirp->ftype[robit]) & 0x80) ro_err(fcbp,dirindx);
                                                /* read-only file error */
        dirp->ftype[arbit] &= 0x7f;             /* clear archive bit        */
        dir_wr(dirindx >> 2);
        UNLOCK
        return(TRUE);

badmerge:
        UNLOCK
        ro_dsk |= (1 << GBL.curdsk);
        return(FALSE);
    }
    else return(FALSE);
}


/************************
*  close_fi entry point *
************************/

UWORD close_fi(fcbp)

struct fcb *fcbp;               /* pointer to fcb for file to close */
{
    flushit();                          /* first, flush the buffers     */
    if ((fcbp->s2) & 0x80) return(0);   /* if file write flag not on,
                                           don't need to do physical close */
    return( dirscan(close, fcbp, 0));   /* call dirscan with close function */
}


/************************
*  search entry point   *
************************/

/* First two functions for dirscan */

BOOLEAN alltrue(p1, p2, i)
UBYTE   *p1;
UBYTE   *p2;
WORD    i;
{
    return(TRUE);
}

BOOLEAN matchit(p1, p2, i)
UBYTE   *p1;
UBYTE   *p2;
WORD    i;
{
    return(match(p1, p2, TRUE));
}


/* search entry point */
UWORD search(fcbp, dsparm, p)

REG struct fcb *fcbp;           /* pointer to fcb for file to search */
REG UWORD dsparm;               /* parameter to pass through to dirscan */
UBYTE   *p;                     /* pointer to pass through to tmp_sel   */

{
    REG UWORD   rtn;            /* return value */
    BSETUP

    if (fcbp->drvcode == '?')
    {
        seldsk(GBL.dfltdsk);
        rtn = dirscan(alltrue, fcbp, dsparm);
    }
    else
    {
        tmp_sel(p);             /* temporarily select disk */
        if (fcbp->extent != '?') fcbp->extent = 0;
        fcbp->s2 = 0;
        rtn = dirscan(matchit, fcbp, dsparm);
    }
    move( GBL.dirbufp, GBL.dmaadr, SECLEN);
    return(rtn);
}


/************************
*  create entry point   *
************************/

BOOLEAN create(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to create */
REG struct dirent *dirp;        /* pointer to directory entry   */
REG WORD dirindx;               /* index into directory         */

{
    REG WORD i;
    REG BOOLEAN rtn;
    BSETUP

    if ( (rtn = ((dirp->entry) == (UBYTE)0xe5)) )
    {
        fcbp->rcdcnt = 0; /* clear fcb rcdcnt */

        for (i = 0; i < 16; i++) /* clear disk map */
          fcbp->dskmap.small[i] = 0;

        move(fcbp, dirp, sizeof *dirp); /* move the fcb to the directory */
        dir_wr(dirindx >> 2);           /* write the directory sector */

        if ( dirindx > (GBL.dphp)->hiwater )
            (GBL.dphp)->hiwater = dirindx;
        crit_dsk |= 1 << (GBL.curdsk);
    }
    return(rtn);
}


/************************
*  delete entry point   *
************************/

BOOLEAN delete(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to delete */
REG struct dirent *dirp;        /* pointer to directory entry   */
REG WORD dirindx;               /* index into directory         */

{
    REG WORD i;
    REG BOOLEAN rtn;
    BSETUP

    if ( (rtn = match(fcbp, dirp, FALSE)) )
    {
        if ( (dirp->ftype[robit]) & 0x80 ) ro_err(fcbp,dirindx);
                                /* check for read-only file */
        dirp->entry = 0xe5;
        LOCK
        dir_wr(dirindx >> 2);
        /* Now free up the space in the allocation vector */
        if ((GBL.parmp)->dsm < 256)
        {
            i = 16;
            do clraloc(UBWORD(dirp->dskmap.small[--i]));
                while (i);
        }
        else
        {
            i = 8;
            do clraloc(swap(dirp->dskmap.big[--i]));
                while (i);
        }
        UNLOCK
    }
    return(rtn);
}


/************************
*  rename entry point   *
************************/

BOOLEAN rename(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to delete */
REG struct dirent *dirp;        /* pointer to directory entry   */
REG WORD dirindx;               /* index into directory         */

{
    REG UWORD i;
    REG BYTE *p;                /* general purpose pointers */
    REG BYTE *q;
    REG BOOLEAN rtn;
    BSETUP

    if ( (rtn = match(fcbp, dirp, FALSE)) )
    {
        if ( (dirp->ftype[robit]) & 0x80 ) ro_err(fcbp,dirindx);
                                /* check for read-only file */
        p = &(fcbp->dskmap.small[1]);
        q = &(dirp->fname[0]);
        i = 11;
        do
        {
            *q++ = *p++ & 0x7f;
            i -= 1;
        } while (i);
        dir_wr(dirindx >> 2);
    }
    return(rtn);
}


/************************
*  set_attr entry point *
************************/

BOOLEAN set_attr(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to delete */
REG struct dirent *dirp;        /* pointer to directory entry   */
REG WORD dirindx;               /* index into directory         */

{
    REG BOOLEAN rtn;
    BSETUP

    if ( (rtn = match(fcbp, dirp, FALSE)) )
    {
        /* Copy 11 name/type bytes including attribute high bits (F1'-T3'). */
        move(&fcbp->fname[0], &dirp->fname[0], 11);
        /* CP/M Plus / DOS Plus Last Record Byte Count:
         * If F6' is set (bit 7 of FCB+6 / fname[5]), FCB+32 (cur_rec)
         * holds the new LRBC (DOS-PLUS: bytes used in last record, 0=full).
         * Write it to directory entry offset 13 (s1). */
        if (fcbp->fname[5] & 0x80)
            dirp->s1 = fcbp->cur_rec;
        dir_wr(dirindx >> 2);
    }
    return(rtn);
}


/****************************
*  utility routine used by  *
*  setran and getsize       *
****************************/

LONG extsize(fcbp)
/* Return size of extent pointed to by fcbp */
REG struct fcb *fcbp;

{
    return( ((LONG)(fcbp->extent & 0x1f) << 7)
                | ((LONG)(fcbp->s2 & 0x3f) << 12) );
}


/************************
*  setran entry point   *
************************/

/* Pack a 24-bit record number into FCB ran0..ran2 in CP/M-68K order:
 * ran0 = bits 16..23, ran1 = bits 8..15, ran2 = bits 0..7 (MSB first).
 * Portable across LE/BE - do not use host-endian unions. */
static void pack_ranrec(struct fcb *fcbp, LONG n)
{
    fcbp->ran0 = (UBYTE)((n >> 16) & 0xff);
    fcbp->ran1 = (UBYTE)((n >> 8) & 0xff);
    fcbp->ran2 = (UBYTE)(n & 0xff);
}

static LONG unpack_ranrec(REG struct fcb *fcbp)
{
    return ((LONG)(fcbp->ran0 & 0xff) << 16)
         | ((LONG)(fcbp->ran1 & 0xff) << 8)
         |  (LONG)(fcbp->ran2 & 0xff);
}

setran(fcbp)

REG struct fcb *fcbp;           /* pointer to fcb for file to set ran rec */

{
    LONG random;

    random = (LONG)UBWORD(fcbp->cur_rec) + extsize(fcbp);
                                /* compute random record field  */
    pack_ranrec(fcbp, random);
}


/**********************************/
/* fsize is a funtion for dirscan */
/* passed from getsize            */
/**********************************/

BOOLEAN fsize(fcbp, dirp, dirindx)

REG struct fcb *fcbp;           /* pointer to fcb for file to delete */
REG struct dirent *dirp;        /* pointer to directory entry   */
WORD dirindx;                   /* index into directory         */

{
    REG BOOLEAN rtn;
    LONG nrecs;

    if ( (rtn = match(fcbp, dirp, FALSE)) )
    {
        /* next-record count = records used in this extent + extent base */
        nrecs = (LONG)UBWORD(dirp->rcdcnt) + extsize(dirp);
        pack_ranrec(fcbp, nrecs);
    }
    return(rtn);
}

/************************
*  getsize entry point  *
************************/

getsize(fcbp)
/* get file size        */
REG struct fcb *fcbp;           /* pointer to fcb to get file size for */

{
    REG WORD dsparm;
    LONG maxrcd, temp;

    maxrcd = 0;
    dsparm = 0;
    while ( dirscan(fsize, fcbp, dsparm) < 255 )
    {                           /* loop until no more matches */
        temp = unpack_ranrec(fcbp);
        if (temp > maxrcd) maxrcd = temp;
        dsparm = 1;
    }
    pack_ranrec(fcbp, maxrcd);
}


/* ---- BDOS 99 (F_TRUNCATE) support ---- */
static LONG  trunc_nrecs;       /* desired size in records (not exceed) */
static UWORD trunc_found;       /* saw at least one matching dirent */
static UWORD trunc_code;        /* dir index code 0-3 for success return */

/* Free all allocation blocks in a directory entry's map. */
static void trunc_free_all(dirp)
REG struct dirent *dirp;
{
    REG WORD i;
    BSETUP

    if ((GBL.parmp)->dsm < 256) {
        i = 16;
        do clraloc(UBWORD(dirp->dskmap.small[--i]));
            while (i);
    } else {
        i = 8;
        do clraloc(swap(dirp->dskmap.big[--i]));
            while (i);
    }
}

/*
 * Free map slots whose first covered record is >= trunc_nrecs.
 * Handles extent-folded FCBs (EXM): slot layout matches blkindx().
 */
static void trunc_free_past(dirp, group_base)
REG struct dirent *dirp;
LONG group_base;
{
    REG UWORD i, maxslots, shift, bsh, k, off;
    REG LONG abs_start;
    REG UWORD blk;
    BSETUP

    bsh = (GBL.parmp)->bsh;
    shift = (UWORD)(7 - bsh);
    maxslots = ((GBL.parmp)->dsm < 256) ? 16 : 8;

    for (i = 0; i < maxslots; i++) {
        k = i >> shift;
        off = (UWORD)((i & ((1u << shift) - 1)) << bsh);
        abs_start = group_base + ((LONG)k << 7) + (LONG)off;
        if (abs_start < trunc_nrecs)
            continue;
        if ((GBL.parmp)->dsm < 256) {
            blk = UBWORD(dirp->dskmap.small[i]);
            if (blk)
                clraloc(blk);
            dirp->dskmap.small[i] = 0;
        } else {
            blk = (UWORD)swap(dirp->dskmap.big[i]);
            if (blk)
                clraloc(blk);
            dirp->dskmap.big[i] = 0;
        }
    }
}

/* First record number covered by this physical dirent (extent fold base). */
static LONG trunc_group_base(dirp)
REG struct dirent *dirp;
{
    REG UBYTE first_ex;
    BSETUP

    first_ex = (UBYTE)(dirp->extent & ~((GBL.parmp)->exm) & 0x1f);
    return ((LONG)(dirp->s2 & 0x3f) << 12) | ((LONG)first_ex << 7);
}

/* End record (exclusive) for data in this dirent - matches fsize()/getsize. */
static LONG trunc_group_end(dirp)
REG struct dirent *dirp;
{
    return extsize(dirp) + (LONG)UBWORD(dirp->rcdcnt);
}

BOOLEAN truncate_one(fcbp, dirp, dirindx)

UBYTE *fcbp;
UBYTE *dirp;
WORD dirindx;

{
    REG struct fcb *f = (struct fcb *)fcbp;
    REG struct dirent *d = (struct dirent *)dirp;
    REG LONG gbase, gend, last;
    BSETUP

    if (UBWORD(d->entry) >= 0x10)
        return(FALSE);

    if (!match(fcbp, dirp, FALSE))
        return(FALSE);

    if ((d->ftype[robit]) & 0x80)
        ro_err(fcbp, dirindx);

    trunc_found = 1;
    gbase = trunc_group_base(d);
    gend  = trunc_group_end(d);

    if (gbase >= trunc_nrecs) {
        /* Whole physical extent past the new EOF - delete it. */
        LOCK
        trunc_free_all(d);
        d->entry = 0xe5;
        dir_wr(dirindx >> 2);
        UNLOCK

        return(FALSE); /* keep scanning */
    }

    if (gend > trunc_nrecs) {
        /* Partial: keep [gbase, trunc_nrecs), free later blocks. */
        LOCK
        trunc_free_past(d, gbase);

        if (trunc_nrecs == 0) {
            d->extent = 0;
            d->s2 = (UBYTE)(d->s2 & 0xc0);
            d->rcdcnt = 0;
        } else {
            last = trunc_nrecs - 1;
            d->s2 = (UBYTE)((d->s2 & 0xc0) | ((last >> 12) & 0x3f));
            d->extent = (UBYTE)((last >> 7) & 0x1f);
            d->rcdcnt = (UBYTE)((last & 127) + 1);
        }

        d->s1 = 0; /* record-granular truncate clears LRBC */
        dir_wr(dirindx >> 2);
        UNLOCK
        trunc_code = (UWORD)(dirindx & 3);
    }

    (void)f;

    return(FALSE); /* always continue - all extents */
}


/****************************
*  truncate_fi entry point  *
****************************/
/* BDOS 99 (F_TRUNCATE): ran0..ran2 = new size in records (cannot extend).
 * Returns 0-3 on success (dir code), 255 if not found / would extend / error. */

UWORD truncate_fi(fcbp)
REG struct fcb *fcbp;
{
    LONG want, cur;
    struct fcb tmp;
    REG WORD i;
    BSETUP

    want = unpack_ranrec(fcbp);

    /* Current size via getsize (does not destroy caller's name fields). */
    for (i = 0; i < (WORD)(sizeof tmp); i++)
        ((UBYTE *)&tmp)[i] = ((UBYTE *)fcbp)[i];
    getsize((UBYTE *)&tmp);
    cur = unpack_ranrec(&tmp);

    if (want > cur)
        return(255); /* cannot extend */
    if (want == cur)
        return(0);   /* no-op success */

    trunc_nrecs = want;
    trunc_found = 0;
    trunc_code = 0;

    /* Full directory scan: process every matching extent */
    (void)dirscan(truncate_one, (UBYTE *)fcbp, 2);

    if (!trunc_found)
        return(255);
    crit_dsk |= 1 << (GBL.curdsk);

    return(trunc_code);
}


/************************
*  free_sp entry point  *
************************/

free_sp(dsknum)

UBYTE dsknum;           /* disk number to get free space of */
{
    REG LONG records;
    REG UWORD   *alvec;
    REG UWORD   bitmask;
    REG UWORD   alvword;
    REG WORD    i;
        LONG    temp;           /*sw For DMA Odd problem        */
    BSETUP

    seldsk(dsknum);             /* select the disk */
    records = (LONG)0;          /* initialize the variables */
    alvec = (void *)(GBL.dphp)->alv;
    bitmask = 0;
    for (i = 0; i <= (GBL.parmp)->dsm; i++)     /* for loop to compute */
    {
        if ( ! bitmask)
        {
            bitmask = 0x8000;
            alvword = ~(*alvec++);
        }
        if ( alvword & bitmask)
            records += (LONG)( ((GBL.parmp)->blm) + 1 );
        bitmask >>= 1;
    }
    temp = records;                      /*sw Put in memory              */
    move(&temp,GBL.dmaadr,sizeof(LONG)); /*sw Move to user's DMA         */
}


/******************************
*  BDOS 152 (F_PARSE) helpers *
******************************/

/* Field / end terminators per seasip (excl. '.' which starts type,
 * and ':' which may mark a drive after a letter). */
static int parse_is_end(int c)
{
    switch (c) {
    case ' ': case '\t': case '\r': case '\0':
    case ';': case '=': case '>': case '<':
    case ',': case '[': case ']': case '/': case '|':
        return 1;
    default:
        return 0;
    }
}

static int parse_is_name_char(int c)
{
    if (c == '?' || c == '*')
        return 1;
    if (c >= '0' && c <= '9')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= 'a' && c <= 'z')
        return 1;
    /* common extras allowed in some parsers */
    if (c == '-' || c == '_' || c == '!')
        return 1;
    return 0;
}

static UBYTE parse_up(int c)
{
    if (c >= 'a' && c <= 'z')
        return (UBYTE)(c - 32);
    return (UBYTE)c;
}

/*
 * Pure parser: ASCII filename -> 36-byte FCB.
 * Returns (CP/M F_PARSE style, relative to src):
 *   0xFFFF  invalid
 *   0       ended on NUL or CR
 *   else    byte offset from src to the next character after the filename
 *
 * Accepts: [d:]filename[.typ][;password]
 * Password stored at FCB+0x10 (8 bytes), length at FCB+0x1A.
 * Wildcards * and ? supported (* fills rest of field with ?).
 */
UWORD f_parse_fn(const UBYTE *src, UBYTE *fcb)
{
    const UBYTE *p = src;
    int i, plen;

    if (!src || !fcb)
        return 0xFFFF;

    for (i = 0; i < 36; i++)
        fcb[i] = 0;
    for (i = 1; i <= 11; i++)
        fcb[i] = ' ';

    while (*p == ' ' || *p == '\t')
        p++;

    /* Optional drive d: */
    if (p[0] && p[1] == ':') {
        UBYTE d = parse_up(p[0]);
        if (d < 'A' || d > 'P')
            return 0xFFFF;
        fcb[0] = (UBYTE)(d - 'A' + 1);
        p += 2;
    }

    /* Filename (8) */
    for (i = 0; i < 8; ) {
        if (*p == '*') {
            while (i < 8)
                fcb[1 + i++] = '?';
            p++;
            break;
        }
        if (*p == '.' || *p == ':' || parse_is_end(*p))
            break;
        if (!parse_is_name_char(*p))
            return 0xFFFF;
        fcb[1 + i++] = parse_up(*p++);
    }
    /* skip excess name chars before type/end */
    while (*p && *p != '.' && *p != ';' && *p != ':' && !parse_is_end(*p)) {
        if (!parse_is_name_char(*p) && *p != '*')
            return 0xFFFF;
        p++;
    }

    /* Type (.typ) */
    if (*p == '.') {
        p++;
        for (i = 0; i < 3; ) {
            if (*p == '*') {
                while (i < 3)
                    fcb[9 + i++] = '?';
                p++;
                break;
            }
            if (*p == '.' || *p == ';' || *p == ':' || parse_is_end(*p))
                break;
            if (!parse_is_name_char(*p))
                return 0xFFFF;
            fcb[9 + i++] = parse_up(*p++);
        }
        while (*p && *p != ';' && *p != ':' && !parse_is_end(*p)) {
            if (*p == '.')
                break;
            if (!parse_is_name_char(*p) && *p != '*')
                return 0xFFFF;
            p++;
        }
    }

    /* Password (;password) -> FCB+16, length FCB+26 */
    if (*p == ';') {
        p++;
        plen = 0;
        while (plen < 8 && *p && !parse_is_end(*p) && *p != ';' && *p != ':' && *p != '.') {
            fcb[0x10 + plen] = parse_up(*p++);
            plen++;
        }
        fcb[0x1A] = (UBYTE)plen;
        while (*p && !parse_is_end(*p) && *p != ':' && *p != '.')
            p++;
    }

    if (*p == '\0' || *p == '\r')
        return 0;
    /* Offset of next character from start of string (fits classic HL return) */
    return (UWORD)(p - src);
}

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
