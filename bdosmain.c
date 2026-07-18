/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 36700c00-82b4-11f1-8e77-80ee73e9b8e7
 */

/*****************************************************************************/

#ifdef RLI
# include "diverge.h"
#endif /* ifdef RLI */

/*****************************************************************************/

/*****************************************************************
 *                                                               *
 *               CP/M-68K BDOS Main Routine                      *
 *                                                               *
 *       This is the main routine for the BDOS for CP/M-68K      *
 *       It has one entry point, _bdos, which is  called from    *
 *       the assembly language trap handler found in bdosif.s.   *
 *       The parameters are a function number (integer) and an   *
 *       information parameter (which is passed from bdosif as   *
 *       both an integer and a pointer).                         *
 *       The BDOS can potentially return a pointer, long word,   *
 *       or word                                                 *
 *                                                               *
 *       Configured for Alcyon C on the VAX                      *
 *                                                               *
 *****************************************************************/

/*****************************************************************************/

#include "bdosinc.h" /* Standard I/O declarations */

/*****************************************************************************/

#include "bdosdef.h" /* Type and structure declarations for BDOS */

/*****************************************************************************/

#include "biosdef.h" /* Declarations of BIOS functions */

/*****************************************************************************/

#include "platform.h" /* VGA text geometry for BDOS 224 */

/*****************************************************************************/

struct tempstr
{
  UBYTE tempdisk;
  BOOLEAN reselect;
  struct fcb *fptr;
};

/*****************************************************************************/

/* Declare EXTERN functions (adapted for modern gcc) */

EXTERN void warmboot (WORD);
EXTERN BOOLEAN constat (void);
EXTERN UBYTE conin (void);
EXTERN void tabout (UBYTE);
EXTERN UBYTE rawconio (UWORD);
EXTERN void prt_line (UBYTE *);
EXTERN void readline (UBYTE *);
EXTERN void seldsk (UBYTE);
EXTERN BOOLEAN openfile (UBYTE *, UBYTE *, WORD);
EXTERN void file_last_lrbc (UBYTE *);
EXTERN UWORD close_fi (UBYTE *);
EXTERN UWORD search (UBYTE *, UWORD, UBYTE *);
EXTERN UWORD bdosrw (UBYTE *, UWORD, UWORD);
EXTERN BOOLEAN create (UBYTE *, UBYTE *, WORD);
EXTERN BOOLEAN delete (UBYTE *, UBYTE *, WORD);
EXTERN BOOLEAN rename (UBYTE *, UBYTE *, WORD);
EXTERN BOOLEAN set_attr (UBYTE *, UBYTE *, WORD);
EXTERN void getsize (UBYTE *);
EXTERN void setran (UBYTE *);
EXTERN UWORD truncate_fi (UBYTE *);
EXTERN UWORD f_parse_fn (const UBYTE *, UBYTE *);
EXTERN void free_sp (UBYTE);
EXTERN UWORD flushit (void);
EXTERN UWORD pgmld (UBYTE *, UBYTE *);
EXTERN UWORD setexc (UBYTE *);
EXTERN void set_tpa (UBYTE *);
EXTERN void move (UBYTE *, UBYTE *, WORD);

/*****************************************************************************/

void tmp_sel (struct tempstr *temptr); /* local helper, defined below */

/*****************************************************************************/

/* Declare "true" global variables; i.e., those which will pertain to the
 * entire file system and thus will remain global even when this becomes
 * a multi-tasking file system
 */

GLOBAL UWORD log_dsk;  /* 16-bit vector of logged in drives      */
GLOBAL UWORD ro_dsk;   /* 16-bit vector of read-only drives      */
GLOBAL UWORD crit_dsk; /* 16-bit vector of drives in "critical"  */
                       /*  state.  Used to control dir checksums */
GLOBAL BYTE *tpa_lp;   /* TPA lower boundary (permanent)         */
GLOBAL BYTE *tpa_lt;   /* TPA lower boundary (temporary)         */
GLOBAL BYTE *tpa_hp;   /* TPA upper boundary (permanent)         */
GLOBAL BYTE *tpa_ht;   /* TPA upper boundary (temporary)         */

/*****************************************************************************/

/*
 * Declare the "state variables".  These are globals for the single-thread
 * version of the file system, but are put in a structure so they can be
 * based, with a pointer coming from the calling process
 */

GLOBAL struct stvars gbls;

/*****************************************************************************/

