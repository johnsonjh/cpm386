/*
 * CP/M-386 - ed.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: dbea540e-9102-11f1-9f49-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * ed.c - the CP/M context editor
 *
 * Ported from the Digital Research / Zilog portable C sources.
 *
 * Usage:
 *   ED filespec           - Edit (or create) a file
 *   ED filespec d:        - Edit, writing the result to drive d:
 *
 * The original file is kept as .BAK, and commands are the classic ED set:
 * A(ppend) B(eginning) C(haracter) D(elete) E(nd) F(ind) H(ead) I(nsert)
 * J(uxtapose) K(ill) L(ine) M(acro) N(find with autoscan) O(riginal)
 * P(age) Q(uit) R(ead library) S(ubstitute) T(ype) U(pper case) V(erify)
 * W(rite) X(fer to library) Z(sleep), and ^ - . = # * as line prefixes.
 */

/*****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*       E D  :   T h e   C P / M   C o n t e x t    E d i t o r            */
/*       -------------------------------------------------------            */
/*                                                                          */
/*      Copyright (c) 1976, 1977, 1978, 1979, 1980, 1981, 1982              */
/*          Digital Research                                                */
/*          Box 579 Pacific Grove                                           */
/*          California 93950                                                */
/*                                                                          */
/*          Revised:                                                        */
/*            07 April 81  by Thomas Rolander                               */
/*            21 July  81  by Doug Huskey                                   */
/*            29 Oct   81  by Doug Huskey                                   */
/*            10 Nov   81  by Doug Huskey                                   */
/*            08 July  82  by Bill Fitler                                   */
/*            26 July  82  by Doug Huskey                                   */
/*             1 Aug   82  by Dave Sallume, Zilog Inc.                      */
/*            12 Sept  82  by Dominic Dunlop, Zilog Inc.                    */
/*            06 Aug   26  by Jeffrey Johnson (CP/M-386 PIP v1.0)           */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*              M O D I F I C A T I O N   L O G                             */
/*              -------------------------------                             */
/*                                                                          */
/* Modified for .PRL operation                  May, 1979                   */
/* Modified for operation with CP/M 2.0         August 1979                 */
/* Modified for MP/M 2.0                        June 1981                   */
/* Modified for CP/M 1.1                        October 1981                */
/* Modified for concurrent CP/M 1.0             July 1982                   */
/* Modified for CP/M 3.0                        July 1982                   */
/* Translated to C                              August 1982                 */
/*                                                                          */
/*   Aug 1982 Zilog: Translated to C.                                       */
/*                                                                          */
/*   July 1982 WHF: Some code cleanup (grouped logicals, declared bool);    */
/*        fixed disk full error handling; fixed read from null files;       */
/*        fixed (some) of the dirty fcb handling (shouldn't use settype     */
/*        function on open FCB's!).                                         */
/*                                                                          */
/*   July 1982 DH: Installed patches to change macro abort command from     */
/*        ^C to ^Y and to not print error message when trying to delete     */
/*        a file that doesn't exist.  Added perror: procedure to print      */
/*        error messages in a consistant format and modified error          */
/*        message handler at reset: entry point.  Also corrected invalid    */
/*        filename error to not abort ed if parsing a R or X command.       */
/*        modified start and setdest to prompt for missing                  */
/*        filenames.  Modified parse_fcb & parse_lib to set a global        */
/*        flag and break if it got an invalid filename for X or R commands. */
/*        Start sets page size from the system control block (SCB) if       */
/*        ed is running under CP/M-80 (ver & 0xff00 == 0).                  */
/*        The H command now works with new files. (sets newfile = FALSE)    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*           P R O G R A M   D E S C R I P T I O N                          */
/*                                                                          */
/*    Command           Function                                            */
/*    -------           --------                                            */
/*     A            Append lines of text to buffer                          */
/*     B            move to Beginning or end of text                        */
/*     C            skip Characters                                         */
/*     D            Delete characters                                       */
/*     E            End of edit                                             */
/*     F            Find string in current buffer                           */
/*     H            move to top of file (Head)                              */
/*     I            Insert characters from keyboard                         */
/*                  up to next <ENDFILE>                                    */
/*     J            Juxtaposition operation - search for first string,      */
/*                  insert second string, delete until third string         */
/*     K            delete (Kill) lines                                     */
/*     L            skip Lines                                              */
/*     M            Macro definition (see comment below)                    */
/*     N            find Next occurrence of string                          */
/*                    with auto scan through file                           */
/*     O            re-edit Old file                                        */
/*     P            Page and display (moves up or down 23 lines and         */
/*                    displays 24 lines)                                    */
/*     Q            Quit edit without updating the file                     */
/*     R<filename>  Read from file <filename> until <ENDFILE> and           */
/*                    insert into text                                      */
/*     S            Search for first string, replace by second string       */
/*     T            Type lines                                              */
/*     U            translate to Upper case (-u changes to no translate)    */
/*     V            Verify (print) line numbers (-v turns them off)         */
/*     W            Write lines of text to file                             */
/*     X<filename>  transfer (Xfer) lines to file <filename>                */
/*     Z            sleep for 1/2 second (used in macros to stop display)   */
/*     <CR>         move up or down and print one line                      */
/*                                                                          */
/*                                                                          */
/*   In general, the editor accepts single letter commands with optional    */
/*   integer values preceding the command.  The editor accepts both upper   */
/*   and lower case commands and values, and performs translation to upper  */
/*   case under the following conditions.  If the command is typed in       */
/*   upper case, then the data which follows is translated to upper case.   */
/*   Thus, if the "I" command is typed in upper case, then all input is     */
/*   automatically translated (although echoed in lower case, as typed).    */
/*   If the "A" command is typed in upper case, then all input is           */
/*   translated as read from the disk.  Global translation to upper case    */
/*   can be controlled by the "U" command (-U to negate its effect).        */
/*   If you are operating with an upper case only terminal, then operation  */
/*   is automatic.  Similarly, if you are operating with a lower case       */
/*   terminal, and translation to upper case is not specified, then lower   */
/*   case characters can be entered.                                        */
/*                                                                          */
/*   A number of commands can be preceded by a positive or                  */
/*   negative integer between 0 and 65535 (1 is default if no value         */
/*   is specified).  This value determines the number of times the          */
/*   command is applied before returning for another command.               */
/*      The commands                                                        */
/*                     C D K L T P U V <CR>                                 */
/*   can be preceded by an unsigned, positive, or negative number,          */
/*   the commands                                                           */
/*                     A F J N W Z                                          */
/*   can be preceded by an unsigned or positive number,                     */
/*   the commands                                                           */
/*                     E H O Q                                              */
/*   cannot be preceded by a number.  The commands                          */
/*                     F I J M R S                                          */
/*   are all followed by one or more strings of characters which can        */
/*   be optionally separated or terminated by either <ENDFILE> or <CR>.     */
/*   The <ENDFILE> is generally used to separate the search strings         */
/*   in the S and J commands, and is used at the end of the commands if     */
/*   additional commands follow.  For example, the following command        */
/*   sequence searches for  the string 'GAMMA', substitutes the string      */
/*   'DELTA', and then types the first part of the line where the           */
/*   change occurred, followed by the remainder of the line which was       */
/*   changed:                                                               */
/*               SGAMMA<ENDFILE>DELTA<ENDFILE>0TT<CR>                       */
/*                                                                          */
/*   The control-L character in search and substitute strings is            */
/*   replaced on input by <CR><LF> characters.  The control-I key           */
/*   is taken as a TAB character.                                           */
/*                                                                          */
/*      The commands R & X must be followed by a file name (with default    */
/*   file type of 'LIB') with a trailing <CR> or <ENDFILE>.  The command    */
/*   I is followed by a string of symbols to insert, terminated by          */
/*   a <CR> or <ENDFILE>.  If several lines of text are to be inserted,     */
/*   the I can be directly followed by an <ENDFILE> or <CR> in which        */
/*   case the editor accepts lines of input to the next <ENDFILE>.          */
/*   The command 0T prints the first part of the current line,              */
/*   and the command 0L moves the reference to the beginning of the         */
/*   current line.  The command 0P prints the current page only, while      */
/*   the command 0Z reads the console rather than waiting (this is used     */
/*   again within macros to stop the display - the macro expansion          */
/*   stops until a character is read.  If the character is not a break      */
/*   then the macro expansion continues normally).                          */
/*                                                                          */
/*      Note that a pound sign (#) is taken as the number 65535, all        */
/*   unsigned numbers are assumed positive, and a single - is assumed -1    */
/*                                                                          */
/*   A number of commands can be grouped together and executed              */
/*   repetitively using the macro command which takes the form              */
/*                                                                          */
/*           <number>Mc1c2...cn<delimiter>                                  */
/*                                                                          */
/*   where <number> is a non-negative integer n, and <delimiter> is         */
/*   <ENDFILE> or <CR>.  The commands c1 ... cn  following the M are        */
/*   executed n times, starting at the current position in the buffer.      */
/*   If n is 0, 1, or omitted, the commands are executed until the end      */
/*   of the buffer is encountered.                                          */
/*                                                                          */
/*   The following macro, for example, changes all occurrences of           */
/*   the name 'gamma' to 'delta', and prints the lines which                */
/*   were changed:                                                          */
/*                                                                          */
/*               MFgamma<ENDFILE>-5IDdelta<ENDFILE>0LT<CR>                  */
/*                                                                          */
/*  (Note: an <ENDFILE> is the CP/M end of file mark - control-Z)           */
/*                                                                          */
/*  If any key is depressed during typing or macro expansion, the           */
/*  function is considered terminated, and control returns to the           */
/*  operator.                                                               */
/*                                                                          */
/*  Error conditions are indicated by printing one of the characters:       */
/*                                                                          */
/*   symbol                    error condition                              */
/*   ------      ----------------------------------------------------       */
/*   greater     free memory is exhausted - any command can be issued       */
/*     (>)       which does not increase memory requirements.               */
/*                                                                          */
/*   question    unrecognized command or illegal numeric field              */
/*     (?)                                                                  */
/*                                                                          */
/*   pound       cannot apply the command the number of times specfied      */
/*     (#)       (occurs if search string cannot be found)                  */
/*                                                                          */
/*   letter O    cannot open <filename>.LIB in R command                    */
/*                                                                          */
/*   The error character is also accompanied by the last character          */
/*   scanned when the error occurred.                                       */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*          H E A D E R   F I L E S ,    C O N F I G U R A T I O N          */
/*          ------------------------------------------------------          */
/*                                                                          */
/****************************************************************************/

/* CP/M-386: the headers below stood in for a C library that is not
   present here; the shim supplies what this program actually uses. */

/****************************************************************************/
/*                                                                          */
/*          C P / M - 3 8 6   R U N T I M E   S H I M                       */
/*          -----------------------------------------                       */
/*                                                                          */
/*      Everything below stands in for the CP/M-68K C startup and the       */
/*      portab.h / bdos.h / basepage.h / setjmp.h headers this program was  */
/*      written against.  There is no C library: the only way out of the    */
/*      program is a direct BDOS call, which is what __BDOS() below is.     */
/*                                                                          */
/****************************************************************************/

/*--------------------------- portab.h ------------------------------------*/

#define BYTE char               /* Signed byte                              */
#define BOOLEAN char            /* 2 valued (true/false)                    */
#define WORD short              /* Signed word (16 bits)                    */
#define UWORD unsigned short    /* Unsigned word (16 bits)                  */
#define LONG long               /* Signed long (32 bits)                    */
#define ULONG unsigned long     /* Unsigned long                            */
#define UBYTE unsigned char     /* Unsigned byte                            */
#define REG register
#define LOCAL auto
#define EXTERN extern
#define MLOCAL static
#define GLOBAL /**/
#define VOID /**/

#define FAILURE (-1)
#define SUCCESS (0)
#define YES 1
#define NO 0
#define FOREVER for (;;)
#define NULL 0
#define EOF (-1)
#define TRUE (1)
#define FALSE (0)

/*
 * UWORD is 16 bits here, as it was on the 68000 and Z8000 this came from.
 * That is not cosmetic: the buffer indices below are UWORD, and widening
 * them would change how the program divides up memory.
 */

/*--------------------------- bdos.h --------------------------------------*/

#define XADDR long                      /* 32-bit address data type         */

#define EXIT 0                          /* Exit to BDOS                     */
#define CONIN 1                         /* Single char input with echo      */
#define CONOUT 2                        /* Direct console output            */
#define READER 3                        /* Paper tape input                 */
#define PUNCH 4                         /* Paper tape output                */
#define LSTOUT 5                        /* Direct list device output        */
#define CONIO 6                         /* Direct console I/O               */
#define GET_IOB 7                       /* Get I/O byte                     */
#define SET_IOB 8                       /* Set I/O byte                     */
#define PRINT 9                         /* Print $-terminated line          */
#define CONBUF 10                       /* Read console buffer              */
#define CONSTAT 11                      /* Check if input char waiting      */
#define VERSION 12                      /* Return version number            */
#define RS_DISK 13                      /* Reset disk system                */
#define SEL_DISK 14                     /* Select disk                      */
#define OPEN 15                         /* Open a disk file                 */
#define CLOSE 16                        /* Close a disk file                */
#define SRCH_1ST 17                     /* Search 1st filename match        */
#define SRCH_NEXT 18                    /* Search next match                */
#define DELETE 19                       /* Delete a disk file               */
#define S_READ 20                       /* Sequential read from file        */
#define S_WRITE 21                      /* Sequential write to file         */
#define CREATE 22                       /* Create a disk file               */
#define RENAME 23                       /* Rename a file                    */
#define RET_LOGIN 24                    /* Return login vector              */
#define RET_CDISK 25                    /* Return current disk              */
#define SETDMA 26                       /* Set DMA address                  */
#define GET_ALLOC 27                    /* Get allocation vector            */
#define WR_PROTD 28                     /* Write protect disk               */
#define GET_RO 29                       /* Get read-only vector             */
#define SET_ATT 30                      /* Set file attributes              */
#define GET_DPB 31                      /* Get disk parameters              */
#define GSET_UCODE 32                   /* Get/set user code                */
#define B_READ 33                       /* Read random record               */
#define B_WRITE 34                      /* Write random record              */
#define FILSIZ 35                       /* Compute file size                */
#define SET_RAND 36                     /* Set random record                */
#define RS_DRIVE 37                     /* Reset specified drive            */
#define B_WRZF 40                       /* Write random, zero fill          */
#define SETMSC 44                       /* Set multi-sector count           */
#define RET_ERRORS 45                   /* Set error return mode            */
#define GET_DFS 46                      /* Get free disk space              */
#define CHAIN 47                        /* Chain to program via CCP         */
#define FLUSH 48                        /* Flush buffers to disk            */
#define GSET_SCB 49                     /* Get/set system control block     */
#define BIOS_CALL 50                    /* Direct call to BIOS              */
#define PROG_LOAD 59                    /* Program load                     */
#define SET_EXV 61                      /* Set exception vector             */
#define SET_SUP 62                      /* Set supervisor state             */
#define SET_LABEL 100                   /* Set directory label              */
#define GET_LABEL 101                   /* Get directory label              */
#define GET_XFCB 102                    /* Get extended FCB                 */
#define SET_XFCB 103                    /* Set extended FCB                 */
#define COND_LST 161                    /* Conditionally attach LST:        */

