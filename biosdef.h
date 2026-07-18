/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/*****************************************************************************/

/*********************************************************
 *                                                       *
 *       BIOS definitions for CP/M-68K                   *
 *                                                       *
 *       Copyright (c) 1982 Digital Research, Inc.       *
 *                                                       *
 *       This include file simply defines the BIOS calls *
 *                                                       *
 *********************************************************/

/*****************************************************************************/

#if 0
EXTERN UBYTE    bios1();  /* used for character I/O functions     */
EXTERN          bios2();  /* parm1 is word, no return value       */
EXTERN          bios3();  /* used for set dma only                */
                          /* parm1 is a pointer, no return        */
EXTERN UBYTE    *bios4(); /* seldsk only, parm1 and parm2 are     */
                          /*   words, returns a pointer to dph    */
EXTERN UWORD    bios5();  /* for sectran and set exception        */
EXTERN BYTE     *bios6(); /* for get memory segment table         */

# define bwboot() bios1 (1)             /* warm boot              */
# define bconstat() bios1 (2)           /* console status         */
# define bconin() bios1 (3)             /* console input          */
# define bconout(parm) bios2 (4, parm)  /* console output parm    */
# define blstout(parm) bios2 (5, parm)  /* list device output     */
# define bpun(parm) bios2 (6, parm)     /* punch char output      */
# define brdr() bios1 (7)               /* reader input           */
# define bhome() bios1 (8)              /* recalibrate drive      */
# define bseldsk(parm1, parm2) bios4 (9, parm1, parm2)
                                        /* select disk+return nfo */
# define bsettrk(parm) bios2 (10, parm) /* set track on disk      */
# define bsetsec(parm) bios2 (11, parm) /* set sector for disk    */
# define bsetdma(parm) bios3 (12, parm) /* set dma address        */
# define bread() bios1 (13)             /* read sector from disk  */
# define bwrite(parm) bios2 (14, parm)  /* write sector to disk   */
# define blistst() bios1 (15)           /* list device status     */
# define bsectrn(parm1, parm2) bios5 (16, parm1, parm2)
                                        /* sector translate       */
# define bgetseg() bios6 (18)           /* get memory segment tbl */
# define bgetiob() bios1 (19)           /* get I/O byte           */
# define bsetiob(parm) bios2 (20, parm) /* set I/O byte           */
# define bflush() bios1 (21)            /* flush buffers          */
# define bsetvec(parm1, parm2) bios5 (22, parm1, parm2)
/* set exception vector */
#endif /* if 0 */

/*****************************************************************************/

extern void bios_wboot (void);
extern unsigned short int bios_const (void);
extern unsigned char bios_conin (void);
extern void bios_conout (unsigned char victim);

/*****************************************************************************/

/* Clear serial (ANSI) + VGA text; see bios_con_clear(). */
extern void bios_con_clear (void);

/*****************************************************************************/

/* BDOS 222/223: enable/disable console devices (0=off 1=on 0xFFFF=query). */
extern unsigned short bios_con_vga_ctl (unsigned short info);
extern unsigned short bios_con_ser_ctl (unsigned short info);

/*****************************************************************************/

extern void bios_list (unsigned char victim);
extern void bios_punch (unsigned char victim);
extern unsigned char bios_reader (void);
extern void bios_home (void);
extern void *bios_seldsk (unsigned char drive, unsigned char logged);
extern void bios_settrk (unsigned short int track);
extern void bios_setsec (unsigned short int sector);
extern void bios_setdma (void *dmaaddress);
extern unsigned short int bios_read (void);
extern unsigned short int bios_write (unsigned short int typecode);
extern unsigned short int bios_listst (void);
extern unsigned short int bios_sectran (unsigned short int sector,
                                        void *table);
extern void *bios_getmrt (void);

/*****************************************************************************/

/*
 * Build classic CP/M base page in TPA[0..0xFF]: FCBs at 0x5C/0x6C, tail at 0x80.
 * fcb1/fcb2 are 36-byte FCBs (may be NULL); tail is the raw command tail text.
 */
extern void bios_setup_basepage (const void *fcb1, const void *fcb2,
                                 const char *tail);

/*****************************************************************************/

/* ZCPR-style GO: re-enter last loaded TPA program (0xFFFF if none). */
extern unsigned short pgm_go (void);

/*****************************************************************************/

extern unsigned short int bios_getiobyte (void);
extern void bios_setiobyte (unsigned short int iobyte);
extern unsigned short int bios_flush (void);
void *bios_setexc (unsigned short int vector, void *handler);

/*****************************************************************************/

#define bwboot() bios_wboot ()
#define bconstat() bios_const ()
#define bconin() bios_conin ()
#define bconout(parm) bios_conout (parm)
#define blstout(parm) bios_list (parm)
#define bpun(parm) bios_punch (parm)
#define brdr() bios_reader ()
#define bhome() bios_home ()
#define bseldsk(parm1, parm2) bios_seldsk (parm1, parm2)
#define bsettrk(parm) bios_settrk (parm)
#define bsetsec(parm) bios_setsec (parm)
#define bsetdma(parm) bios_setdma (parm)
#define bread() bios_read ()
#define bwrite(parm) bios_write (parm)
#define blistst() bios_listst ()
#define bsectrn(parm1, parm2) bios_sectran (parm1, parm2)
#define bgetseg() bios_getmrt ()
#define bgetiob() bios_getiobyte ()
#define bsetiob(parm) bios_setiobyte (parm)
#define bflush() bios_flush ()
#define bsetvec(parm1, parm2) bios_setexc (parm1, parm2)

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