/******************************************************************
 *                                                                *
 *               _bdos MAIN ROUTINE                               *
 *                                                                *
 *       Called with  _bdos(func, info, infop)                    *
 *                                                                *
 *       Where:                                                   *
 *               func    is the BDOS function number (d0.w)       *
 *               info    is the word parameter (d1.w)             *
 *               infop   is the pointer parameter (d1.l)          *
 *                       note that info is the word form of infop *
 *                                                                *
 ******************************************************************/

/*****************************************************************************/

/*
 * Program return code for BDOS 108 (P_CODE).  CCP zeroes before each
 * transient; programs may set before BDOS 0.  Survives BDOS 47 chain.
 */

static UWORD program_retcode;

/*****************************************************************************/

UWORD
_bdos (func, info, infop) REG WORD func; /* BDOS function number   */
REG UWORD info;                          /* d1.w word parameter    */
REG UBYTE *infop;                        /* d1.l pointer parameter */
{
  REG UWORD rtnval;
  LOCAL struct tempstr temp;
  BSETUP

  temp.reselect = FALSE;
  temp.fptr = (void *)infop;
  rtnval = 0;

  switch (func) /* switch on function number */
    {
    case 0:
      warmboot (0); /* warm boot function */
      break;        /* does not return */

    case 1:
      return (UWORD)conin (); /* console input function */

      /* break; */

    case 2:
      tabout ((UBYTE)info); /* console output with */
      break;                /*    tab expansion    */

    case 3:
      return (UWORD)brdr (); /* get reader from bios */

      /* break; */

    case 4:
      bpun ((UBYTE)info); /* punch output to bios */
      break;

    case 5:
      blstout ((UBYTE)info); /* list output from bios */
      break;

    case 6:
      return (UWORD)rawconio (info); /* raw console I/O */

      /* break; */

    case 7:
      return bgetiob (); /* get i/o byte */

      /* break; */

    case 8:
      bsetiob (info); /* set i/o byte function */
      break;

    case 9:
      prt_line (infop); /* print line function */
      break;

    case 10:
      readline (infop); /* read buffered con input */
      break;

    case 11:
      return (UWORD)constat (); /* console status */

      /* break; */

    case 12:
      return VERSION; /* return version number */

      /* break; */

    case 13:
      log_dsk = 0; /* reset disk system */
      ro_dsk = 0;
      crit_dsk = 0;
      GBL.curdsk = 0xff;
      GBL.dfltdsk = 0;
      break;

    case 14:
      seldsk ((UBYTE)info); /* select disk */
      GBL.dfltdsk = (UBYTE)info;
      break;

    case 15:
      { /* open file */
        REG UBYTE want_lrbc;
        tmp_sel (&temp);
        want_lrbc = (((struct fcb *)infop)->cur_rec == (UBYTE)0xFF);
        ((struct fcb *)infop)->extent = 0;
        ((struct fcb *)infop)->s2 = 0;
        rtnval = dirscan (openfile, (UBYTE *)infop, 0);

        /*
         * DOS-PLUS LRBC is stored on the last extent; open always
         * binds extent 0, so rewrite s1/cur_rec when requested.
         */

        if (want_lrbc && rtnval < 255)
          {
            file_last_lrbc ((UBYTE *)infop);
          }

        break;
      }

    case 16:
      tmp_sel (&temp); /* close file */
      rtnval = close_fi (infop);
      break;

    case 17:
      GBL.srchp = (void *)infop; /* search first */
      rtnval = search ((UBYTE *)infop, 0, (UBYTE *)&temp);
      break;

    case 18:
      infop = (void *)GBL.srchp; /* search next */
      temp.fptr = (void *)infop;
      rtnval = search ((UBYTE *)infop, 1, (UBYTE *)&temp);
      break;

    case 19:
      tmp_sel (&temp); /* delete file */
      rtnval = dirscan (delete, (UBYTE *)infop, 2);
      break;

    case 20:
      tmp_sel (&temp); /* read sequential */
      rtnval = bdosrw (infop, TRUE, 0);
      break;

    case 21:
      tmp_sel (&temp); /* write sequential */
      rtnval = bdosrw (infop, FALSE, 0);
      break;

    case 22:
      tmp_sel (&temp); /* create file */
      ((struct fcb *)infop)->extent = 0;
      ((struct fcb *)infop)->s1 = 0;
      ((struct fcb *)infop)->s2 = 0;
      ((struct fcb *)infop)->rcdcnt = 0;
      /* Zero extent, S1, S2, rcrdcnt. create zeros rest */
      rtnval = dirscan (create, (UBYTE *)infop, 8);
      break;

    case 23:
      tmp_sel (&temp); /* rename file */
      rtnval = dirscan (rename, (UBYTE *)infop, 2);
      break;

    case 24:
      return log_dsk; /* return login vector */

      /* break; */

    case 25:
      return UBWORD (GBL.dfltdsk); /* return current disk */

      /* break; */

    case 26:
      GBL.dmaadr = infop; /* set dma address */
      break;

      /* No function 27 -- Get Allocation Vector */

    case 28:
      ro_dsk |= 1 << GBL.dfltdsk; /* set disk read-only */
      break;

    case 29:
      return ro_dsk; /* get read-only vector */

      /* break; */

    case 30:
      tmp_sel (&temp); /* set file attributes */
      rtnval = dirscan (set_attr, (UBYTE *)infop, 2);
      break;

    case 31:
      if (GBL.curdsk != GBL.dfltdsk)
        {
          seldsk (GBL.dfltdsk);
        }

      move ((UBYTE *)(GBL.parmp), (UBYTE *)infop, sizeof *(GBL.parmp));
      break; /* return disk parameters */

    case 32:
      if ((info & 0xff) <= 15) /* get/set user number */
        {
          GBL.user = (UBYTE)info;
        }

      return UBWORD (GBL.user);

      /* break; */

    case 33:
      tmp_sel (&temp); /* random read */
      rtnval = bdosrw (infop, TRUE, 1);
      break;

    case 34:
      tmp_sel (&temp); /* random write */
      rtnval = bdosrw (infop, FALSE, 1);
      break;

    case 35:
      tmp_sel (&temp); /* get file size */
      getsize (infop);
      break;

    case 36:
      tmp_sel (&temp); /* set random record */
      setran (infop);
      break;

    /*
     * BDOS 37 (DRV_RESET): DE = bitmap of drives to reset
     * (bit0=A: … bit15=P:).  Logs them off and clears software R/O
     * so the next select rebuilds the allocation vector from the dir.
     */

    case 37:
      info = ~info;
      log_dsk &= info;
      ro_dsk &= info;
      crit_dsk &= info;
      rtnval = 0;
      break;

    /*
     * BDOS 40 (F_WRITEZF): random write; newly allocated blocks
     * are zero-filled before the user record is written (bdosrw random=2).
     */

    case 40:
      tmp_sel (&temp);
      rtnval = bdosrw (infop, FALSE, 2);
      break;

    case 46:
      free_sp (info); /* get disk free space */
      break;

    /*
     * BDOS 98 - CP/M 3 "clean up disc" after program exit.
     * Officially closes open files without flushing dirty data.
     * No multi-open table here; no-op returning 0 is correct.
     */

    case 98:
      rtnval = 0;
      break;

    /*
     * BDOS 99 (F_TRUNCATE): ran0..2 = new size in records; cannot
     * extend.  Releases directory extents / allocation past EOF.
     */

    case 99:
      tmp_sel (&temp);
      rtnval = truncate_fi (infop);
      break;

    case 47:
      chainp = GBL.dmaadr; /*sw chain to program */
      warmboot (0);        /* terminate calling program */
      break;               /* does not return */

    case 48:
      return flushit (); /* flush buffers */

      /* break; */

    case 59:
      return pgmld (infop, GBL.dmaadr); /* program load */

      /* break; */

    case 61:
      return setexc (infop); /* set exception vector */

      /* break; */

    case 63:
      set_tpa (infop); /* get/set TPA limits */
      break;

    /*
     * BDOS 152 (F_PARSE): DE -> PFCB { u32 ascii; u32 fcb }
     * Ring-3! both fields are TPA-relative (use unsigned char for LE32;
     * UBYTE is signed char and would sign-extend address bytes).
     * Return (CP/M-386): 0xFFFF invalid; 0 if ended on NUL/CR;
     * else byte offset from the start of the ASCII string to the next
     * character (caller does ascii + ret with a 32-bit pointer
     * HL cannot hold full TPA offsets above 64K).
     * Password after ';' -> FCB+0x10, length FCB+0x1A.
     */

    case 152:
      {
        unsigned char *pfcb = (unsigned char *)infop;
        unsigned long ao, fo;
        UBYTE *ascii, *fcbp;
        extern int pmode_active (void);
        extern unsigned long pmode_tpa_base (void);
        extern unsigned long pmode_tpa_len (void);

        ao = (unsigned long)pfcb[0] | ((unsigned long)pfcb[1] << 8)
             | ((unsigned long)pfcb[2] << 16) | ((unsigned long)pfcb[3] << 24);
        fo = (unsigned long)pfcb[4] | ((unsigned long)pfcb[5] << 8)
             | ((unsigned long)pfcb[6] << 16) | ((unsigned long)pfcb[7] << 24);

        if (pmode_active ())
          {
            if (ao >= pmode_tpa_len () || fo >= pmode_tpa_len ())
              {
                rtnval = 0xFFFF;
                break;
              }

            ascii = (UBYTE *)(pmode_tpa_base () + ao);
            fcbp = (UBYTE *)(pmode_tpa_base () + fo);
          }
        else
          {
            ascii = (UBYTE *)ao;
            fcbp = (UBYTE *)fo;
          }

        rtnval = f_parse_fn (ascii, fcbp);
      }
      break;

    /* BDOS 163 (S_OSVER): user-visible product version (not BDOS 12). */
    case 163:
      rtnval = (UWORD)OSVER_CPM386;
      break;

    /*
     * CP/M Plus-style date/time (CP/M-386: binary h/m/s, not BCD).
     * DE -> struct cpm_datetime { UWORD days; UBYTE h,m,s; }
     * days since 1978-01-01.  Implemented via CMOS RTC in ring 0.
     */

    case 104: /* set date and time */
      {
        extern int rtc_set (void *);
        rtnval = (UWORD)rtc_set (infop);
      }
      break;

    case 105: /* get date and time (binary h/m/s; always includes sec) */
      {
        extern int rtc_get (void *);
        rtnval = (UWORD)rtc_get (infop);
      }
      break;

    /* BDOS 107 (S_SERIAL) - Get serial number. DE -> 6-byte buffer. */
    case 107:
      {
        extern char *serial; /* defined in bdosmisc.c */
        char *buf = (char *)infop;
        int i;

        if (buf)
          {
            /*
             * Copy 6 bytes, assume SN is at least 16 chars, something like
             * "XXXX-0000-654321$", and take the last 6 chars before the `$`
             */
            for (i = 0; i < 6; i++)
              {
                buf[i] = serial[10 + i];
              }
          }

        rtnval = 0;
      }

      break;

    /*
     * BDOS 141 (P_DELAY) - Delay process for DE ticks (60Hz).
     */

    case 141:
      {
        extern void pit_read (unsigned long *, unsigned long *);
        extern void pit_sleep_until (unsigned long, unsigned long);
        unsigned long lo = 0, hi = 0;
        unsigned long long target;
        unsigned long ticks = (unsigned long)infop;

        if (ticks > 0)
          {
            pit_read (&lo, &hi);
            target = ((unsigned long long)hi << 32) | lo;
            /* 1 tick at 60Hz = 1193182 / 60 = 19886 PIT cycles */
            target += (unsigned long long)ticks * 19886ULL;
            pit_sleep_until ((unsigned long)target,
                             (unsigned long)(target >> 32));
          }

        rtnval = 0;
      }

      break;

    /*
     * BDOS 155 (T_SECONDS) - MP/M / Concurrent: get date and time with
     * packed-BCD hour/minute/second (and filled seconds field).
     * DE -> same layout as 105; day count matches our BDOS 105.
     */

    case 155:
      {
        extern int rtc_get_bcd (void *);
        rtnval = (UWORD)rtc_get_bcd (infop);
      }
      break;

    /*
     * BDOS 108 (P_CODE) - CP/M 3 get/put program return code.
     * DE=0xFFFF -> return current code in HL (our UWORD return).
     * else      -> set code from DE and return it.
     * Ranges (seasip): 0000-FEFF non-fatal, FF00-FF7F fatal,
     * FFFD hardware error terminate, FFFE Control-C terminate.
     */

    case 108:
      if (info == (UWORD)0xFFFF)
        {
          rtnval = program_retcode;
        }
      else
        {
          program_retcode = info;
          rtnval = program_retcode;
        }

      break;

    /*
     * CP/M-386 private: machine reboot (REBOOT.386).
     * info & 1 = warm (BDA 0x472).
     */

    case 220:
      {
        extern void bios_system_reboot (int);
        bios_system_reboot ((int)(info & 1));
      }
      break;

    /*
     * CP/M-386 private: clear console (CLS.386).
     * Serial ANSI + VGA wipe; reset column.
     */

    case 221:
      {
        extern void bios_con_clear (void);
        bios_con_clear ();
        GBL.column = 0;
      }
      break;

    /*
     * BDOS 222: VGA console on/off/query.
     * DE=0 off, DE=1 on, DE=0xFFFF query.
     * Returns 0/1 or 0xFF if disabling would leave no output device.
     */

    case 222:
      {
        extern unsigned short bios_con_vga_ctl (unsigned short);
        rtnval = bios_con_vga_ctl (info);
      }
      break;

    /* BDOS 223: serial console on/off/query (same ABI as 222). */
    case 223:
      {
        extern unsigned short bios_con_ser_ctl (unsigned short);
        rtnval = bios_con_ser_ctl (info);
      }
      break;

    /*
     * BDOS 224 (CON_VIDEO): direct VGA text map for ring-3.
     * DE -> struct cpm_vga_text (TPA-relative).  Fills sel/cols/rows/…
     * AX = selector|RPL3 on success, 0xFFFF if platform has no text map
     * (see platform.h CPM386_HAS_VGA_TEXT).
     */
    case 224:
      {
        extern unsigned short pmode_vga_selector (void);
        extern unsigned long pmode_vga_phys_base (void);
        extern unsigned long pmode_vga_map_size (void);
        REG struct cpm_vga_text *vp = (struct cpm_vga_text *)infop;
        UWORD sel = pmode_vga_selector ();

        if (!sel || !vp)
          {
            if (vp)
              {
                vp->sel = 0;
                vp->cols = 0;
                vp->rows = 0;
                vp->cell_bytes = 0;
                vp->map_size = 0;
                vp->phys_base = 0;
              }
            rtnval = 0xFFFF;
            break;
          }

        vp->sel = sel;
#if CPM386_HAS_VGA_TEXT
        vp->cols = (UWORD)CPM386_VGA_TEXT_COLS;
        vp->rows = (UWORD)CPM386_VGA_TEXT_ROWS;
        vp->cell_bytes = (UWORD)CPM386_VGA_TEXT_CELL;
#else
        vp->cols = 0;
        vp->rows = 0;
        vp->cell_bytes = 0;
#endif
        vp->map_size = (ULONG)pmode_vga_map_size ();
        vp->phys_base = (ULONG)pmode_vga_phys_base ();
        rtnval = sel;
      }
      break;

    /*
     * BDOS 225 (GET_TICKS): high-resolution free-running counter.
     * DE -> struct cpm_ticks (TPA-relative).
     * Fills lo/hi/hz.
     * hz = 1_193_182 (8253/8254 PIT).
     * AX = 0 on success, 0xFFFF if DE is null / out of range.
     */

    case 225:
      {
        extern void pit_read (unsigned long *, unsigned long *);
        REG struct cpm_ticks *tp = (struct cpm_ticks *)infop;

        if (!tp)
          {
            rtnval = 0xFFFF;
            break;
          }

        {
          unsigned long lo = 0, hi = 0;

          pit_read (&lo, &hi);
          tp->lo = (ULONG)lo;
          tp->hi = (ULONG)hi;
          tp->hz = 1193182UL; /* PIT_HZ; keep literal for freestanding */
        }

        rtnval = 0;
      }

      break;

    /*
     * BDOS 226 (SLEEP_UNTIL): busy-wait until absolute tick >= (hi:lo).
     * DE -> struct cpm_ticks
     * only lo/hi matter (hz ignored).
     * Returns immediately if already in the past.
     * AX = 0, or 0xFFFF if DE null / out of range.
     */

    case 226:
      {
        extern void pit_sleep_until (unsigned long, unsigned long);
        REG struct cpm_ticks *tp = (struct cpm_ticks *)infop;

        if (!tp)
          {
            rtnval = 0xFFFF;

            break;
          }

        pit_sleep_until ((unsigned long)tp->lo, (unsigned long)tp->hi);
        rtnval = 0;
      }

      break;

    default:
      return -1; /* bad function number */
                 /* break; */
    }; /* end of switch statement */

  if (temp.reselect)
    {
      ((struct fcb *)infop)->drvcode = temp.tempdisk;
    }

  /* if reselected disk, restore it now */

  return rtnval; /* return the BDOS return value */
} /* end _bdos */

/*****************************************************************************/

void
tmp_sel (
    REG struct tempstr *temptr) /* temporarily select disk pointed to by fcb */
{
  REG struct fcb *fcbp;
  REG UBYTE tmp_dsk;
  BSETUP

  fcbp = temptr->fptr; /* get local copy of fcb pointer */
  tmp_dsk = (temptr->tempdisk = fcbp->drvcode);
  seldsk (tmp_dsk ? tmp_dsk - 1 : GBL.dfltdsk);

  fcbp->drvcode = GBL.user;
  temptr->reselect = TRUE;
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