/*
 * The CP/M-386 BDOS entry point: function in EAX, parameter in EDX, result
 * in EAX.  Pointer parameters are TPA-relative, and since a transient is
 * linked at 0x100 and loaded at TPA+0x100, a plain C pointer already is
 * one.  Unimplemented functions return 0xFFFF, which is what the version
 * guards in this program keep us away from anyway.
 */

#define BDOS_INT 0x30

static long
__BDOS (parm_func, parm_info)
int parm_func;
long parm_info;
{
  unsigned int ret;

  __asm__ volatile ("int %2"
                    : "=a" (ret)
                    : "a" ((unsigned int) parm_func),
                      "i" (BDOS_INT),
                      "d" ((unsigned long) parm_info)
                    : "memory", "cc");

  return (long) (unsigned short) ret;
}

#define _conin()        (__BDOS (CONIN, (long) 0))
#define _conout(a)      (__BDOS (CONOUT, (long) (a)))
#define _reader()       (__BDOS (READER, (long) 0))
#define _punch(a)       (__BDOS (PUNCH, (long) (a)))
#define _lstout(a)      (__BDOS (LSTOUT, (long) (a)))
#define _conio(a)       (__BDOS (CONIO, (long) (a)))
#define _get_iob()      (__BDOS (GET_IOB, (long) 0))
#define _set_iob(a)     (__BDOS (SET_IOB, (long) (a)))
#define _print(a)       (__BDOS (PRINT, (long) (a)))
#define _conbuf(a)      (__BDOS (CONBUF, (long) (a)))
#define _constat()      (__BDOS (CONSTAT, (long) 0))
#define _version()      (__BDOS (VERSION, (long) 0))
#define _rs_disk(a)     (__BDOS (RS_DISK, (long) (a)))
#define _sel_disk(a)    (__BDOS (SEL_DISK, (long) (a)))
#define _open(a)        (__BDOS (OPEN, (long) (a)))
#define _close(a)       (__BDOS (CLOSE, (long) (a)))
#define _srch_1st(a)    (__BDOS (SRCH_1ST, (long) (a)))
#define _srch_next()    (__BDOS (SRCH_NEXT, (long) 0))
#define _delete(a)      (__BDOS (DELETE, (long) (a)))
#define _s_read(a)      (__BDOS (S_READ, (long) (a)))
#define _s_write(a)     (__BDOS (S_WRITE, (long) (a)))
#define _create(a)      (__BDOS (CREATE, (long) (a)))
#define _rename(a)      (__BDOS (RENAME, (long) (a)))
#define _ret_login()    (__BDOS (RET_LOGIN, (long) 0))
#define _ret_cdisk()    (__BDOS (RET_CDISK, (long) 0))
#define _setdma(a)      (__BDOS (SETDMA, (long) (a)))
#define _get_alloc()    (__BDOS (GET_ALLOC, (long) 0))
#define _wr_protd()     (__BDOS (WR_PROTD, (long) 0))
#define _get_ro()       (__BDOS (GET_RO, (long) 0))
#define _set_att(a)     (__BDOS (SET_ATT, (long) (a)))
#define _get_dpb(a)     (__BDOS (GET_DPB, (long) (a)))
#define _get_dpa()      (__BDOS (GET_DPB, (long) 0))
#define _gset_ucode(a)  (__BDOS (GSET_UCODE, (long) (a)))
#define _b_read(a)      (__BDOS (B_READ, (long) (a)))
#define _b_write(a)     (__BDOS (B_WRITE, (long) (a)))
#define _filsiz(a)      (__BDOS (FILSIZ, (long) (a)))
#define _set_rand(a)    (__BDOS (SET_RAND, (long) (a)))
#define _rs_drive(a)    (__BDOS (RS_DRIVE, (long) (a)))
#define _b_wrzf(a)      (__BDOS (B_WRZF, (long) (a)))
#define _setmsc(a)      (__BDOS (SETMSC, (long) (a)))
#define _ret_errors(a)  (__BDOS (RET_ERRORS, (long) (a)))
#define _get_dfs(a)     (__BDOS (GET_DFS, (long) (a)))
#define _chain()        (__BDOS (CHAIN, (long) 0))
#define _flush()        (__BDOS (FLUSH, (long) 0))
#define _gset_scb(a)    (__BDOS (GSET_SCB, (long) (a)))
#define _bios_call(a)   (__BDOS (BIOS_CALL, (long) (a)))
#define _prog_load(a)   (__BDOS (PROG_LOAD, (long) (a)))
#define _set_exv(a)     (__BDOS (SET_EXV, (long) (a)))
#define _set_sup(a)     (__BDOS (SET_SUP, (long) 0))
#define _get_label(a)   (__BDOS (GET_LABEL, (long) (a)))
#define _set_label(a)   (__BDOS (SET_LABEL, (long) (a)))
#define _get_xfcb(a)    (__BDOS (GET_XFCB, (long) (a)))
#define _set_xfcb(a)    (__BDOS (SET_XFCB, (long) (a)))
#define _cond_lst()     (__BDOS (COND_LST, (long) 0))

struct fcbtab                           /* File control block               */
{
  BYTE drive;                           /* Disk drive field                 */
  BYTE fname[8];                        /* File name                        */
  BYTE ftype[3];                        /* File type                        */
  BYTE extent;                          /* Current extent number            */
  BYTE s1, s2;                          /* "system reserved"                */
  BYTE reccnt;                          /* Record counter                   */
  BYTE resvd[16];                       /* More "system reserved"           */
  LONG record;                          /* Overlaps the current record      */
};                                      /*   field to make this useful      */

#define fcb fcbtab                      /* A useful synonym                 */

/*
 * The FCB's 24-bit random record number lives in the three bytes after the
 * current-record byte, most significant first - the CP/M-68K packing that
 * CP/M-386's BDOS uses.  This source reaches it through the overlapping
 * LONG, which makes the answer depend on the host's byte order as well as
 * the field's, and on a little-endian machine neither of the arrangements
 * the original allows for is right.  Go at the bytes by name instead: that
 * is correct either way round.
 */

#define CURREC(f) (((UBYTE *) &(f)->record)[0])

#if 0
static long
ranrec (fcbp)                           /* Read the random record number   */
struct fcbtab *fcbp;
{
  UBYTE *b = (UBYTE *) &fcbp->record;

  return ((long) b[1] << 16) | ((long) b[2] << 8) | (long) b[3];
}

static void
setranrec (fcbp, n)                     /* Write the random record number  */
struct fcbtab *fcbp;
long n;
{
  UBYTE *b = (UBYTE *) &fcbp->record;

  b[1] = (UBYTE) (n >> 16);
  b[2] = (UBYTE) (n >> 8);
  b[3] = (UBYTE) n;
}
#endif

#define SECSIZ 128                      /* Size of CP/M sector              */
#define _MAXSXFR 1                      /* Max # sectors transferable       */
#define _MAXSHFT 12                     /* Shift right BDOS return value    */

/*
 * Last record byte count.
 *
 * CP/M stores a file's length as a whole number of 128-byte records, so the
 * tail of the last one is padding and the exact length is lost.  CP/M-386
 * keeps the missing byte count in the directory alongside each file: open
 * with the current-record byte set to LRBC_ASK and it comes back in that
 * same byte, 0 meaning the last record is full.
 *
 * Setting it is an attribute change with the F6\' bit of the name turned on
 * and the count in the current-record byte.  F6\' is an ordinary
 * user-definable attribute the rest of the time, and a later attribute
 * change made from a directory entry that still had it set would rewrite
 * the count from whatever happened to be in that byte - so turn it off
 * again once the count has been stored.
 */

#define LRBC_ASK 0xFF                   /* cur_rec value that requests it   */
#define LRBC_F6 0x80                    /* fname[5] bit that commits it     */

static void
set_lrbc (drive, name, lrbc)
int drive;                              /* FCB drive code, 0 = default      */
BYTE *name;                             /* 11 bytes: 8 name plus 3 type     */
int lrbc;                               /* 0 = last record full             */
{
  struct fcbtab t;
  int i;

  for (i = 0; i < (int) sizeof t; i++)
    ((BYTE *) &t)[i] = 0;

  t.drive = (BYTE) drive;

  for (i = 0; i < 8; i++)               /* Carries the attribute bits too   */
    t.fname[i] = name[i];

  for (i = 0; i < 3; i++)
    t.ftype[i] = name[8 + i];

  t.fname[5] |= LRBC_F6;
  CURREC (&t) = (UBYTE) lrbc;
  (void) _set_att (&t);

  t.fname[5] &= 0x7F;                   /* Put the attribute back as it was */
  (void) _set_att (&t);
}

/*------------------------- basepage.h ------------------------------------*/

/*
 * CP/M-386 keeps the classic CP/M base page rather than the CP/M-68K one:
 * the parsed command line FCB sits at 0x5C and the command tail, which
 * doubles as the default DMA buffer, at 0x80.  Only those two fields are
 * used here, so the rest is simply reserved.
 */

struct b_page
{
  BYTE resvd[0x5C];                     /* 0x00 - not used by this program  */
  struct fcbtab fcb1;                   /* 0x5C - first command line FCB    */
  BYTE buff[128];                       /* 0x80 - tail / default DMA buffer */
};

struct b_page *_base;                   /* Set up by _start()               */

/*-------------------------- setjmp.h -------------------------------------*/

typedef int jmp_buf[10];

extern int setjmp (/* jmp_buf */) __attribute__ ((returns_twice));
extern void longjmp (/* jmp_buf, int */) __attribute__ ((noreturn));

/*
 * The 386 half of the non-local goto.  Only the callee-saved registers, the
 * stack pointer and the return address need preserving; the compiler has
 * spilled anything else it cares about by the time setjmp is called.
 */

__asm__ (".text\n"
         "      .globl  setjmp\n"
         "setjmp:\n"
         "      movl    4(%esp), %eax\n"        /* env                      */
         "      movl    %ebx,  0(%eax)\n"
         "      movl    %esi,  4(%eax)\n"
         "      movl    %edi,  8(%eax)\n"
         "      movl    %ebp, 12(%eax)\n"
         "      leal    4(%esp), %ecx\n"        /* ESP as of the return     */
         "      movl    %ecx, 16(%eax)\n"
         "      movl    (%esp), %ecx\n"         /* return address           */
         "      movl    %ecx, 20(%eax)\n"
         "      xorl    %eax, %eax\n"
         "      ret\n"
         "      .globl  longjmp\n"
         "longjmp:\n"
         "      movl    4(%esp), %edx\n"        /* env                      */
         "      movl    8(%esp), %eax\n"        /* value                    */
         "      testl   %eax, %eax\n"           /* never return 0           */
         "      jnz     1f\n"
         "      incl    %eax\n"
         "1:\n"
         "      movl     0(%edx), %ebx\n"
         "      movl     4(%edx), %esi\n"
         "      movl     8(%edx), %edi\n"
         "      movl    12(%edx), %ebp\n"
         "      movl    16(%edx), %esp\n"
         "      jmp     *20(%edx)\n");

/*------------------------ memory and exit --------------------------------*/

extern char __bss_start[], _end[];      /* Laid down by user.ld             */

/*
 * Free memory runs from the end of BSS up to the stack, which CP/M-386 puts
 * at the top of the TPA.  That gap is measured in megabytes, and this
 * program cannot use it: its buffer indices are 16-bit.  Hand out what it
 * can address and leave the rest alone.
 */

#define HEAP_LIMIT 0xF000UL             /* Most a UWORD index can reach     */
#define STACK_MARGIN 0x1000UL           /* Keep clear of the stack          */

static char *_brk;                      /* Current break; 0 until first use */

static unsigned long
_membytes ()                            /* Bytes sbrk() can still hand out  */
{
  char probe;
  unsigned long top = (unsigned long) &probe;
  unsigned long bot = (unsigned long) (_brk ? _brk : _end);

  if (top < bot + STACK_MARGIN)
    return 0;

  top -= STACK_MARGIN;

  if ((top - bot) > HEAP_LIMIT)
    return HEAP_LIMIT;

  return top - bot;
}

static char *
sbrk (n)
int n;
{
  char *old;

  if (!_brk)
    _brk = (char *) (((unsigned long) _end + 15UL) & ~15UL);

  if (n < 0 || (unsigned long) n > _membytes ())
    return (char *) -1;

  old = _brk;
  _brk += n;

  return old;
}

static void
_exit (status)
int status;
{
  (void) status;

  for (;;)
    __BDOS (EXIT, (long) 0);            /* Never returns                    */
}

/*
 * Program entry.  The loader does not clear BSS - the image it reads has
 * none - so that is the first job, or every zero-initialised variable in
 * this program starts life as whatever the last transient left behind.
 */

extern void _start (void) __attribute__ ((section (".text._start")));
extern int _main ();                    /* Program proper; implicit int     */

void
_start ()
{
  char *p;

  for (p = __bss_start; p < _end; p++)
    *p = 0;

  {                                     /* Base page is at TPA+0; keep the  */
    void *z = (void *) 0;               /*   compiler from calling that a   */
    __asm__ ("" : "+r" (z));            /*   null pointer and folding it    */
    _base = (struct b_page *) z;
  }

  _brk = 0;
  _main ();
  _exit (0);
}

/****************************************************************************/
/*                       end of CP/M-386 runtime shim                       */
/****************************************************************************/

#define MPMPRODUCT      0x01                    /* requires MP/M            */
#define CPM3            0x30                    /* requires 3.0 CP/M        */
#define SECTSIZE        0x80                    /* sector size              */

/* CP/M-386's setjmp, longjmp, and sbrk come from the shim above. */

/****************************************************************************/
/*                                                                          */
/*        G L O B A L   V A R I A B L E S ,    D E F I N E S                */
/*        --------------------------------------------------                */
/*                                                                          */
/****************************************************************************/

#define SFCB    fcb1                            /* source fcb = default fcb */
                                                /*   (see below)            */
#define MARGIN  0x400                           /* # of bytes between stack */
                                                /*   & top of edit buffer   */

#define CTL_C   0x03                            /* Control C: reboot cmd    */
#define CTL_H   0x08                            /* backspace                */
#define TAB     0x09                            /* tab character            */
#define LF      0x0a                            /* Line-feed                */
#define CTL_L   0x0c                            /* Form-feed                */
#define CR      0x0d                            /* Carriage return          */
#define CTL_R   0x12                            /* insert mode repeat line  */
#define CTL_U   0x15                            /* insert mode line delete  */
#define CTL_X   0x18                            /* equivalent to CTL_U      */
#define CTL_Y   0x19                            /* used as 'break' command  */
#define ESC     0x1b                            /* escape character         */
#define ENDFILE 0x1a                            /* CP/M end of file (ctl-Z) */
#define POUND   '#'                             /* Taken to mean 65535      */
#define WHAT    '?'                             /* Error message (see above)*/
#define LCA     'a'                             /* lower case a             */
#define LCZ     'z'                             /* lower case z             */
#define RUBOUT  0x7f                            /* line kill during insert  */

#define PASSLEN 8                               /* Length of password       */

#define FORWARD         1
#define BACKWARD        0
#define MACSIZE         128                     /* max macro size           */
#define SCRSIZE         100                     /* scratch buffer size      */

#define RESTART         0                       /* Error codes used in      */
#define GET_LINE        1                       /*   longjmp calls to error */
#define OVERCOUNT       2                       /*   handler in main()      */
#define BADCOM          3
#define OVERFLOW        4
#define DISK_ERR        5
#define DIR_ERR         6
#define RESET           7

