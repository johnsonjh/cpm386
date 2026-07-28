/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 8d61c422-82b4-11f1-8327-80ee73e9b8e7
 */

/*****************************************************************************/

#ifdef RLI
# include "diverge.h"
#endif

/*****************************************************************************/

/****************************************************************
*                                                               *
*               CP/M-68K BDOS Disk Utilities Module             *
*                                                               *
*       This module contains the miscellaneous utilities        *
*       for manipulating the disk in CP/M-68K.  Included are:   *
*                                                               *
*               dirscan()   - general purpose dir scanning      *
*               setaloc()   - set bit in allocation vector      *
*               clraloc()   - clear bit in allocation vector    *
*               getaloc()   - get free allocation block         *
*               dchksum()   - directory checksum calculator     *
*               dir_rd()    - read directory sector             *
*               dir_wr()    - write directory sector            *
*               rdwrt()     - read/write disk sector            *
*                                                               *
*                                                               *
*       Configured for Alcyon C on the VAX                      *
*                                                               *
****************************************************************/

/*****************************************************************************/

#include "bdosinc.h"            /* Standard I/O declarations */

/*****************************************************************************/

#include "bdosdef.h"            /* Type and structure declarations for BDOS */

/*****************************************************************************/

#include "pktio.h"              /* Packet I/O definitions */

/*****************************************************************************/

/* declare external functions and variables */
EXTERN UWORD    do_phio(void *); /* external physical disk I/O routine */
EXTERN UWORD    error(WORD);     /* external error routine             */

/*****************************************************************************/

EXTERN UWORD    log_dsk;  /* logged-on disk vector */
EXTERN UWORD    ro_dsk;   /* read-only disk vector */
EXTERN UWORD    crit_dsk; /* critical disk vector  */
UBYTE dchksum();

/*****************************************************************************/

/**********************
* read/write routine  *
**********************/

UWORD rdwrt(secnum, dma, parm)
/* General disk sector read/write routine */
/* It simply sets up a I/O packet and sends it to do_phio */
LONG    secnum;                 /* logical sector number to read/write */
UBYTE   *dma;                   /* dma address                          */
REG WORD parm;                  /* 0 for read, write parm + 1 for write */
{
    struct iopb rwpkt;
    BSETUP

    rwpkt.devnum = GBL.curdsk;          /* disk to read/write   */

    if (parm)
    {
        rwpkt.iofcn = (BYTE)write; /* if parm non-zero, we're doing a write */
        rwpkt.ioflags = (BYTE)(parm-1); /* pass write parm      */
        if ( ro_dsk & (1 << (rwpkt.devnum)) ) error(4);
                                /* don't write on read-only disk        */
    }
    else
    {
        rwpkt.iofcn = (BYTE)read;
        rwpkt.ioflags = (BYTE)0;
    }

    rwpkt.devadr = secnum;                      /* sector number        */
    rwpkt.xferadr = dma;                        /* dma address          */

/*              parameters that are currently not used by do_phio
    rwpkt.devtype = disk;
    rwpkt.xferlen = 1;
                                */
    rwpkt.infop = GBL.dphp;                     /* pass ptr to dph      */

    while ( do_phio(&rwpkt) )
        if ( error( parm ? 1 : 0 ) ) break;

    return(0);
}

/*****************************************************************************/

/***************************
*  directory read routine  *
***************************/

UWORD dir_rd(secnum)
WORD secnum;
{
    BSETUP

    return( rdwrt((LONG)secnum, GBL.dirbufp, 0) );
}

/*****************************************************************************/

/****************************
*  directory write routine  *
****************************/

UWORD dir_wr(secnum)
REG WORD secnum;
{
    REG UWORD rtn;
    BSETUP

    rtn = rdwrt( (LONG)secnum, GBL.dirbufp, 2);

    if ( secnum < (GBL.parmp)->cks )
        *((GBL.dphp)->csv + secnum) = dchksum();

    return(rtn);
}

/*****************************************************************************/

/*******************************
*  directory checksum routine  *
*******************************/

UBYTE dchksum()
/* Compute checksum over one directory sector */
/* Note that this implementation is dependent on the representation */
/*   of a LONG and is therefore not very portable.  But it's fast   */
{
    REG LONG    *p;             /* local temp variables */
    REG LONG    lsum;
    REG WORD    i;

    BSETUP

    p = (void *)GBL.dirbufp;            /* point to directory buffer */
    lsum = 0;
    i = SECLEN / (sizeof lsum);

    do
    {
        lsum += *p++;           /* add next 4 bytes of directory */
        i -= 1;
    } while (i);

    lsum += (lsum >> 16);
    lsum += (lsum >> 8);

    return( (UBYTE)(lsum & 0xff) );
}

/*****************************************************************************/

/************************
*  dirscan entry point  *
************************/

UWORD dirscan(funcp, fcbp, parms)
DIRSCAN_FN funcp; /* pointer to Boolean match/action function */
UBYTE *fcbp;      /* pointer to FCB (often struct fcb *)      */
REG UWORD parms;  /* parms is 16 bit set of bit parameters    */

