/*********************************************************
 *                                                       *
 *               CP/M-68K header file                    *
 *    Copyright (c) 1982 by Digital Research, Inc.       *
 *    Structure definitions for doing I/O in packets     *
 *                                                       *
 *********************************************************/

/*
 * May use this information structure instead of disk parameter header and
 * disk parameter block in future, but for now it's unused
 */

#if 0
struct dskinfo
{
  UBYTE *dbuffp;
  UBYTE *csv;
  UBYTE *alv;
  UBYTE blksize;
  UBYTE didummy;
  UWORD dskmax;
  UWORD dirmax;
  UWORD chksize;
};
#endif /* if 0 */

struct iopb
{
  UBYTE iofcn;       /* function number, see defines below    */
  UBYTE ioflags;     /* used for login flag and write flag    */
  UBYTE devtype;     /* device type, see defines below        */
                     /* currently unused                      */
  UBYTE devnum;      /* device number, or, devtype and devnum */
                     /* taken together form int device number */
  LONG devadr;       /* item nmbr on device to start xfer at  */
                     /* note -- item is sector for disks,     */
		     /*         byte for char devs            */
  UWORD xferlen;     /* number items to transfer              */
  UBYTE *xferadr;    /* memory address to xfer to/from        */
  struct dph *infop; /* pointer to disk parameter header      */
                     /* return parm for fcn 0, input for rest */
};

/*  Definitions for iofcn, the function number */
#define sel_info 0 /* select and return info on device */
#define read 1
#define write 2
#define flush 3
#define status 4 /* not currently used */

/* Definitions for devtype, the device type */
#define console 0
#define printer 1
#define disk 2
#define memory 3  /* gets TPA boundaries  */
#define redir 4   /* read/write IOByte    */
#define exc_vec 5 /* set exception vector */