#define TIME            3500                    /* See time() function below*/

/****************************************************************************/
/*                                                                          */
/* The two following variables are initialized on startup to point to the   */
/* basepage structures.                                                     */
/*                                                                          */
/****************************************************************************/

struct fcbtab   *fcb1;                          /* 1st basepage FCB         */
char    *buff;                                  /* Basepage DMA buffer      */

jmp_buf main_env;                               /* Used in error recovery   */

char    *base;                                  /* Address of edit buffer   */
UWORD   max;                                    /* base[max] is top of mem  */
UWORD   maxm;                                   /* base[maxm] is last usable*/
UWORD   hmax;                                   /* max/2 (halfway up memory)*/

struct fcbtab   rfcb    =                       /* reader file control block*/
        {
                0,                              /* "disk"                   */
                { ' ', ' ', ' ', ' ',
                  ' ', ' ', ' ', ' ' },         /* filename                 */
                { 'L', 'I', 'B' },              /* filename extension       */
                0,                              /* extent                   */
                0,                              /* (reserved)               */
                0,                              /* (reserved)               */
                0,                              /* record count             */
                { 0 },                          /* (reserved)               */
                0                               /* random record            */
        };

UWORD   rbp;                                    /* index into read buffer    */

struct fcbtab   xfcb    =                       /* xfer file control block  */
        {
                0,
                { 'X', '$', '$', '$',
                  '$', '$', '$', '$' },         /* filename                 */
                { 'L', 'I', 'B' },              /* filename extension       */
                0,                              /* extent                   */
                0,                              /* (reserved)               */
                0,                              /* (reserved)               */
                0,                              /* record count             */
                { 0, 0, 0 },                    /* (more reserved stuff)    */
                0                               /* random record            */
        };

BYTE    xfcbext = 0;                            /* save extent for appends  */
LONG    xfcbrec = 0;                            /* save record for appends  */

char    xbuff[SECTSIZE];                        /* xfer buffer              */
UWORD   xbp;                                    /* xfer index               */

UWORD   nbuf;                                   /* number of buffers        */
UWORD   bufflength;                             /* nbuf * SECTSIZE          */

char    *sbuffadr;                              /* source buffer address    */

char    pwd[16];                                /* source password          */

struct fcbtab   dfcb;                           /* dest file control block  */

char    *dbuffadr;                              /* destination buff. address*/

UWORD   nsource;                                /* next source char index   */
UWORD   ndest;                                  /* next dest char index     */

struct fcbtab   tmpfcb;                         /* temp. for rename & delete*/

BOOLEAN newfile         = FALSE;                /* TRUE if no source file   */
BOOLEAN onefile         = TRUE;                 /* TRUE if o/p file=i/p file*/
BOOLEAN xferon          = FALSE;                /* TRUE if xfer active      */
BOOLEAN printsuppress   = FALSE;                /* TRUE if print suppressed */
BOOLEAN sys             = FALSE;                /* TRUE if system file      */
BOOLEAN protection      = FALSE;                /* Password protection mode */
BOOLEAN inserting;                              /* TRUE if inserting chars. */
BOOLEAN readbuff;                               /* TRUE if end of read buff.*/
BOOLEAN translate       = FALSE;                /* TRUE if xlation to u/c   */
BOOLEAN upper           = FALSE;                /* TRUE if global xlate u/c */
BOOLEAN lineset         = TRUE;                 /* TRUE if line #'s printed */
BOOLEAN has_bdos3       = FALSE;                /* TRUE if bdos vers. >= 3.0*/
BOOLEAN tail            = TRUE;                 /* TRUE if reading cmd tail */
BOOLEAN dot_found       = FALSE;                /* TRUE if . found in parse */

char    dtype   [3];                            /* destination file type    */

struct fcbtab   libfcb  =                       /* default lib name         */
        {
                0,
                { 'X', '$', '$', '$',
                  '$', '$', '$', '$' },         /* filename                 */
                { 'L', 'I', 'B' },              /* filename extension       */
                0,                              /* extent                   */
                0,                              /* (reserved)               */
                0,                              /* (reserved)               */
                0,                              /* record count             */
                { 0 },                          /* (reserved)               */
                0                               /* random record            */
        };

char    tempfl  []      = "$$$";                /* temporary file type      */

char    backup  []      = "BAK";                /* backup file type         */

int     column          = 0;                    /* console column position  */
int     scolumn         = 8;                    /* start column in "i" mode */

int     dcnt;                                   /* CP/M call return code    */

struct  con                                     /* console command buffer   */
        {
                UBYTE   maxlen, comlen;         /* 0..128: not a signed  */
                char    comline[128];           /*   char's range        */
                UBYTE   cbp;
        } combuf =
        {
                        0, 0, {0}, 0
        };

/* line counters */

UWORD   baseline;                               /* current line             */
UWORD   relline;                                /* relative line in typeout */

char    macro[MACSIZE];                         /* Macro command buffer     */
char    scratch[SCRSIZE];                       /* scratch space for F, N, S*/
UWORD   wbp, wbe;                               /* end of F , S or J string */
char    flag;                                   /* tracks current context   */
UWORD   mp;                                     /* index to end of macro    */
UWORD   mi;                                     /* "miscellaneous index"    */
UWORD   xp;                                     /* index into macro buffer  */

UWORD   mt;                                     /* number of times for macro*/

/* Global variables used by file parsing routines                           */

UWORD   ncmd    = 0;                            /* Index into command buffer*/

UWORD   distance;                               /* # of lines changed by cmd*/
int     direction;                              /* FORWARD or BACKWARD      */
int     chr;                                    /* Char being processed now */
int     delimiter;                              /* Latest delimiter char    */
UWORD   front, back, first, lastc;              /* indices to edit buffer   */

int     lpp     = 23;                           /* lines per "page" (screen)*/

char    pb [2]  =                               /* Argument to CPM 3 when   */
        {                                       /*   reading lines/page     */
                28, 0
        };

int     ver;                                    /* version number           */

char    *err_msg;                               /* File handling error msgs */
char    diskfull[]      = "disk full$";
char    dirfull[]       = "directory full$";
char    not_found[]     = "file not found$";
char    invalid[]       = "invalid filename$";
char    pwd_err[]       = "creating password$";
char    notavail[]      = "file not available$";

/****************************************************************************/
/*                                                                          */
/*              C P / M   I N T E R F A C E   R O U T I N E S               */
/*              ---------------------------------------------               */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*  I / O   S E C T I O N       */
                /*                              */
                /********************************/

                /********************************/
                /*                              */
                /*        P R I N T C H         */
                /*                              */
                /********************************/
VOID
printch(ch)                                     /* Single char console o/p  */
char ch;
{
        if (printsuppress)
                return;
        _conout(ch);
}

                /********************************/
                /*                              */
                /*        T T Y C H             */
                /*                              */
                /********************************/
VOID
ttych(ch)                                       /* Single char o/p, keeping */
char    ch;                                     /*  track of current column */
{
        if (ch >= ' ')
                column++;
        if (ch == LF)
                column = 0;
        printch(ch);
}

                /********************************/
                /*                              */
                /*       B A C K S P A C E      */
                /*                              */
                /********************************/
VOID
backspace()                                     /* Overprint last displayed */
{                                               /*  char with space, move   */
        if (column == 0)                        /*  cursor to that position */
                return;
        ttych(CTL_H);                           /* column = column - 1      */
        ttych(' ');                             /* column = column + 1      */
        ttych(CTL_H);                           /* column = column - 1      */
        column -= 2;
}

                /********************************/
                /*                              */
                /*      P R I N T A B S         */
                /*                              */
                /********************************/
VOID
printabs(ch)                                    /* Print a character        */
char    ch;
{
                                                /* Expand tabs (tabs are    */
        if (ch == TAB)                          /*   fixed at every eight   */
        {                                       /*   columns - 0, 8, 16 etc)*/
                do
                        ttych(' ');
                while (column & 7);
        }
        else
                ttych(ch);
}

                /********************************/
                /*                              */
                /*        G R A P H I C         */
                /*                              */
                /********************************/
BOOLEAN
graphic(ch)                                     /* TRUE if ch is printable  */
char    ch;
{
        if (ch >= ' ')
                return(TRUE);
        return ((ch==CR) | (ch==LF) | (ch==TAB));
}

                /********************************/
                /*                              */
                /*        P R I N T C           */
                /*                              */
                /********************************/
VOID
printc(ch)                                      /* Print character,         */
char    ch;                                     /*   presenting non-print   */
{                                               /*   control chars as       */
        if (! graphic(ch))                      /*   ^<letter>              */
        {                                       /* For example, control-A   */
                printabs('^');                  /* prints as ^A             */
                ch += '@';
        }
        printabs(ch);
}

                /********************************/
                /*                              */
                /*           C R L F            */
                /*                              */
                /********************************/
VOID
crlf()                                          /* Move to new line         */
{                                               /* (current column set to   */
        printc(CR);                             /* zero by ttych()          */
        printc(LF);
}

                /********************************/
                /*                              */
                /*           P R I N T          */
                /*                              */
                /********************************/
VOID
print(a)                                        /* Go to new line, then     */
char    *a;                                     /*   print a string         */
{
        crlf();
        _print(a);
}

                /********************************/
                /*                              */
                /*         P E R R O R          */
                /*                              */
                /********************************/
VOID
perror(a)                                       /* Print an error message   */
char    *a;
{
        print("\tError - $");
        _print(a);
        crlf();
}

                /********************************/
                /*                              */
                /*          O P E N             */
                /*                              */
                /********************************/
int
open(fcb)                                       /* Open a file - used for   */
struct fcbtab   *fcb;                           /*   library files          */
{                                               /* Restart ED from the top  */
        if (_open(fcb) == 0xff)                 /*   after printing error   */
        {                                       /*   message if operation   */
                flag = 'O';                     /*   fails                  */
                err_msg = not_found;
                longjmp(main_env, RESET);
        }
}

                /********************************/
                /*                              */
                /*         R E A D C O M        */
                /*                              */
                /********************************/
VOID
readcom()                                       /* Read console input line  */
{                                               /*   into combuf            */
        combuf.maxlen = 128;
        _conbuf(&combuf);
}

                /********************************/
                /*                              */
                /*      B R E A K _ K E Y       */
                /*                              */
                /********************************/
BOOLEAN
break_key()                                     /* Return TRUE if character */
{                                               /*   entered at console is  */
        return ((_constat() != 0)               /*   CTL_Y.  If not CTL_Y   */
           && (_conio(0xff) == CTL_Y));         /*   or no char entered,    */
}                                               /*   return FALSE.  Any     */
                                                /*   character read is lost */

                /********************************/
                /*                              */
                /*          M O V E             */
                /*                              */
                /********************************/
VOID
move(count, source, dest)                       /* Move count bytes from    */
register char   *source, *dest;                 /*   source to dest.  Useful*/
register UWORD  count;                          /*   where types not suited */
{                                               /*   for struct assignment  */
        for (; count--; *dest++ = *source++);
}

                /********************************/
                /*                              */
                /*      W R I T E _ X F C B     */
                /*                              */
                /********************************/
VOID
write_xfcb(fcb)                                 /* Write an extended FCB    */
struct fcbtab   *fcb;
{
        move(8, pwd, &pwd[8]);
        if (_set_xfcb(fcb) == 0xff)
                perror(pwd_err);
}

                /********************************/
                /*                              */
                /*         R E B O O T          */
                /*                              */
                /********************************/
VOID
reboot()                                        /* CP/M warm start following*/
{                                               /*   error or on request    */
        if (xferon == TRUE)                     /*   (user enters control-C)*/
                _delete(&libfcb);
        _exit(0);
}

                /********************************/
                /*                              */
                /*            T I M E           */
                /*                              */
                /********************************/

VOID                                            /* Provide a delay of about */
time()                                          /*   25 milliseconds.  Tune */
{                                               /*   for processor, compiler*/
        register UWORD  downer;                 /*   and clock rate with    */
                                                /*   defined value for TIME */
        downer = TIME;
        while (downer--);
}

/****************************************************************************/
/*                                                                          */
/*              * * *   S U B R O U T I N E S   * * *                       */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*  I / O   B U F F E R I N G   */
                /*                              */
                /********************************/

                /********************************/
                /*                              */
                /*         A B O R T            */
                /*                              */
                /********************************/
VOID
abort(a)                                        /* Print an error message   */
char    *a;                                     /*   then abort ED          */
{
        perror(a);
        reboot();
}

                /********************************/
                /*                              */
                /*           F E R R            */
                /*                              */
                /********************************/
VOID
ferr()                                          /* Abort when directory full*/
{
        _close(&dfcb);                          /* Try to close file so it  */
        abort(dirfull);                         /*   can be recovered later */
}

                /********************************/
                /*                              */
                /*        S E T P W D           */
                /*                              */
                /********************************/

                                                /* Set password addr (only  */
                                                /*   CP/M 3 supports this)  */
                                                /* NOTE: this is a macro    */

#define setpwd()        if (has_bdos3) _setdma(pwd);

                /********************************/
                /*                              */
                /*     D E L E T E _ F I L E    */
                /*                              */
                /********************************/
VOID
delete_file(fcb)                                /* Delete the file describ- */
struct fcbtab   *fcb;                           /*   -ed by the arument     */
{
        setpwd();
        _delete(fcb);
}

                /********************************/
                /*                              */
                /*      R E N A M E _ F I L E   */
                /*                              */
                /********************************/
VOID
rename_file(fcb)                                /* Rename the file describ- */
struct fcbtab   *fcb;                           /*   by the argument        */
{
        delete_file((struct fcbtab *) fcb->resvd);/* *** Delete any existing*/
        setpwd();                               /*   file of same name ***  */
        _rename(fcb);                           /* Now do rename            */
}

                /********************************/
                /*                              */
                /*      M A K E _ F I L E       */
                /*                              */
                /********************************/
VOID
make_file(fcb)                                  /* Create the file describ- */
struct fcbtab   *fcb;                           /*   by the argument        */
{
        delete_file(fcb);                       /* *** Delete any existing  */
        setpwd();                               /*  file of same name ***   */
        dcnt = _create(fcb);                    /* Now create file          */
}

                /********************************/
                /*                              */
                /*           F I L L            */
                /*                              */
                /********************************/
VOID
fill(s, f, c)                                   /* Fill string starting at  */
register char   *s, f;                          /*   s for c bytes with     */
register int    c;                              /*   character s            */
{
        for (; c--; *s++ = f);
}

/****************************************************************************/
/*                                                                          */
/*              F I L E   H A N D L I N G   R O U T I N E S                 */
/*              -------------------------------------------                 */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*        S E T T Y P E         */
                /*                              */
                /********************************/

VOID
settype(fcb, a)                                 /* Set type of destination  */
struct fcbtab   *fcb;                           /*   file afcb to string    */
char    *a;                                     /*   pointed at by a        */
{
        move(3, a, fcb->ftype);
}

                /********************************/
                /*                              */
                /*        S E T X D M A         */
                /*                              */
                /********************************/

                                                /* Set DMA to transfer buff */
                                                /* NOTE: this is a macro    */
#define setxdma()       _setdma(xbuff)

                /********************************/
                /*                              */
                /*      F I L L S O U R C E     */
                /*                              */
                /********************************/