/* Parms & 1  = 0 to start at beginning of dir, 1 to continue from last */
/* Parms & 2  = 0 to stop when *funcp is true, 1 to go until end        */
/* Parms & 4  = 0 to check the dir checksum, 1 to store new checksum    */
/* Parms & 8  = 0 to stop at hiwater, 1 to go until end of directory    */

#define continue 1
#define full     2
#define initckv  4
#define pasthw   8
{
    REG UWORD   i;              /* loop counter         */
    REG struct dpb *dparmp;     /* pointer to disk parm block */
    REG UWORD   dirsec;         /* sector number we're working on */
    REG UWORD   rtn;            /* return value         */
    REG UBYTE   *p;             /* scratch pointer      */
    REG UWORD   bitvec;         /* disk nmbr represented as a vector */

    BSETUP

    dparmp = GBL.parmp;                 /* init ptr to dpb */
    rtn  = 255;                         /* assume it doesn't work */

    i = ( (parms & continue) ? GBL.srchpos + 1 : 0 );

    while ( (parms & pasthw) || (i <= ((GBL.dphp)->hiwater + 1)) )
    {                           /* main directory scanning loop         */
        if ( i > dparmp->drm ) break;

        if ( ! (i & 3) )
        {                       /* inside loop happens when we need to
                                   read another directory sector        */
retry:      dirsec = i >> 2;
            dir_rd(dirsec);     /* read the directory sector    */

            if ( dirsec < (dparmp->cks) )  /* checksumming on this sector? */
            {
                p = ((GBL.dphp)->csv) + dirsec;
                                        /* point to checksum vector byte  */
                if (parms & initckv) *p = dchksum();
                else if (*p != dchksum())
                {                       /* checksum error! */
                    (GBL.dphp)->hiwater = dparmp->drm;  /* reset hi water */
                    bitvec = 1 << (GBL.curdsk);

                    if (crit_dsk & bitvec)      /* if disk in critical mode */
                        ro_dsk |= bitvec;       /* then set it to r/o   */
                    else
                    {
                        log_dsk &= ~bitvec;     /* else log it off  */
                        seldsk(GBL.curdsk);     /* and re-select it */
                        goto retry;             /* and re-do current op */
                    }
                }
            }
        }

        GBL.srchpos = i;

        if ( (*funcp)(fcbp, (GBL.dirbufp) + (i&3), (WORD)i) )
                        /* call function with parms of (1) fcb ptr,
                           (2) pointer to directory entry, and
                           (3) directory index */
        {
            if (parms & full)
              rtn = 0;  /* found a match, but keep going */
            else
              return(i & 3); /* return directory code */
        }

        i += 1;
    }

    return(rtn);
}

/*****************************************************************************/

/****************************************
*  Routines to manage allocation vector *
*       setaloc()                       *
*       clraloc()                       *
*       getaloc()                       *
****************************************/

setaloc(bitnum)
/*  Set bit in allocation vector        */
REG UWORD       bitnum;
{
    BSETUP

#if 0
    if (bitnum >= 0 && bitnum <= (GBL.parmp)->dsm)
#endif
    if( bitnum <= (GBL.parmp)->dsm )   /* bitnum is unsigned */
        *((GBL.dphp)->alv + (bitnum>>3)) |= 0x80 >> (bitnum & 7);
}

/*****************************************************************************/

clraloc(bitnum)
/* Clear bit in allocation vector       */
REG UWORD       bitnum;
{
    BSETUP

    if (bitnum > 0 && bitnum <= (GBL.parmp)->dsm)
        *((GBL.dphp)->alv + (bitnum>>3)) &= ~(0x80 >> (bitnum & 7));
}

/*****************************************************************************/

UWORD   chkaloc(i)
/* Check bit i in allocation vector                     */
/* Return non-zero if block free, else return zero      */
REG UWORD i;
{
    BSETUP

    return( ~(*( (GBL.dphp)->alv + (i >> 3) )) & (0x80 >> (i&7)) );
}

/*****************************************************************************/

UWORD   getaloc(leftblk)
/* Get a free block in the file system and set the bit in allocation vector */
/* It is passed the block number of the last block allocated to the file    */
/* It tries to allocate the block closest to the block that was passed      */
REG UWORD leftblk;
{
    REG UWORD   blk;            /* block number to allocate     */
    REG UWORD   rtblk;          /* high block number to try     */
    REG UWORD   diskmax;        /* # bits in alv - 1            */

    BSETUP
    LOCK                        /* need to lock the file system while messing
                                   with the allocation vector           */

    diskmax = (GBL.parmp)->dsm;
                                /* get disk max field from dpb          */
    rtblk = leftblk;
    blk = ~0;                   /* -1 returned if no free block found   */
    while (leftblk || rtblk < diskmax)
    {
        if (leftblk)
            if (chkaloc(--leftblk))
            {
                blk = leftblk;

                break;
            }

        if (rtblk < diskmax)
            if (chkaloc(++rtblk))
            {
                blk = rtblk;

                break;
            }
    }

    if (blk != (UWORD)~0) setaloc(blk);

    UNLOCK

    return(blk);
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