VOID
fillsource()                                    /* Fill the source buffer   */
{                                               /*   from file described by */
        register int j;                         /*   fcb1 (source file)     */

        nsource = 0;                            /* Point to start of buffer */
        for (j = 0; j <= nbuf; j++)             /* Fill buffer until no more*/
        {                                       /*   room or until end of   */
                _setdma(&sbuffadr[nsource]);    /*   file reached           */
                if ((dcnt = _s_read(fcb1)) != 0)
                {
                        if (dcnt > 1) ferr();   /* Non-EOF error: report it */
                        sbuffadr[nsource] = ENDFILE;
                        break;
                }
                else nsource += SECTSIZE;       /* Point to end of new data */
        }
        nsource = 0;                            /* Point to buff start again*/
}

                /********************************/
                /*                              */
                /*       G E T S O U R C E      */
                /*                              */
                /********************************/
int
getsource()                                     /* Get next character in    */
{                                               /*   source file            */
        char    b;

        if (newfile)                            /* New file: no next char   */
                return (ENDFILE);
        if (nsource >= bufflength)              /* Char not in buffer: read */
                fillsource();                   /*   next bufferful from src*/
        if ((b = sbuffadr[nsource]) != ENDFILE) /* Not at end of file:      */
                nsource++;                      /*   point to next char     */
        return (b);                             /* Return char (may be      */
}                                               /*   ENDFILE)               */

                /********************************/
                /*                              */
                /*      E R A S E _ B A K       */
                /*                              */
                /********************************/
BOOLEAN
erase_bak()                                     /* Try to free disk space   */
{                                               /*   by erasing back-up file*/
        if (onefile && newfile)                 /* Called when write of     */
        {                                       /*   output file fails      */
                tmpfcb = dfcb;                  /* Back-up is open: can't   */
                                                /*   change its own fcb, so */
                settype(&tmpfcb, backup);       /*   use a copy instead     */
                delete_file(&tmpfcb);           /* Delete .BAK file         */
                if (dcnt != 255) return (TRUE); /* TRUE if deletion OK      */
        }
        return (FALSE);                         /* File not deleted         */
}

                /********************************/
                /*                              */
                /*       W R I T E D E S T      */
                /*                              */
                /********************************/
VOID
writedest()                                     /* Write output buffer      */
{                                               /* Write only whole sectors:*/
        register int    n, save_ndest;          /*   blocking is done       */
                                                /*   elsewhere              */

        if ((n = ndest / SECTSIZE) == 0) return;/* How many sectors to write*/
                                                /* Return if none           */
        save_ndest = ndest;                     /* save for error recovery  */
        ndest = 0;
        while (n--)                             /* Write out n sectors      */
        {
            FOREVER                             /* For each sector, write   */
            {                                   /*   and move onto next if  */
                _setdma(&dbuffadr[ndest]);      /*   successful.            */
                if (_s_write(&dfcb) == 0)
                    break;
                if (! erase_bak ())             /* If write fails, try to   */
                {                               /*   make space & try again */
                    if (ndest != 0)             /* If erase fails, move     */
                    {                           /*   unwritten data to base */
                                                /*   of buffer.  User must  */
                                                /*   pick up the pieces     */
                        ndest = save_ndest - ndest;
                        move(ndest, &dbuffadr[ndest], dbuffadr);
                        longjmp(main_env, DISK_ERR);
                    }
                }
            }
            ndest += SECTSIZE;                  /* Written OK: bump pointer */
        }
        ndest = 0;                              /* Point to base of buffer  */
}

                /********************************/
                /*                              */
                /*         P U T D E S T        */
                /*                              */
                /********************************/
VOID
putdest(b)                                      /* Put a character in the   */
char    b;                                      /*   buffer.  If buffer is  */
{                                               /*   already full, flush it */
        if (ndest >= bufflength) writedest();   /*   onto disk first        */
        dbuffadr[ndest++] = b;
}

                /********************************/
                /*                              */
                /*        P U T X F E R         */
                /*                              */
                /********************************/
VOID
putxfer(c)                                      /* Put a character into the */
char    c;                                      /*   transfer buffer.  If   */
{                                               /*   buffer initially full, */
        if (xbp >= SECTSIZE)                    /*   flush it to disk first */
        {
            FOREVER                             /* Write buffer to disk     */
            {
                setxdma();
                xfcbext = xfcb.extent;          /* Save current position    */
                xfcbrec = xfcb.record;          /*   for later appends      */
                if (_s_write(&xfcb) == 0)       /* Leave loop if write OK   */
                    break;
                if (! erase_bak ())             /* Not OK: try to make space*/
                    longjmp(main_env, DISK_ERR);/* Erase failure - give up  */
            }                                   /* NOTE: xfer file left open*/

            xbp = 0;                            /* Point to buffer base     */
        }
        xbuff[xbp++] = c;                       /* Put character in buffer  */
}

                /********************************/
                /*                              */
                /*     C L O S E _ X F E R      */
                /*                              */
                /*   (allows LIB files to be    */
                /*    saved for future edits)   */
                /*                              */
                /********************************/
VOID
close_xfer()                                    /* Flush transfer buffer to */
{                                               /*   disk (whether buff full*/
        register int    j;                      /*   or not) and close file */
                                                /* Fill empty part with EOF */
        for (j = xbp; j++ <= SECTSIZE; putxfer(ENDFILE));
        _close(&xfcb);
}

                /********************************/
                /*                              */
                /*   C O M P A R E _ X F E R    */
                /*                              */
                /********************************/

BOOLEAN
compare_xfer()                                  /* Return TRUE if xfcb and  */
{                                               /*   rfcb are accessing the */
        register int j;                         /*   same file              */
                                                /* Compare drive, name,     */
        for (j = 12; j--;)                      /*   and type               */
                if (((char *) &xfcb)[j] != ((char *) &rfcb)[j])
                        return (FALSE);
        return (TRUE);
}

                /********************************/
                /*                              */
                /*    A P P E N D _ X F E R     */
                /*                              */
                /********************************/
VOID
append_xfer()                                   /* Restore xfer file        */
{                                               /*   to record last written */
        xfcb.extent = xfcbext;                  /*   by putxfer; read that  */
        open(&xfcb);                            /*   record into xfer buff; */
        xfcb.record = xfcbrec;                  /*   leave current record   */
        setxdma();                              /*   pointer on that record */
        if (_s_read(&xfcb) == 0)
        {
                xfcb.record = xfcbrec;
                for (xbp = 0; xbp++ < SECTSIZE;)
                        if (xbuff[xbp] == ENDFILE) return;
        }                                       /* On exit xbp points to    */
}                                               /*   ENDFILE in buffer      */

/****************************************************************************/
/*                                                                          */
/*          R O U T I N E S   U S E D   T O   E N D   E D I T               */
/*          -------------------------------------------------               */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*          M O V E U P         */
                /*                              */
                /********************************/
VOID
moveup(fcb)                                     /* Move filename up fcb to  */
struct fcbtab   *fcb;                           /*   correct position for   */
{                                               /*   use by rename call     */
        move(16, (char *) fcb, fcb->resvd);
}

                /********************************/
                /*                              */
                /*          F I N I S           */
                /*                              */
                /********************************/
VOID
finis()                                         /* Finish edit, close files,*/
{                                               /*   rename temp to output  */
        int     lrbc;                           /* CP/M-386: exact length of*/
                                                /*   the text, taken before */
                                                /*   the padding goes on    */

        lrbc = ndest % SECTSIZE;

        while (ndest % SECTSIZE)                /* Fill to end of current   */
                putdest(ENDFILE);               /*   record with ENDFILEs   */
        writedest();                            /* Write the record         */

        if (!newfile)                           /* Close the source file    */
                _close(SFCB);
        _close(&dfcb);                          /* Close temp dest file     */
        if (dcnt == 255)                        /* Report error and give up */
                ferr();                         /*   on failure             */
        if (sys)                                /* If output is to be system*/
        {                                       /*   file, set bit to say so*/
                dfcb.ftype[1] |= 0x80;
                setpwd();
                _set_att(&dfcb);
        }
        if (onefile)                            /* If source and backup are */
        {                                       /*   same file, retype src  */
                moveup(SFCB);                   /*   to .BAK (this is case  */
                                                /*   if no o/p file given)  */
                settype((struct fcbtab *) SFCB->resvd, backup);
                rename_file(SFCB);
        }

        moveup(&dfcb);                          /* Retype dest file to same */
                                                /*   type as source         */
        settype((struct fcbtab *) dfcb.resvd, dtype);
        rename_file(&dfcb);
                                                /* CP/M-386: record how much*/
                                                /*   of the last record is  */
                                                /*   text.  The file has its*/
                                                /*   final name by now.     */
        set_lrbc((int) dfcb.resvd[0], (BYTE *) &dfcb.resvd[1], lrbc);
}

/****************************************************************************/
/*                                                                          */
/*                  C O M M A N D   R O U T I N E S                         */
/*                  -------------------------------                         */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*        P R T N M A C         */
                /*                              */
                /********************************/

                                                /* Print character if not   */
                                                /*   part of macro expansion*/
                                                /* NOTE: this is a macro    */
#define prtnmac(ch)     if (! mp) printc(ch);

                /********************************/
                /*                              */
                /*       L O W E R C A S E      */
                /*                              */
                /********************************/

                                                /* Return TRUE if character */
                                                /*   is lowercase           */
                                                /* NOTE: this is a macro    */
#define lowercase(ch)   ((LCA <= ch) && (ch <= LCZ))

                /********************************/
                /*                              */
                /*          U C A S E           */
                /*                              */
                /********************************/
int
ucase(ch)                                       /* Translate character to   */
int     ch;                                     /*   upper case if it is a  */
{                                               /*   lower case letter      */
        return (lowercase(ch) ? ch & 0x5f : ch);/*   lower case letter      */
}

                /********************************/
                /*                              */
                /*       G E T P A S S W D      */
                /*                              */
                /********************************/

getpasswd()                                     /* Get password from user  */
{
        char    c;
        register int    j;

        crlf();                                 /* Ask for password         */
        print("Password? $");
        FOREVER                                 /* Until password obtained  */
        {
retry:
            fill(pwd, ' ', PASSLEN);            /* Fill password with spaces*/
            for (j = 0; j < PASSLEN;)           /* Read PASSLEN characters  */
            {                                   /*   without echoing        */
                                                /* What did user enter?     */
                switch (c = ucase((int) _conio(0xff)))
                {
                  case CR:                      /* All done                 */
                    goto way_out;
                  case CTL_X:                   /* Start over               */
                    goto retry;
                  case CTL_H:
                    if (j > 0)
                        pwd[--j] = ' ';         /* Prepare to reenter       */
                    break;                      /*   previous character     */
                  case CTL_C:
                    reboot();                   /* Exit from ED at once     */
                                                /* reboot() does not return */
                                                /* FALLTHROUGH              */
                  default:
                    if (c >= ' ')               /* Printable?  Put in passwd*/
                        pwd[j++] = c;
                }
            }
        }
way_out:
        c = (char) break_key();                 /* Clear raw I/O mode       */
}

                /********************************/
                /*                              */
                /*         U T R A N            */
                /*                              */
                /********************************/
int
utran(c)                                        /* Translate to upper case  */
int     c;                                      /*   if translate flag set  */
{                                               /*   (for U/C only terminal)*/
        if (c == ESC) return (ENDFILE);         /* Translate ESC to ENDFILE */
        return ((translate) ? ucase(c) : c);
}

                /********************************/
                /*                              */
                /*     P R I N T V A L U E      */
                /*                              */
                /********************************/
VOID
printvalue(v)                                   /* Print the line number as */
UWORD   v;                                      /*   unsigned decimal value */
{
        register BOOLEAN        zero;
        register int    d, k;

        zero = FALSE;                           /* Blank leading zeros      */
        for (k = 10000; k != 0; k /= 10)        /* Print 5 digits           */
        {
                d = v / k;
                v %= k;
                if ((zero) || (d != 0))         /* Print first non-zero     */
                {                               /*   and all following      */
                        zero = TRUE;            /*   digits                 */
                        printc('0' + d);
                }
                else printc(' ');
        }
}

                /********************************/
                /*                              */
                /*      P R I N T L I N E       */
                /*                              */
                /********************************/
VOID
printline(v)                                    /* Print line number:       */
UWORD   v;                                      /*   "12345:  " if inserting*/
{                                               /*   else "12345: *"        */
        if (! lineset) return;                  /* Do nothing if no number  */
        printvalue(v);                          /*   wanted                 */
        printc(':');
        printc(' ');
        printc((inserting) ? ' ' : '*');
}

                /********************************/
                /*                              */
                /*     P R I N T B A S E        */
                /*                              */
                /********************************/

                                                /* Print current line number*/
                                                /*   (baseline)             */
                                                /* NOTE: this is a macro    */
#define printbase()     printline(baseline)

                /********************************/
                /*                              */
                /*   P R I N T N M B A S E      */
                /*                              */
                /********************************/

                                                /* Print current line number*/
                                                /*   (baseline) if not      */                                                  /*   expanding macro        */
                                                /* NOTE: this is a macro    */
#define printnmbase()   if (! mp) printline(baseline)

                /********************************/
                /*                              */
                /*          G E T C M D         */
                /*                              */
                /********************************/

                                                /* Get next char from cmd   */
                                                /*   buffer or CR if none   */
                                                /* NOTE: this is a macro    */
#define getcmd()        ((buff[ncmd + 1] != 0) ? buff[++ncmd] : CR)

                /********************************/
                /*                              */
                /*           R E A D C          */
                /*                              */
                /********************************/
int
readc()                                         /* Read next input character*/
{                                               /* This may be from         */
                                                /*   a) macro expansion     */
                                                /*   b) keyboard in insert  */
                                                /*   c) command line        */
        if (mp != 0)                            /* From macro expansion?    */
        {
            if (break_key())                    /* CTL_Y stops macro        */
                longjmp(main_env, OVERCOUNT);
            if (xp >= mp)                       /* Reached end of macro?    */
            {
                if ((mt != 0) && (--mt == 0))   /* Count down repeats       */
                    longjmp(main_env, OVERCOUNT);/*Macro done               */
            xp = 0;                             /* Point to macro start     */
            }
            return (utran(macro[xp++]));        /* Return char from macro   */
        }

        if (inserting)                          /* Inserting? Just read     */
                return (utran((int) _conio(0xff)));     /* NOTE: no echo    */

        if (readbuff)                           /* Take from command buffer */
        {                                       /* Get new line if empty    */
            readbuff = FALSE;
            if (lineset && (column == 0))       /* Need a line number?      */
            {
                if (back >= maxm)               /* Does a number make sense?*/
                    printline(0);               /* No: print spaces         */
                else
                    printbase();
            }
            else
                printc('*');                    /* No number: just print "*"*/
            readcom();                          /* Read a line              */
            combuf.cbp = 0;                     /* Point at first char      */
            printc(LF);
            column = 0;
        }
                                                /* Buffer loaded: get char  */
        if ((readbuff = (combuf.cbp == combuf.comlen)))
            combuf.comline[combuf.cbp] = CR;    /* If last, return CR;      */
                                                /*   reload buffer next time*/
        return (utran(combuf.comline[combuf.cbp++]));
}

                /********************************/
                /*                              */
                /*          G E T _ U C         */
                /*                              */
                /********************************/
VOID
get_uc()                                        /* Get upper case char from */
{                                               /*   buffer or command line */
        chr = ucase((tail) ? getcmd() : readc());
}

                /********************************/
                /*                              */
                /*           D E L I M          */
                /*                              */
                /********************************/

BOOLEAN
delim()                                         /* TRUE if chr is delimiter */
{                                               /* Complains about wildcards*/
        static char del[] =                     /* Table of delimiters      */
                {
                CR, ENDFILE,' ',',','.',';','=',':','!','=','[',']','*','?'
            /*   0    1      2   3   4   5   6   7   8   9  10  11  12  13*/
                };

        for (delimiter = 0; delimiter < sizeof del; delimiter++)
        {
                if (chr == del[delimiter])
                {
                        if (delimiter > 11)     /* * or ?                   */
                                perror("cannot edit wildcard filename$");
                        return (TRUE);
                }
        }
        return (FALSE);
}

                /********************************/
                /*                              */
                /*      P A R S E _ F C B       */
                /*                              */
                /********************************/
BOOLEAN
parse_fcb(fcbadr)                               /* Parse input file name;   */
struct fcbtab   *fcbadr;                        /*   set up in fcb if valid */
{                                               /*   return FALSE if invalid*/
        register int    j;                      /* There must be an easier  */
        BOOLEAN         pflag, colon_found;     /*   way to do this...      */

        pflag = FALSE;                          /* No non-delimiters found  */
        flag = TRUE;                            /* Filename is valid so far */
        dot_found = colon_found = FALSE;        /* No drive, type found yet */
        get_uc();
        if ((chr == CR) || (chr == ENDFILE))    /* Empty line or end of file*/
            return (FALSE);                     /*    does not change fcb   */

        fill((char *) fcbadr->fname, ' ', 11);  /* File name to spaces      */
        fill((char *) &fcbadr->extent, 0, 21);   /* Rest of fcb to nulls    */
        while (chr == ' ')                      /* Skip leading spaces      */
            get_uc();
                                                /* Parse the filename       */
        fcbadr->drive = j = 0;                  /* Guess it's default drive */

        FOREVER                                 /* Find drive and/or name   */
        {
            while (! delim())
            {
                if (j >= 8) goto err;           /* Too long for filename    */
                fcbadr->fname[j++] = chr;       /* Put into fcb             */
                pflag = TRUE;
                get_uc();
            }
            if (chr != ':')                     /* Was this a drive name?   */
                break;                          /* No.  Must be file name   */

            if ((j != 1)                        /* Drive not 1 char: invalid*/
               || colon_found           /* Second colon: error      */
               || ((fcbadr->drive = fcbadr->fname[0] - 'A' + 1) > 16))
                goto err;                       /* drive > P : invalid      */

            fcbadr->fname[0] = ' ';             /* Leave fname blank        */
            colon_found = TRUE;         /* Go see if there is a name*/
            j = 0;
            get_uc();
        }

        if (chr == '.')                         /* Start of extension       */
        {
            j = 0;
            dot_found = TRUE;
            get_uc();
            while (! delim())
            {
                if (j >= 3) goto err;           /* Extension too long       */
                fcbadr->ftype[j++] = chr;
                pflag = TRUE;
                get_uc();
            }
        }
        if (chr == ';')                         /* Start of password        */
        {
            fill(fcbadr->resvd,' ', 8);         /* Initialize to spaces     */
            j = 0;
            get_uc();
            while (! delim())
            {
                if (j >= 8)
                    goto err;                   /* Password too long        */
                fcbadr->resvd[j++] = chr;
                pflag = TRUE;
                get_uc();
            }
            move(8, fcbadr->resvd, pwd);        /* Make copy for ED         */
        }

        if ((delimiter <= 3) && pflag)          /* Everything OK?           */
            return (pflag);
                                                /* Not CR, ENDFILE, space   */
                                                /*   or comma: error        */
err:
        perror(invalid);                        /* Complain about invalid   */
        return (flag = FALSE);                  /*   filename               */
}

                /********************************/
                /*                              */
                /*        C O P Y D E S T       */
                /*                              */
                /********************************/

                                                /* Copy source name to      */
                                                /*   destination            */
                                                /* NOTE: this is a macro    */

#define copydest()      move(16, (char *) fcb1, (char *) &dfcb)

                /********************************/
                /*                              */
                /*        S E T D E S T         */
                /*                              */
                /********************************/
VOID
setdest()                                       /* Set up the destination   */
{                                               /*   file                   */
        register int    j, k;                   /* Onefile == TRUE on entry */

        k = SFCB->drive;                        /* Get source drive         */
        if (! tail)                             /* No command line params:  */
        {                                       /*   get file name from user*/
                print("Output file? $");
                readcom();
                combuf.cbp = readbuff = 0;
                printc(LF);
        }
        if (parse_fcb(&dfcb)                    /* If name is valid and    */
           && (dfcb.fname[0] != ' '))           /*   non-blank, is it same */
        {                                       /*   as source name?       */
                k = dfcb.drive;                 /* Save dest drive name    */
                for (j = 0; j < 11; j++)
                        if (! (onefile = (fcb1->fname[j] == dfcb.fname[j])))
                                break;
        }
        if (onefile)                            /* Same name?  Copy it.    */
        {
                copydest();
                dfcb.drive = k;                 /* Plug in drive name      */
        }
        move(3, dfcb.ftype, dtype);             /* save destination type   */
}

                /********************************/
                /*                              */
                /*       S E T R D M A          */
                /*                              */
                /********************************/

                                                /* Set DMA address to read  */
                                                /*   library file           */
                                                /* NOTE: this is a macro    */

#define setrdma()       _setdma(buff)

                /********************************/
                /*                              */
                /*        R E A D F I L E       */
                /*                              */
                /********************************/
int
readfile()                                      /* Read a character from a  */
{                                               /*   library file           */
        if (rbp >= SECTSIZE)                    /* Refill buffer if end has */
        {                                       /*   been reached           */
                setrdma();
                if (_s_read(&rfcb) != 0)
                        return (ENDFILE);       /* End of libray file       */
                rbp = 0;                        /* Point to buffer start    */
        }
        return (utran(buff[rbp++]));            /* Return current character */
}

                /********************************/
                /*                              */
                /*     W R I T E _ X F E R      */
                /*                              */
                /********************************/
VOID
wrt_xfer()                                      /* Write lines to transfer  */
{                                               /*   file                   */
        register int    j;

        xbp = 0;                                /* Initialize transfer index*/
        if (xferon && compare_xfer())           /* If transfer file in use  */
                append_xfer();                  /*   & same as read file,   */
                                                /*   prepare to append to it*/
        else
        {                                       /* Otherwise, create it:    */
                xferon = TRUE;                  /* Name is that of read file*/
                move(12, (char *) &rfcb, (char *) &xfcb);
                xfcbext = xfcb.extent = 0;
                xfcbrec = xfcb.record = 0;
                make_file(&xfcb);
                if (dcnt == 0xff)               /* Can't create: give up    */
                        longjmp(main_env, DIR_ERR);
        }

        setlimits();                            /* Map line cnt to pointers */
        for (j = first; j <= lastc; putxfer(base[j++]));/* Write out to file*/
}

/****************************************************************************/
/*                                                                          */
/*                      I N I T I A L I Z A T I O N                         */
/*                      ---------------------------                         */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*          S E T U P           */
                /*                              */
                /********************************/
VOID
setup()                                         /* Start up edit session    */
{
        int     error_code;

        SFCB->extent =  SFCB->s2 = 0;
        SFCB->record = 0;                       /* Open the source file    */
        if (has_bdos3)
        {
                _ret_errors(0xfe);              /* Set error mode           */
                setpwd();
        }
        error_code = _open(SFCB);               /* Get source file name     */
        if (has_bdos3)                          /* Has extended BDOS errors */
        {
                _ret_errors(0);                 /* Reset error mode         */
                if (error_code == 0x7ff)        /* Password required?       */
                {
                        getpasswd();            /* Let them enter pwd       */
                        crlf();
                        crlf();
                        setpwd();               /* Set DMA to pwd           */
                                                /* Retry the open           */
                        error_code = _open(fcb1);
                }
                if (((error_code & 0xff) == 0xff)/* Abort unless open       */
                   && ((error_code >> 8) != 0)) /*   successful or file not */
                        abort(notavail);        /*   found                  */
        }
        dcnt = error_code & 0xff;
        if (onefile)                            /* If output same as input  */
        {
                if (fcb1->ftype[0] & 0x80)      /* Check for read only      */
                        abort("File is read/only$");
                if (fcb1->ftype[1] & 0x80)      /*   and system attribute   */
                {
                        if (fcb1->fname[7] & 0x80)
                                dcnt = 255;     /* User 0 file so create    */
                        else
                                sys = TRUE;
                }
        }

        if (dcnt == 255)                        /* A new file is needed     */
        {
                if (! onefile)                  /* Two files given, but     */
                        abort(not_found);       /*   can't open source      */
                newfile = TRUE;                 /* Onefile: must be new     */
                print("new file$");
                crlf();
        }

        settype(&dfcb, tempfl);                 /* Make a new .$$$ temp file*/
        dfcb.extent = 0;
        make_file(&dfcb);
        if (dcnt == 255) ferr();                /* Couldn't do it: error    */

        if (protection != 0)                    /* Create password if       */
        {                                       /*   necessary              */
                dfcb.extent = protection | 1;   /* Set password             */
                setpwd();
                write_xfcb(&dfcb);
        }
        dfcb.record = 0;                        /* Next record is zero      */
        dfcb.extent = 0;                        /*   in file extent zero    */

        nsource = bufflength;                   /* Initialize edit buffer   */
        ndest = 0;
        baseline = 1;                           /* Start with line 1        */
}

/****************************************************************************/
/*                                                                          */
/*             E D I T   B U F F E R   M A N A G E M E N T                  */
/*             -------------------------------------------                  */
/*                                                                          */
/*      Some words about how ED manages its edit buffer.  This is best      */
/*      understood by remembering the original ED documentation ("ED: a     */
/*      Context Editor for the CP/M Disk System", DRI 1976, 1978.)  This    */
/*      shows the following conceptual picture of the memory buffer (the    */
/*      text shown is arbitrary):                                           */
/*                                                                          */
/*      First line      ABCDEFGHIJKLMNOP<CR><LF>                            */
/*                                                                          */
/*      Second line     QRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234<CR><LF>    */
/*                                                                          */
/*      Current line    567890ABCDEFGHIJKLMNOPRSTUVWXYZabcdef<CR><LF>       */
/*                                                    ^                     */
/*                                            Current | Pointer             */
/*                                                                          */
/*      Last line       ghijklmnopqrstuvwxyz<CR><LF>                        */
/*                                                                          */
/*      The buffer seems to the user like a contiguous block of text        */
/*      within which the Current Pointer can be moved at will, either       */
/*      explicitly (for example, with the L command) or as a side effect    */
/*      of a command (X deletes the character on which the pointer rests,   */
/*      in effect moving the pointer to the next character.)                */
/*                                                                          */
/*      Although the text seems to the user to be stored contiguously, in   */
/*      fact it is not: it is stored as two separate blocks.  Text from     */
/*      the first character on the first line up to BUT NOT INCLUDING the   */
/*      character on which the Current Pointer rests is held as a contiguous*/
/*      block at the base of the edit buffer.  The remaining text,          */
/*      including the character on which the pointer rests, is held as a    */
/*      contiguous block at the top of the buffer.  If the Current Pointer  */
/*      is moved forward (towards the end of the text) characters are moved */
/*      from the beginning of the block at the top of the buffer to the     */
/*      end of the block at the bottom of the buffer until the first        */
/*      character in the top block is the one on which the pointer rests.   */
/*      Similarly, if the pointer is moved backward, characters are moved   */
/*      from the end of the block at the bottom of the buffer to the        */
/*      beginning of the block at the top of the buffer.                    */
/*                                                                          */
/*      The data movement is accomplished by mover().  Deletion of          */
/*      characters before or after the current pointer does not require     */
/*      any data movement.  Instead an adjustment of the pointers to the    */
/*      end of the bottom block or to the beginning of the top block is     */
/*      accomplished by set_ptrs().                                         */
/*                                                                          */
/*      Many of ED's operations act over a given number of lines rather     */
/*      than of characters.  This is accomplished by having setimits()      */
/*      convert the line count to a character count before the primitive    */
/*      operation takes place.  Mover() and set_ptrs() keep track of        */
/*      current line number by counting line-feeds as they are encountered. */
/*      Note that deleting data after the Current Pointer does not affect   */
/*      the current line number.                                            */
/*      Most ED operations affect either data before the Current Pointer    */
/*      (direction backward), on or after it (direction forward) but not    */
/*      both.  The two operations which operate somewhere else - Append     */
/*      and Write - temporarily compact the data to a single contiguous     */
/*      block at the bottom or top of the buffer respectively.  When        */
/*      complete, they split the data in two again.                         */
/*                                                                          */
/*      You've read the book.  Now see the picture.  The Current pointer is */
/*      resting on the second 'Z' in the buffer.  Dots represent the free   */
/*      area:                                                               */
/*                                                                          */
/*      +-------------------------------+  <--  base points here            */
/*      |<LF>ABCDEFGHIJKLMNOP<CR><LF>QRS|                                   */
/*      |TUVWXYZabcdefghijklmnopqrstuvwx|                                   */
/*      |yz1234<CR><LF>567890ABCDEFGHIKL|                                   */
/*      |MNOPQRSTUVWXY..................|  <--  base[front] is first char   */
/*      |...............................|       in free area (ie first dot) */
/*                                                                          */
/*      |...............................|                                   */
/*      |...............Zabcdef<CR><LF>g|  <--  base[back] is current char  */
/*      |hijklmnopqrstuvwxyz<CR><LF><\0>|                                   */
/*      +-------------------------------+                                   */
/*                                                                          */
/*      The leading line feed and terminating null are put into the buffer  */
/*      by ED.  They are not part of the user's text.                       */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*      D I S T N Z E R O       */
                /*                              */
                /********************************/
BOOLEAN
distnzero()                                     /* If non-zero distance     */
{                                               /*   decrement distance and */
        if ( distance != 0)                     /*   return TRUE            */
        {
                distance--;
                return (TRUE);
        }
        return (FALSE);                         /* distance == 0            */
}

                /********************************/
                /*                              */
                /*      S E T L I M I T S       */
                /*                              */
                /********************************/
VOID
setlimits()                                     /* Set memory limits over   */
{                                               /*   which command operates */
        register int j, k, limit, m;            /*   using distance and     */
        BOOLEAN middle, looping;                /*   direction              */

        relline = 1;                            /* Relative line count      */
        if (direction == BACKWARD)              /* Working down buffer?     */
        {
                distance++;                     /* Account for current line */
                j = front;
                limit = 0;
                k = 0xffff;
        }
        else
        {
                j = back;
                limit = maxm;
                k = 1;
        }
        looping = TRUE;                         /* The algorithm scans the  */
        while (looping)                         /*   buffer until it has    */
        {                                       /*   found required number  */
                while ((middle = (j != limit))  /*   of line feeds or until */
                      && (base[m = j + k] != LF))/*  buffer limits reached  */
                        j = m;                  /* If operation covers      */
                looping = (--distance != 0);    /*   lines outside buffer,  */
                if (! middle)                   /*   distance is non-zero   */
                {                               /*   on exit                */
                        looping = FALSE;
                        j -= k;
                }
                else
                {
                        relline--;
                        if (looping)
                                j = m;
                }
        }

        if (direction == BACKWARD)              /* Operation starts at      */
        {                                       /*   base[j] and stops at   */
                first = j;                      /*   base[front - 1]        */
                lastc = front - 1;
        }
        else                                    /* FORWARD: operation       */
        {                                       /*   at base[back  -1] and  */
                first = back + 1;               /*   staps at base[j + 1]   */
                lastc = j + 1;
        }
}

                /********************************/
                /*                              */
                /*       D E C F R O N T        */
                /*                              */
                /********************************/

                                                /* Decrement front pointer, */
                                                /*   and, if now at line    */
                                                /*   new line, baseline     */
                                                /* NOTE: this is a macro    */

#define decfront() if (base[--front] == LF) baseline--

                /********************************/
                /*                              */
                /*       M E M _ M O V E        */
                /*                              */
                /********************************/
VOID
mem_move(moveflag)                              /* If moveflag is TRUE, move*/
BOOLEAN moveflag;                               /*   character at base[back]*/
{                                               /*   to base[front]; adjust */
        register UWORD  tfront, tback;          /*   pointers and baseline  */
        register char   *tbase;                 /* Use registers for speed  */

        tfront = front;                         /* Get current values       */
        tback = back;
        tbase = base;

        if (direction == FORWARD)               /* If moveflag FALSE, just  */
        {                                       /*   adjust pointers (lose  */
            while (tback < lastc)               /*   the character)         */
            {
                tback++;
                if (moveflag)
                {                               /* Count lines if real move */
                    if ((tbase[tfront++] = base[tback]) == LF)
                        baseline++;
                }
            }
        }
        else                                    /* Backward move moves in   */
        {                                       /*   opposite direction;    */
            while (tfront > first)              /*   adjusts baseline even  */
            {                                   /*   if no move made        */
                if (tbase[--tfront] == LF)
                    baseline--;
                if (moveflag)
                    tbase[tback--] = tbase[tfront];
            }
        }

        front = tfront;                         /* Assign final values to   */
        back = tback;                           /*   globals                */
}

                /********************************/
                /*                              */
                /*         M O V E R            */
                /*                              */
                /********************************/

                                                /* Force a memory move      */
                                                /* NOTE: this is a macro    */

#define mover() mem_move(TRUE)

                /********************************/
                /*                              */
                /*         S E T P T R S        */
                /*                              */
                /********************************/

                                                /* Reset pointers, deleting */
                                                /*   characters (used by    */                                                  /*   delete command)        */
                                                /* NOTE: this is a macro    */

#define setptrs() mem_move(FALSE)

                /********************************/
                /*                              */
                /*      M O V E L I N E S       */
                /*                              */
                /********************************/

                                                /* Set memory limits and    */                                                  /*   force a move           */
                                                /* NOTE: this is a macro    */

#define movelines()     {setlimits(); mover();}

                /********************************/
                /*                              */
                /*       S E T F R O N T        */
                /*                              */
                /********************************/
VOID
setfront(newfront)                              /* Adjust front to lower    */
UWORD   newfront;                               /*   value, deleting chars  */
{                                               /*   and adjusting baseline */
        while (front != newfront)               /*   (used by S & J cmds)   */
                decfront();
}

                /********************************/
                /*                              */
                /*     S E T C L I M I T S      */
                /*                              */
                /********************************/
VOID
setclimits()                                    /* Set memory move limits   */
{
        if (direction == BACKWARD)
        {
                lastc = back;
                first = (distance > front) ? 1 : front - distance;
        }
        else
        {
                first = front;
                lastc = (distance >= max - back) ? maxm : back + distance;
        }
}

                /********************************/
                /*                              */
                /*        R E A D L I N E       */
                /*                              */
                /********************************/
VOID
readline()                                      /* Read line of user input  */
{                                               /*   into end of buffer     */
        register char   b;

        FOREVER
        {
                if (front >= back)              /* Buffer overflow: give up */
                        longjmp(main_env, OVERFLOW);
                if ((b = getsource()) == ENDFILE)
                {                               /* End of user input        */
                        distance = 0;
                        break;                  /* Translate to upper case  */
                }                               /*   if necessary before    */
                base[front++] = (upper) ? utran(b) : b; /* adding to buffer */
                if (b == LF)                    /* Bump current line ?      */
                {
                        baseline++;
                        break;
                }
        }
}

                /********************************/
                /*                              */
                /*      W R I T E L I N E       */
                /*                              */
                /********************************/
VOID
writeline()                                     /* Write a single line out */
{                                               /*   on the disk            */
        register char   b;

        FOREVER
        {
                if (back >= maxm)               /* Nothing to write?        */
                {
                        distance = 0;           /* Command is done          */
                        break;
                }
                putdest(b = base[++back]);      /* Write a character        */
                if (b == LF)                    /* Tally lines written      */
                {
                        baseline++;
                        break;
                }
        }
}

                /********************************/
                /*                              */
                /*         W R H A L F          */
                /*                              */
                /********************************/
VOID
wrhalf()                                        /* Write lines until the    */
{                                               /*   buffer is at least     */
        distance = 0xffff;                      /*   half empty (free up    */
        while (distnzero())                     /*   some workspace)        */
        {
                if (hmax >= (maxm - back))
                        distance = 0;
                else
                        writeline();
        }
}

                /********************************/
                /*                              */
                /*      W R I T E O U T         */
                /*                              */
                /********************************/
VOID
writeout()                                      /* Write number of lines    */
{                                               /*   given by distance      */
        direction = BACKWARD;                   /* Called by W and E cmds   */
        first = 1;
        lastc = back;
        mover();
        if (distance == 0)
                wrhalf();
        while (distnzero())
                writeline();
        if (back < lastc)
        {
                direction = FORWARD;
                mover();
        }
}

                /********************************/
                /*                              */
                /*        C L E A R M E M       */
                /*                              */
                /********************************/

                                                /* Write out the whole      */
                                                /*   memory buffer          */
                                                /* NOTE: this is a macro    */

#define clearmem()      {distance = 0xffff; writeout();}

                /********************************/
                /*                              */
                /*      T E R M I N A T E       */
                /*                              */
                /********************************/
VOID
terminate()                                     /* Clear buffers before     */
{                                               /*   terminating edit       */
        clearmem();
        if (! newfile)
                while ((chr = getsource()) != ENDFILE)
                        putdest(chr);
        finis();
}

/****************************************************************************/
/*                                                                          */
/*                C O M M A N D   P R I M I T I V E S                       */
/*                -----------------------------------                       */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*        I N S E R T           */
                /*                              */
                /********************************/
VOID
insert()                                        /* Insert a character into  */
{                                               /*   memory buffer          */
        if (front == back)
                longjmp(main_env, OVERFLOW);
        if ((base[front++] = chr) == LF)        /* Tally new lines          */
                baseline++;
}

                /********************************/
                /*                              */
                /*       S C A N N I N G        */
                /*                              */
                /********************************/
BOOLEAN
scanning()                                      /* Read a character.  Return*/
{                                               /*   TRUE if not ENDFILE and*/
        return (((chr = readc()) != ENDFILE)    /*   not CR.  When inserting*/
               && ((chr != CR) || inserting));  /*   CR also returns TRUE   */
}

                /********************************/
                /*                              */
                /*        C O L L E C T         */
                /*                              */
                /********************************/
VOID
collect()                                       /* Read characters from     */
{                                               /*   command buffer into    */
        while (scanning())                      /*   scratch buffer for Next*/
        {                                       /*   Juxt or Substitute cmds*/
                if (chr == CTL_L)               /* Translate CTL_L to CR LF */
                {
                        scratch[wbe] = chr = CR;/* Complain if scratch buff */
                        if (++wbe>= SCRSIZE)    /*   overflows              */
                                longjmp(main_env, OVERFLOW);
                        chr = LF;
                }
                if (chr == 0)
                        longjmp(main_env, BADCOM);
                scratch[wbe] = chr;
                if (++wbe >= SCRSIZE)
                        longjmp(main_env, OVERFLOW);
        }                                       /* wbe indexes next free    */
}                                               /*   byte on exit           */

                /********************************/
                /*                              */
                /*           F I N D            */
                /*                              */
                /********************************/
BOOLEAN
find(p1, p2)                                    /* On entry, p1 indexes the */
UWORD   p1, p2;                                 /*   start of a string in   */
{                                               /*   the scratch buffer, p2 */
        register UWORD  j, k;                   /*   indexes the end.       */
        BOOLEAN         match;                  /* Return TRUE if a matching*/
                                                /*   string is found between*/
        j = back ;                              /*   base[back + 1] and     */
        match = FALSE;                          /*   base[maxm - 1]         */
                                                /* If match found, move     */
                                                /*   memory so that matched */
                                                /*   string is at base[back]*/

        while (! match && (maxm > j))           /* Try  match at base[j + 1]*/
        {
                lastc = ++j;
                k = p1;                         /* Index 1st match char     */
                while (! (match = (k == p2))
                      && (scratch[k] == base[lastc]))
                {                               /* On exit, match is TRUE if*/
                        k++;                    /*   complete match found   */
                        lastc++;
                }
        }
        if (match)                              /* Success: do move         */
        {
                lastc--;
                mover();
        }
        return (match);
}

                /********************************/
                /*                              */
                /*         S E T F I N D        */
                /*                              */
                /********************************/
VOID
setfind()                                       /* Set up search string for */
{                                               /*   F, N & S commands      */
        wbe = 0;                                /* Index buffer start       */
        collect();
        wbp = wbe;                              /* Index buffer end         */
}

                /********************************/
                /*                              */
                /*       C H K F O U N D        */
                /*                              */
                /********************************/
VOID
chkfound()                                      /* Flag error if string not */
{                                               /*   found in F & S commands*/
        if (! find(0, wbp))
                longjmp(main_env, OVERCOUNT);
}

                /********************************/
                /*                              */
                /*      P A R S E _ L I B       */
                /*                              */
                /********************************/
BOOLEAN
parse_lib(fcbadr)
struct fcbtab   *fcbadr;                        /* Parse a filename string  */
{                                               /* (read, xfer or lib file) */
        BOOLEAN b;                              /* Fill in any blank fields */

        b = parse_fcb(fcbadr);                  /* Try the parse            */
        if (! flag)                             /* If it failed, give up    */
        {
                flag = 'O';
                longjmp(main_env, RESET);
        }
        if ((fcbadr->ftype[0] == ' ')           /* Parse worked.  If no     */
           && (! dot_found))                    /*   extension, use default */
                move(3, libfcb.ftype, fcbadr->ftype);
        if (fcbadr->fname[0] == ' ')            /* If no name, use default  */
                move(8, libfcb.fname, fcbadr->fname);
        return (b);
}

                /********************************/
                /*                              */
                /*      T Y P E L I N E S       */
                /*                              */
                /********************************/

typelines()                                     /* Type out lines (T and    */
{                                               /*   many other commands)   */
        register int    j;
        int     c;

        setlimits();                            /* Map count to buffer ptrs */
        inserting = TRUE;                       /* Disable the * prompt     */
        if (direction == FORWARD)               /* Do we need new line      */
        {                                       /*   before we start?       */
            relline = 0;
            j = front;
        }
        else
            j = first;

        if ((c = base[--j]) == LF)
        {
            if (column != 0)
                crlf();
        }
        else
            relline++;

        for (j = first; j <= lastc; j++)        /* Print loop               */
        {
            if (c == LF)                        /* New line?                */
            {
                printline(baseline + relline++);/* Print line number        */
                if (break_key())                /*   and bump count         */
                    longjmp(main_env, OVERCOUNT);       /* User wants out   */
            }
            printc(c = base[j]);                /* Print the character      */
        }
}

                /********************************/
                /*                              */
                /*            P A G E           */
                /*                              */
                /********************************/
VOID
page()                                          /* Page command: move N     */
{                                               /*   pages and print        */
        UWORD   tdist;                          /* place to save distance   */
        int     j;

        tdist = distance;
        distance = lpp;                         /* Move one page in current */
        movelines();                            /*   direction              */
        j = direction;                          /* Print page in forward    */
        direction = FORWARD;                    /*   direction              */
        distance = lpp;
        typelines();
        direction = j;                          /* Restore direction        */
        distance = ((lastc == maxm) || (first == 1)) ? 0 : tdist;
}

                /********************************/
                /*                              */
                /*           W A I T            */
                /*                              */
                /********************************/
VOID
wait()                                          /* Half-second wait (Z cmd) */
{
        register int    j;

        for (j=0; j++ <= 19; time())
                if (break_key())                /* Allow user to abort      */
                        longjmp(main_env, RESET);
}

                /********************************/
                /*                              */
                /*         A P P H A L F        */
                /*                              */
                /********************************/
VOID
apphalf()                                       /* Append until buffer is   */
{                                               /*   at least half full     */
        distance = 0xffff;                      /* Set maximum distance     */
        while (distnzero())                     /* Until buffer half full   */
        {
                if (front >= hmax) distance = 0;
                else  readline();               /*   read lines             */
        }
}

                /********************************/
                /*                              */
                /*       I N S C R L F          */
                /*                              */
                /********************************/
VOID
inscrlf()                                       /* Insert CR LF characters  */
{
        chr = CR;
        insert();
        chr = LF;
        insert();
}

                /********************************/
                /*                              */
                /*  I N S _ E R R O R _ C H K   */
                /*                              */
                /********************************/

                                                /* Test invalid delete or   */
                                                /*   backspace at beginning */
                                                /*   of insert              */
                                                /* NOTE: this is a macro    */

#define ins_error_chk() if ((tcolumn == 255) || (front == 1))\
                                longjmp(main_env, RESET);

                /********************************/
                /*                              */
                /*   I N S E R T _ C H A R S    */
                /*                              */
                /********************************/
VOID
insert_chars()                                  /* Insert characters into   */
{                                               /*   buffer (I command)     */
        int     tcolumn, qcolumn;               /* temps during backspace    */

        if ((inserting = ((combuf.cbp == combuf.comlen)
                          && (mp == 0))))
        {                                       /* I<CR>? Enter line insert */
            tcolumn = 255;                      /* Stop backspace working   */
            distance = 0;                       /*   temporarily            */
            direction = BACKWARD;
            if (base[front - 1] == LF)          /* Start of new line? If so */
                printbase();                    /*   print number           */
            else
                typelines();                    /* No.  Print end of current*/
        }

        while (scanning())                      /* Do until end of insert   */
        {                                       /*   reached                */
            switch (chr)                        /* Test all special cases   */
            {
              case CTL_H:                       /* Backspace, screen style  */
                ins_error_chk();                /* Abort if leftmost column */
                tcolumn = column;               /* What is the current posn */
                decfront();                     /* Delete char in buffer    */
                if (tcolumn > scolumn)          /* Have there been printable*/
                {                               /*   characters on this line*/
                    printsuppress = TRUE;       /* Yes: Make change on scren*/
                    column = scolumn;           /* Recompute current pos'n  */
                    typelines();                /*   (covers case where TAB */
                    printsuppress = FALSE;      /*    char is deleted)      */
                                                /* Move to greater of column*/
                                                /*   and start column       */
                    qcolumn = (scolumn > column) ? scolumn : column;
                    column = tcolumn;           /* Restore original value   */
                    while (column > qcolumn)
                        backspace();            /* Erase char(s) on screen  */
                }
                else                            /* Cover case where newline */
                {                               /*   sequence deleted       */
                    if (base[front - 1] == CR)
                        decfront();
                    crlf();
                    printnmbase();
                    typelines();                /* Print previous line      */
                }
                break;

              case LF:                          /* Map line feeds into CR LF*/
              case CR:                          /* Sme for carriage return  */
                prtnmac(CR);                    /* Send CR-LF sequence      */
                prtnmac(LF);                    /*   (zeros column)         */
                inscrlf();
                printnmbase();                  /* Print line # if required */
                break;

              case CTL_R:                       /* Redisplay line so user   */
                crlf();                         /*   can see how it looks   */
                printnmbase();                  /* Line number if needed    */
                typelines();
                break;

              case CTL_U:                       /* Delete line, teletype    */
                setlimits();                    /*   style.  Restore indices*/
                setptrs();                      /*   to line start, print   */
                crlf();                         /*   line number on new line*/
                printnmbase();
                break;

              case CTL_X:
                setlimits();                    /* Delete line, screen syle */
                setptrs();                      /* Restore indices to line  */
                while (column > scolumn)        /*   start, backspace to    */
                        backspace();            /*   start column           */
                break;

              case RUBOUT:                      /* Character delete, tty    */
                ins_error_chk();                /*   style.  Reasonable?    */
                decfront();                     /* Apparently.  Print out & */
                printc(chr = base[front]);      /*   forget most recently   */
                break;                          /*   entered character      */

              case CTL_L:                       /* Control-L in single-line */
                if (! inserting)                /*   command interpreted as */
                {                               /*   as new-line sequence   */
                    inscrlf();
                    break;                      /* NOTE: falls through to   */
                }                               /*   default if inserting!  */
                                                /* FALLTHROUGH              */

              default:                          /* End of special cases     */

                insert();                       /* Just insert character and*/
                prtnmac(chr);                   /*   echo for anything else  */
                                                /*   (accomodates tabs, ctl */
                                                /*   chars, adjusts column) */

                tcolumn = 0;                    /* Allow CTL_H, DEL         */
            }                                   /* End of switch            */
        }                                       /* End of while (scanning())*/

        if (chr != ENDFILE)                     /* Not ENDFILE: must be CR  */
        {                                       /*   at end of single line  */
            inscrlf();                          /*   command.  Put in buffer*/
            column = 0;
        }
        if (inserting && lineset)               /* Was unneeded # printed?  */
            crlf();                             /* Go to new line           */
}

                /********************************/
                /*                              */
                /*       R E A D _ L I B        */
                /*                              */
                /********************************/
VOID
read_lib()                                      /* Read library file: R cmd */
{
        static BOOLEAN  reading = FALSE;        /* TRUE if reading rfcb     */

        setrdma();                              /* Tell BDOS where data goes*/
        if ((flag = parse_lib(&rfcb)))          /* Is it R<filename> command*/
                reading = FALSE;                /* If so, must open file    */
        if (! reading)                          /* Need to open file?       */
        {
                if (! flag)                     /* Use transfer file name   */
                        move(12, (char *) &xfcb, (char *) &rfcb);
                rfcb.record = 0;                /* Read from beginning      */
                rfcb.extent = 0;
                rbp = SECTSIZE;                 /* 1st readfile forces read */
                open(&rfcb);
                reading = TRUE;
        }
        while ((chr = readfile()) != ENDFILE)   /* Read and insert data     */
                insert();
        reading = FALSE;
        _close(&rfcb);
}

                /********************************/
                /*                              */
                /*           J U X T            */
                /*                              */
                /********************************/

VOID                                            /* Juxtapose operation:     */
juxt()                                          /*   search for first string*/
{                                               /*   insert second, delete  */
        UWORD    wbj, t;                        /*   until third            */

        setfind();                              /* Get the search string, & */
        collect();                              /*   substitute string      */
        wbj = wbe;                              /* Note where it ends       */
        collect();                              /* Get terminating string   */
                                                /* Search for string at 0 to*/
                                                /*   (wbp - 1), insert      */
                                                /*   string at wpb to       */
                                                /*   (wbj - 1) and terminat-*/
                                                /*   ing string at wbj to   */
                                                /*   (wbe -1)  (Phew!)      */
        while (distnzero())                     /* While lines in buffer    */
        {
                chkfound();                     /* Find match, abort if none*/
                mi = wbp - 1;                   /* Got a match.  Insert     */
                while (++mi < wbj)              /*   string here            */
                {
                        chr = scratch[mi];
                        insert();
                }

                t = front;                      /* Save match start position*/
                if (! find(wbj, wbe))           /* Look for terminator      */
                                                /* Abort if not found       */
                        longjmp(main_env, OVERCOUNT);

                first = front - (wbe - wbj);    /* Found it.  Move it back  */
                direction = BACKWARD;           /*   so it follows last     */
                mover();                        /*   inserted character     */

                setfront(t);                    /* Position at start of     */
        }                                       /*   inserted string        */
}

                /********************************/
                /*                              */
                /*           N E X T            */
                /*                              */
                /********************************/

VOID                                            /* Next command: search     */
next()                                          /*   whole file for a match,*/
{                                               /*   not just edit buffer   */
        UWORD   tdist;                          /* place to save distance   */

        setfind();                              /* Collect the search string*/
        while (distnzero())                     /* While data in buffer     */
        {
                while (! find(0, wbp))          /* While string not found   */
                {
                        if (break_key())        /* Allow user to abort      */
                                longjmp(main_env, RESET);

                        tdist = distance;       /* Save distance, then write*/
                        clearmem();             /*   buffer to disk         */
                        apphalf();              /* Get half a bufferfull    */
                        direction = BACKWARD;   /* Move to base of buffer   */
                        first = 1;
                        mover();
                        distance = tdist;       /* Restore distance and     */
                        direction = FORWARD;    /*   prepare to search      */
                        if (back >= maxm)       /* If end of file, abort    */
                                longjmp(main_env, OVERCOUNT);
                }
        }
}

/****************************************************************************/
/*                                                                          */
/*                     C O M M A N D   P A R S I N G                        */
/*                     -----------------------------                        */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*       R E A D C T R A N      */
                /*                              */
                /********************************/
VOID
readctran()                                     /* Get a char, translate to */
{                                               /*   uppercase.  If char was*/
        BOOLEAN t;                              /*   uppercase anyway, or if*/
                                                /*   upper flag set, set    */
                                                /*   flag so whole line will*/
                                                /*   be translated          */

        translate = FALSE;                      /* Do not translate to u/c  */
        chr = readc();
        t = lowercase(chr);                     /* Is character lowercase?  */
        translate = TRUE;
        chr = utran(chr);                       /* Make sure it's uppercase */
        translate = (upper || (!t));            /* Translate following      */
}                                               /*   characters to u/c?     */

                /********************************/
                /*                              */
                /*        S N G L C O M         */
                /*                              */
                /********************************/

                                                /* TRUE if command is only  */
                                                /*   char, not in macro or  */
                                                /*   combination on a line  */
                                                /* NOTE: this is a macro    */

#define snglcom(c)      ((chr == (c)) && (combuf.comlen == 1) && (mp == 0))

                /********************************/
                /*                              */
                /*        S N G L R C O M       */
                /*                              */
                /********************************/
BOOLEAN
snglrcom()                                      /* Return TRUE if user      */
{                                               /*   responds "Y" to a Y/N  */
        int     j;                              /*   request.  Aborts if    */
                                                /*   answer is "N"          */
        if (! snglcom(chr))
                return (FALSE);
        FOREVER                                 /* Until user enters "Y"    */
        {                                       /*   or "N"                 */
                crlf();
                printch(chr);
                _print("-(Y/N)? $");
                printc(j = ucase((int) _conio(0xff)));
                crlf();
                switch (j)
                {
                  case 'N':
                        longjmp(main_env, GET_LINE);
                  case 'Y':
                        return(TRUE);
                }
        }
}

                /********************************/
                /*                              */
                /*         N U M B E R          */
                /*                              */
                /********************************/
VOID
number()                                        /* Set distance to the      */
{                                               /*   number (if any) on cmd */
        int     i;                              /*   line, else 0.  On exit */
                                                /*   chr holds cmd char     */
        distance = 0;
        while (((i = ((int) chr) - '0') <= 9)   /* Uses signed arithmetic!  */
              && (i >= 0))
        {                                       /* While digits read...     */
                distance = 10 * distance + i;
                readctran();
        }
}

                /********************************/
                /*                              */
                /*     R E L D I S T A N C E    */
                /*                              */
                /********************************/
VOID
reldistance()                                   /* Change to distance from  */
{                                               /*   absolute value to value*/
        if (distance > baseline)                /*   relative to current    */
        {                                       /*   line                   */
                direction = FORWARD;
                distance -= baseline;
        }
        else
        {
                direction = BACKWARD;
                distance = baseline - distance;
        }
}

/****************************************************************************/
/*                                                                          */
/*                     C O M M A N D   A C T I O N S                        */
/*                     -----------------------------                        */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*         S I M P L E          */
                /*                              */
                /********************************/
VOID
simple()                                        /* Actions the "simple"     */
{                                               /*   commands: those which  */
                                                /*   may not be preceded by */
                                                /*   a number.  These are:  */
                                                /*   E, H, I, O, Q, R       */

        switch(chr)                             /* Some commands MUST be    */
        {                                       /*   alone on the line for  */
          case 'E': case 'H':                   /*   safety.  If they are   */
                if(!snglcom(chr))               /*   not, flag error.  If   */
                        longjmp(main_env,BADCOM);/*  'E' or 'H', must be    */
                break;                          /*   alone on line but need */
                                                /*   not be confirmed.  If  */
          case 'O': case 'Q':                   /*   'O' or 'Q', then user  */
                if (! snglrcom())               /*   is asked to confirm    */
                        longjmp(main_env, BADCOM);/*   them.                */
        }                                       /*   (get new line if not   */
                                                /*   confirmed in snglrcom) */

        switch(chr)                             /* Now take appropriate     */
        {                                       /*   action for command     */
          case 'E':                             /* End edit normally        */
                terminate();                    /* Write & rename O/P file  */
                _exit(0);                       /* Reboot to CP/M           */
                                                /* _exit() does not return  */
                                                /* FALLTHROUGH              */

          case 'H':                             /* Go to top (head).  This  */
                terminate();                    /*   is like ending edit    */
                newfile = FALSE;                /*   then starting new edit */
                if (onefile)                    /*   on file just written   */
                {                               /* If source & dest names   */
                        chr = SFCB->drive;      /*   the same, they may be  */
                        SFCB->drive = dfcb.drive;/*  on different disks:    */
                        dfcb.drive = chr;       /*   swap drive names       */
                }
                else                            /* Src & dest names differ: */
                {                               /*   new src name is old    */
                        *SFCB = dfcb;           /*   dest file name         */
                        onefile = TRUE;         /* Src & dest now the same  */
                }
                longjmp(main_env, RESTART);     /* Start new edit           */

          case 'I':                             /* Insert characters        */
                insert_chars();
                break;

          case 'O':                             /* Re-edit old file: forget */
                _close(SFCB);                   /*   any changes made       */
                longjmp(main_env, RESTART);

          case 'R':                             /* Read from .LIB file      */
                read_lib();
                break;

          case 'Q':                             /* Quit: abort edit session */
                delete_file(&dfcb);             /* Delete .$$$ dest file    */
                if (newfile || (! onefile))     /* Is there correctly named */
                {                               /*   dest file?  If so,     */
                        settype(&dfcb, dtype);  /*   delete that too        */
                        _delete(&dfcb);
                }
                _exit(0);                       /* Reboot CP/M              */
        }
}

                /********************************/
                /*                              */
                /*     C O N T R O L L E D      */
                /*                              */
                /********************************/
VOID
controlled()                                    /* Actions the "controlled" */
{                                               /*   commands: those which  */
                                                /*   may be preceded by a   */
                                                /*   direction and/or line  */
                                                /*   number.  These are:    */
                                                /*   B, C, D, K, L, P, T,   */
                                                /*   U, V, <CR>             */

        switch(chr)                             /* Take appropriate action  */
        {
          case 'B':                             /* Beginning/bottom of buff */
                                                /*   according to dir'n     */
                                                /* Reverse dir'n for move   */
                direction = (direction == FORWARD) ?
                              BACKWARD : FORWARD;
                first = 1;                      /* Move whole buffer        */
                lastc = maxm;
                mover();
                break;

          case 'C':                             /* Move character positions */
                setclimits();                   /* Map distance to buffer   */
                mover();                        /*   positions, do move     */
                break;

          case 'D':                             /* Delete characters        */
                setclimits();                   /* Map distance to buffer   */
                setptrs();                      /*   position, adjust ptrs  */
                break;

          case 'K':                             /* Kill lines               */
                setlimits();                    /* Map distance to buffer   */
                setptrs();                      /*   position, adjust ptrs  */
                break;

          case 'L':                             /* Move line position       */
                movelines();
                break;

          case 'P':                             /* Print page/pages         */
                if (distance == 0)              /* Only one?                */
                {                               /* Just print it            */
                        direction = FORWARD;
                        distance = lpp;         /* Get screen height        */
                        typelines();
                }
                else                            /* More than one page       */
                {                               /* Pause after each: give   */
                        while (distnzero())     /*   user chance to break in*/
                        {
                                page();
                                wait();
                        }
                }
                break;

          case 'T':                             /* Type lines               */
                typelines();
                break;

          case 'U':                             /* Map lower to upper case  */
                upper = (direction == FORWARD); /*   (turn off mapping if   */
                break;                          /*    -U entered)           */

          case 'V':                             /* Verify (print) line #'s  */
                if (distance == 0)              /* 0V is special case:      */
                {
                        printvalue(back - front);/*Print space available    */
                        printc('/');            /*   in and size of memory  */
                        printvalue(maxm);       /*   buffer                 */
                        crlf();
                }
                else                            /* Normal case: set lineset */
                {                               /*   according to direction;*/
                                                /*   set correct startcolumn*/
                        scolumn = (lineset = (direction == FORWARD)) ?
                                    8 : 0;
                }
                break;

          case CR:                              /* CR character: Interpret  */
                if ((mi == 1) && (mp == 0))     /*   as nLT if no command   */
                {                               /*   follows                */
                        movelines();
                        direction = FORWARD;
                        distance = 1;
                        typelines();
                }
                break;
        }
}

                /********************************/
                /*                              */
                /*       R E P E A T E D        */
                /*                              */
                /********************************/
VOID
repeated()                                      /* Actions the "repeated"   */
{                                               /*   commands.  These may be*/
                                                /*   preceded by a count,   */
                                                /*   but not a direction.   */
                                                /*   They are:              */
                                                /*   A, F, M, N, S, W, X, Z */

        switch (chr)                            /* Take appropriate action  */
        {
          case 'A':                             /* Append lines from source */
                direction = FORWARD;            /* Move all text to base of */
                first = front;                  /*   buffer                 */
                lastc = maxm;
                mover();
                if (distance == 0)              /* User wants to append as  */
                {                               /*   much as management will*/
                        apphalf();              /*   allow (half of buffer) */
                }
                else                            /* User gave a specific     */
                {                               /*   line count             */
                        while (distnzero())
                                readline();
                }
                direction = BACKWARD;           /* Split buffer as before   */
                mover();
                break;

          case 'F':                             /* Find a string            */
                setfind();                      /* Set up string in temp buf*/
                while (distnzero())             /* Look for it              */
                        chkfound();
                break;

          case 'J':                             /* Juxtapose strings: see   */
                juxt();                         /*   juxt() function for    */
                break;                          /*   explanation!           */

          case 'M':                             /* Macro: repeat actions    */
                if (mp != 0)                    /*   following "distance"   */
                        longjmp(main_env, BADCOM);/* times.  Macros cannot  */
                                                /*   be nested              */
                                                /* Copy macro to buffer     */
                for (xp = 0; (macro[xp++] = readc()) != CR;)
                {
                        if (xp == sizeof macro) /* Beware overflow!         */
                                longjmp(main_env, OVERFLOW);
                }
                mp = xp - 1;                    /* Point to end of macro    */
                xp = 0;                         /*   and beginning          */
                                                /* Set repeat count: 0M, 1M,*/
                                                /*   M all mean 1M          */
                mt = (distance > 1) ? distance : 1;
                break;

          case 'N':                             /* Search whole file for    */
                next();                         /*   next match of string   */
                break;

          case 'S':                             /* Substitute string        */
                setfind();                      /* Set up the string to find*/
                collect();                      /*   and to substitute      */
                while(distnzero())              /* Repeat the following:    */
                {
                        chkfound();             /* Find a match             */
                                                /* Delete matched string    */
                        setfront(front - (mi = wbp));
                        while (mi < wbe)        /* Insert replacement string*/
                        {
                                chr = scratch[mi++];
                                insert();
                        }
                }
                break;

          case 'W':                             /* Write out buffer         */
                writeout();
                break;

          case 'X':                             /* Copy lines into transfer */
                flag = parse_lib(&rfcb);        /*   file (for block move)  */
                if (distance == 0)              /* 0X means delete the file */
                {
                        xferon = FALSE;         /* Transfer file inactive   */
                        _delete(&rfcb);         /* Delete it                */
                        if (dcnt == 0xff)       /* Grouse if it's not there */
                                perror(not_found);
                }
                else                            /* Not 0X: write the file   */
                {
                        wrt_xfer();
                }
                close_xfer();
                break;

          case 'Z':                             /* Sleep.  If 0Z, wait until*/
                if (distance == 0)              /*   any key entered.  CTL_Y*/
                {                               /*   junks current cmd line */
                        if (break_key())
                                longjmp(main_env, RESET);
                }
                else                            /* Sleep "distance" quarter */
                {                               /*   seconds, allowing user */
                        while (distnzero())     /*   to abort               */
                                wait();
                }
        }
}

/****************************************************************************/
/*                                                                          */
/*                      I N I T I A L I Z A T I O N                         */
/*                                                                          */
/*      How does ED allocate memory?  Like this (not to scale):             */
/*                                                                          */
/*      +-------------------------------+  <--- Address zero                */
/*      |                               |                                   */
/*      |              E D              |       Only this area is allocated */
/*      |                               |       when ED is initially loaded */
/*      |  C O D E ,  D A T A ,  B S S  |       by CP/M (ED is very approx  */
/*      |                               |       12k in length)              */
/*      |                               |                                   */
/*      +-------------------------------+  <--- base points here            */
/*      |                               |                                   */
/*      |     E D I T   B U F F E R     |       Allocated by allocte_memory.*/
/*      |                               |                                   */
/*      |                               |  <--- base[hmax] halfway up buffer*/
/*      |    3/4 of space between base  |                                   */
/*      |        and top of stack       |  <--- base[maxm] penultimate byte */
/*      |                               |  <--- base[max] is last byte      */
/*      +-------------------------------+  <--- dbuffadr points here        */
/*      |                               |                                   */
/*      |    Destination File Buffer    |       Allocated by allocate_memory*/
/*      | 1/8 space between base & tos  |                                   */
/*      |                               |  <--- dbuffaddr[bufflength - 1]   */
/*      +-------------------------------+  <--- sbuffadr points here        */
/*      |                               |                                   */
/*      |       Source File Buffer      |       Allocated by allocate_memory*/
/*      | 1/8 space between base & tos  |                                   */
/*      |                               |  <--- sbuffaddr[bufflength - 1]   */
/*      +-------------------------------+                                   */
/*      |XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|       Stack growth allowance      */
/*      +-------------------------------+                                   */
/*      |                               |                                   */
/*      |          User stack           |                                   */
/*      |                               |                                   */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /* A L L O C A T E _ M E M O R Y*/
                /*                              */
                /********************************/
VOID
allocate_memory()                               /* Carve up free memory     */
{
                                                /* CP/M-386: the base page has*/
                                                /*   no segment length; ask   */
                                                /*   the shim instead       */
        max = (UWORD) _membytes() - MARGIN;     /* How much space can we get*/
        if ((int) (base = sbrk(max)) == -1)     /* Grab the lot!            */
        {
            perror("Can't allocate memory$");   /* "This should never       */
            _exit(1);                           /*   happen" (famous last   */
        }                                       /*   words!)                */

#ifdef DEBUG
        fill(base, 0xa5, max);                  /* Make buff changes visible*/
#endif
        nbuf = ((max / SECTSIZE) / 8) - 1;      /* Allocate 2 I/O buffers,  */
                                                /*  each of (nbuff + 1)     */
        bufflength = (nbuf + 1) * SECTSIZE * 2; /*  sectors, and each taking*/
        if ((bufflength + 1024) > max)          /*  1/8 of free memory      */
        {
            perror("Insufficient memory$");     /* No room left for edit    */
            _exit(1);                           /*   buffer                 */
        }
        max -= (bufflength + 1);                /* Adjust max to index byte */
                                                /*   below I/O buffer       */
        base[max] = 0;                          /* Stop match at buffer end */
        maxm = max - 1;                         /* maxm is max valid index  */
        hmax = maxm / 2;                        /* hmax is halfway up buffer*/

        bufflength /= 2;                        /* Divide disk buff between */
        sbuffadr = &base[max + 1];              /*   source buff at sbuffadr*/
        dbuffadr = &sbuffadr[bufflength];       /*   & dest buff at dbuffadr*/
}

                /********************************/
                /*                              */
                /*   S E T _ U P _ F I L E S    */
                /*                              */
                /********************************/
VOID
set_up_files()                                  /* Determine source and     */
{                                               /*   destination filenames  */
        if (fcb1->fname[0] == ' ')              /* Was file named on command*/
        {                                       /*   line?  If not, request */
           print("Input file? $");              /*   one                    */
           readcom();
           printc(LF);
           tail = FALSE;                        /* Flag command line empty  */
        }
        if (! parse_fcb(SFCB))                  /* Reboot if not valid name */
            reboot();

        if (has_bdos3)                          /* For version 3 BDOS,      */
        {                                       /*   provisionally give dest*/
            copydest();                         /*   same name and password */
            _get_xfcb(&dfcb);                   /*   protection mode as src */
            protection = dfcb.extent;
            if ((ver & 0xff00) == 0)            /* For CP/M 80, try to read */
                                                /*   lines per page         */
                if ((lpp = _gset_scb(pb)) == 0)
                    lpp = 23;                   /* Failure: use default     */
        }
        setdest();                              /* Parse destination file   */
        tail = FALSE;                           /* Finished with cmd line   */

        /* If source and destination disks differ, check for an existing    */
        /*   source file on the destination disk - there could be a fatal   */
        /*   error condition which could destroy a file if the user happened*/
        /*   to be addressing the wrong disk                                */

        if (((! onefile)                        /* Input and output not same*/
           || (SFCB->drive != dfcb.drive))      /* On different disks?      */
           && (_open(&dfcb) != 255))            /* Try to open output file  */
                abort("output file exists, erase it$");
}

/****************************************************************************/
/*                                                                          */
/*                      M A I N   P R O G R A M                             */
/*                      -----------------------                             */
/*                                                                          */
/* NOTE: this function is named _main, not main, so as to avoid the         */
/*       standard C prolog function (also called _main) being pulled in     */
/*       from the library at link-edit time.  The library function would    */
/*       add a considerable amount of dead wood - notably several parts     */
/*       of the standard I/O package - which are not required by ED         */
/*                                                                          */
/****************************************************************************/

VOID
_main()
{
        int reason;                             /* Internal error code      */

        ver = _version();                       /* Where are we?  What      */
        has_bdos3 = ((ver & 0x00ff) >= CPM3);   /*   facilites are there?   */
                                                /* BDOS 3 has passwds, xfcbs*/

        allocate_memory();
        fcb1 = &_base->fcb1;                    /* Initialize pointers to   */
        buff = _base->buff;                     /*   basepage structures    */
        set_up_files();

                /********************************/
                /*                              */
                /*         Error recovery       */
                /*                              */
                /********************************/

        /* The following control structure is used in error recovery:       */
        /*   serious errors pass control to the top of the switch by means  */
        /*   of longjmp(main_env, ERROR_CODE).  Control then passes to the  */
        /*   switch label corresponding to ERROR_CODE.                      */

        switch (reason = setjmp(main_env))
        {
        case RESTART:                           /* Junk buffer contents (if */
            setup();                            /*   any) and start edit    */
            base[0] = LF;                       /*   from the top           */
            front = 1;
            back = maxm;
            column = 0;
            break;

        case GET_LINE:                          /* Just get new command     */
            break;

        case OVERCOUNT:                         /* Command fell out of buff */
            flag = POUND;
            break;

        case BADCOM:                            /* Invalid command          */
            flag = WHAT;
            break;

        case OVERFLOW:                          /* I, F, or S command hit   */
            flag = '>';                         /*   end of memory          */
            break;

        case DISK_ERR:                          /* Disk is irrevocably full */
            flag = 'F';
            err_msg = diskfull;
            break;

        case DIR_ERR:                           /* Can't create file:       */
            flag = 'F';                         /*   directory full         */
            err_msg = dirfull;

        case RESET:                             /* Clean up after error     */
            break;
        }                                       /*   (see next comment)     */

        if (reason > GET_LINE)                  /* Did something go wrong?  */
        {
            printsuppress = FALSE;
            print("\tBreak \"$");               /* Say what went wrong      */
            printc(flag);
            _print("\" at $");
            if ((chr == CR) || (chr == LF))
            {
                _print("end of line$");
            }
            else
            {
                printc(chr);                    /* On which command         */
            }
            if (err_msg != NULL)                /*   and why                */
            {
                perror(err_msg);
                err_msg = NULL;
            }
            crlf();
        }

        readbuff = TRUE;                        /* Need to read new command */
        mp = 0;                                 /* Forget about any macro   */

                /********************************/
                /*                              */
                /*      Main Processing Loop    */
                /*                              */
                /********************************/

        FOREVER                                 /* Until unrecoverable error*/
        {                                       /*   or user wants out      */
            inserting = FALSE;                  /* Clear text insert mode   */
            readctran();                        /*   Get character          */
            flag = 'E';
            mi = combuf.cbp;                    /* Save command start addr  */

            /****************************************************************/
            /*                                                              */
            /* SIMPLE COMMANDS (cannot be preceded by direction/distance)   */
            /*                                                              */
            /*  E       End the edit normally                               */
            /*  H       Move to head of edited file                         */
            /*  I       Insert characters                                   */
            /*  O       Return to the original file (junk any changes)      */
            /*  Q       Quit edit without change to original file           */
            /*  R       Read from library file                              */
            /*                                                              */
            /****************************************************************/

            switch(chr)
            {
              case 'E': case 'H': case 'I': case 'O': case 'Q': case 'R':

                simple();                       /* Go honor command         */
                continue;                       /* Return to FOREVER        */
            }

                /********************************/
                /*                              */
                /*     Not a simple command     */
                /*                              */
                /*  Could be a count or number  */
                /*                              */
                /********************************/

            direction = FORWARD;                /* Direction provisionally  */
            distance = 1;                       /*   forward, over 1 item   */

            if (chr == '-')                     /* Direction is backward    */
            {
                readctran();                    /* Read another character   */
                direction = BACKWARD;
            }

            switch(chr)                         /* Direction set: how far?  */
            {
              case POUND:                       /* "As far as you can go"   */
                distance = 0xffff;
                readctran();                    /* Read another character   */
                break;

              case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':

                number();                       /* Get the count.  If colon */
                if (chr == ':')                 /*   follows, it's absolute */
                {                               /* Treat as "Move to Line"  */
                    chr = 'L';
                    reldistance();
                }
                break;

              case ':':                         /* Absolute (rather than    */
                readctran();                    /*   relative) line number  */
                number();
                reldistance();                  /* Convert to relative      */
                if (direction == FORWARD)
                    distance++;
            }

            if (distance == 0)
                direction = BACKWARD;

            switch(chr)                         /* Should be at command now */
            {
              case 0:                           /* Could be null left by    */
                break;                          /*   last stop: discard it  */

            /****************************************************************/
            /*                                                              */
            /* CONTROLLED EFFECT COMMANDS (may have direction and distance) */
            /*                                                              */
            /*  B       Move to Beginning/Bottom of buffer                  */
            /*  C       Move Character positions                            */
            /*  D       Delete characters                                   */
            /*  K       Kill lines                                          */
            /*  L       Move Line position                                  */
            /*  P       Page up or down lpp lines and print                 */
            /*  T       Type lines                                          */
            /*  U       Upper case translate                                */
            /*  V       Verify line numbers                                 */
            /*  <CR>    Move up or down lines and print                     */
            /*                                                              */
            /****************************************************************/

              case 'B': case 'C': case 'D': case 'K': case 'L':
              case 'P': case 'T': case 'U': case 'V': case CR:

                controlled();                   /* Execute the command      */
                break;                          /* Go back to FOREVER       */

            /****************************************************************/
            /*                                                              */
            /* REPEATED COMMANDS (allow only a preceding number)            */
            /*                                                              */
            /*  A       Append lines from source file to buffer             */
            /*  F       Find n'th occurrence of match string                */
            /*  J       Juxtapose (combined replace & delete)               */
            /*  M       Apply Macro                                         */
            /*  N       Same as F, but scan whole file, not just buffer     */
            /*  S       Perform n Substitutions                             */
            /*  W       Write lines to the output file                      */
            /*  X       Transfer (Xfer) lines to temp file                  */
            /*  Z       Sleep (ZZZZ)                                        */
            /*                                                              */
            /****************************************************************/

              case 'A': case 'F': case 'J': case 'M': case 'N':
              case 'S': case 'W': case 'X': case 'Z':

                if ((direction == FORWARD)      /* Not valid commands if    */
                   || (distance == 0))          /*  not just number precedes*/
                {
                    repeated();                 /* OK to do command         */
                    break;                      /* Go back to FOREVER       */
                }                               /* NOTE: else case falls    */
                                                /*   thru to default below  */
                                                /* FALLTHROUGH              */

            /****************************************************************/
            /*                                                              */
            /* NONE OF THE ABOVE                                            */
            /*                                                              */
            /*  Not a valid command letter: error                           */
            /*                                                              */
            /****************************************************************/

              default:
                longjmp(main_env, BADCOM);      /* Jump out to error handler*/
            }                                   /* End of switch (chr)      */

        }                                       /* End of FOREVER           */
}

/*****************************************************************************/

/*
 * Local Variables:
 * mode: c
 * indent-tabs-mode: nil
 * tab-width: 8
 * c-basic-offset: 8
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/*****************************************************************************/
/* vim: set ft=c ts=8 sw=8 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
