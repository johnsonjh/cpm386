/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 90ef8eaa-9103-11f1-a89e-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * pip.c - the CP/M peripheral interchange program
 *
 * Ported from the Digital Research / Zilog portable C sources.
 *
 * Usage:
 *   PIP                      - Interactive mode, prompts with *
 *   PIP dest=source[,source] - Copy, concatenating multiple sources
 *   PIP d:=filespec          - Copy to drive d: under the same name
 *
 * Devices: CON:, AXI:, AXO:, LST:, PRN:, NUL:, EOF:, INP:, OUT:.
 * Options in [ ]: A B Dn E F Gn H I L N O Pn Qs Ss Tn U V W Z.
 */

/*****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*       P e r i p h e r a l  I n t e r c h a n g e  P r o g r a m          */
/*       ---------------------------------------------------------          */
/*                                                                          */
/*         Copyright (c) 1976, 1977, 1978, 1979, 1980, 1981                 */
/*         Digital Research                                                 */
/*         Box 579                                                          */
/*         Pacific Grove, CA                                                */
/*         93950                                                            */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*         Revised:                                                         */
/*            17 Jan 80  by  Thomas Rolander (MP/M 1.1)                     */
/*            05 Oct 81  by  Ray Pedrizetti  (MP/M-86 2.0)                  */
/*            18 Dec 81  by  Ray Pedrizetti  (CP/M-86 1.1)                  */
/*            29 Jun 82  by  Ray Pedrizetti  (CCP/M-86 3.0)                 */
/*            24 Aug 82  by  Dave Sallume    (Translate to C)               */
/*            20 Sep 82  by  Dominic Dunlop  (PCP/M 2.2)                    */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*      Signon message - Please modify version and date with each change    */
/*                                                                          */
/****************************************************************************/

char    signon[] = "Zilog CP/M-Z8000 PIP v1.0A 01/03/84";

/****************************************************************************/
/*                                                                          */
/* Command lines used for command file generation                           */
/*                                                                          */
/*      === To be supplied later for Z8000-based systems ===                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*         W H A T   I T   D O E S   A N D   H O W   I T   W O R K S        */
/*                                                                          */
/* 1  PIP'S FUNCTION                                                        */
/*                                                                          */
/*      PIP, the Peripheral Interchange Program, copies data from one       */
/*      or more peripheral or disk file to another peripheral or disk file. */
/*      It is also able to copy all files matching an ambiguous file name   */
/*      (one containing a wild card such as * or ?) from one disk to        */
/*      another or from one user number to another.                         */
/*                                                                          */
/*      Finally, PIP can concatenate data from more than one source file    */
/*      and/or device, sending the combined data to a single destination    */
/*      file or device.                                                     */
/*                                                                          */
/*      While the copy is taking place, PIP can optionally perform checks   */
/*      - such as checking that an Intel Hex format file's checksums are    */
/*      correct, and translations - for example from upper to lower case.   */
/*      There are also facilites for adding line numbers to output and      */
/*      formatting it into pages for printing.  It is also possible to      */
/*      specify that only part of a file, delimited by start and end        */
/*      strings, is copied.                                                 */
/*                                                                          */
/*      When the destination is a disk file, PIP first copies all the       */
/*      source data into a temporary file.  Only when the transfer has      */
/*      completed without errors, and optional verification (if selected)   */
/*      has checked the data, does PIP delete any existing file with the    */
/*      destination file name and rename the temporary file to the          */
/*      destination name.  This minimizes the possibility that a failing    */
/*      transfer will result in the loss of a backup or older version of    */
/*      the detination file.  In the event that a transfer does fail, PIP's */
/*      temporary file is not deleted in case it contains usable data.      */
/*                                                                          */
/* 2   PIP'S COMMAND MODES                                                  */
/*                                                                          */
/*      PIP has two command modes: single and multiple.  In single command  */
/*      mode the user enters details of a single transfer on the command    */
/*      line following the name of the program, PIP.  For example,          */
/*                                                                          */
/*              A>PIP LST:=MYFILE.LST   (where A> is the CCP prompt)        */
/*              A>                      (prompt from CCP on completion)     */
/*                                                                          */
/*      copies A:MYFILE.LST to the LST: logical device (normally a printer) */
/*      and returns control to the CCP (Console Command Processor, CP/M's   */
/*      command interpreter) when the operation is complete.  See the next  */
/*      section for details of valid commands.                              */
/*                                                                          */
/*      To enter multiple command mode, the user gives PIP no command line  */
/*      arguments.  In response, PIP prompts (with an asterisk) for         */
/*      commands, and continues to do this until a blank line is entered,   */
/*      showing that the user has finished with PIP and wishes to return to */
/*      the CCP.  Suppose the user wants to make a backup copy of a file,   */
/*      then print it:                                                      */
/*                                                                          */
/*              A>PIP                   (Run PIP In multiple command mode)  */
/*              *MYFILE.BAK=MYFILE.C    (Make backup copy of a file)        */
/*              *LST:=MYFILE.C          (Copy file to printer)              */
/*              *                       (User types return only)            */
/*              A>                      (Prompt from CCP)                   */
/*                                                                          */
/*      This sequence of commands is a quicker alternative to running PIP   */
/*      twice in single command mode to achieve the same effect because PIP */
/*      is only loaded from disk once instead of twice. It also reduces the */
/*      amount of typing required.                                          */
/*                                                                          */
/* 3   SOURCES AND DESTINATIONS                                             */
/*                                                                          */
/*      PIP knows about three classes of data source or destination:        */
/*      logical devices, files and disks.                                   */
/*                                                                          */
/* 3.1 Logical Devices                                                      */
/*                                                                          */
/*      The following table lists the logical devices known to the system.  */
/*      These devices are mapped to actual physical devices in CP/M's BIOS  */
/*      (Basic Input/Output System) software.  Many CP/M implementations    */
/*      allow the operator to change the mapping by manipulating a system   */
/*      variable, IOBYTE either with direct operating system calls or with  */
/*      the STAT utility.  MP/M does not have this facility.                */
/*                                                                          */
/*      Name    | Function              | Src?  | Dst?  | Comments          */
/*      --------+-----------------------+-------+-------+---------------    */
/*      AXI:    | Auxilliary input      | Yes   | No    | Serial input,     */
/*              |                       |       |       |  tape reader etc. */
/*      AXO:    | Auxilliary output     | No    | Yes   | Serial ouput,     */
/*              |                       |       |       |  tape punch etc.  */
/*      CON:    | Console               | Yes   | Yes   | Input is keyboard,*/
/*              |                       |       |       |  output is screen */
/*      EOF:    | End of file character | Yes   | No    | Dummy device      */
/*      INP:    | (User defined input)  | No    | No    | Obsolete: not     */
/*              |                       |       |       |  supported        */
/*      LST:    | List device           | No    | Yes   | System printer    */
/*      NUL:    | Null trailer for paper| Yes   | No    | Dummy device      */
/*              |  tape runout          |       |       |                   */
/*      PRN:    | Print on 66 line pages| No    | Yes   | Actually same     */
/*              |  with line numbers    |       |       |  device as LST:   */
/*      OUT:    | (User defined output) | No    | No    | Obsolete: not     */
/*              |                       |       |       |  supported        */
/*                                                                          */
/* 3.2 Files                                                                */
/*                                                                          */
/*      File names come in two forms: unambiguous and ambiguous.  Both have */
/*      the form                                                            */
/*                                                                          */
/*              d:filename.typ;password                                     */
/*                                                                          */
/*              where d:        is an optional disk name.  If missing, the  */
/*                              current disk is assumed                     */
/*                    filename  is a 1 to 8 character name consisting of    */
/*                              alphabetic, numeric underscore or wildcard  */
/*                              characters (* or ?)                         */
/*                    .ext      is an optional file type (1 to 3 of the     */
/*                              characters which may appear in filename.)   */
/*                              If missing, presumed blank                  */
/*                    password  is a 1 to 8 character password.  If not     */
/*                              given for a file which requires a password, */
/*                              PIP prompts the user for one.  Passwords    */
/*                              are implemented only in MP/M and CP/M       */
/*                              version 3.0 or later                        */
/*                                                                          */
/*      The difference between unambiguous and ambiguous filenames is that  */
/*      the latter contain one or more wildcard while the former do not.    */
/*      An asterisk anywhere in a filename makes that name match all names. */
/*      A question mark specifies a single "don't care" character.  Similar */
/*      rules apply to file types.                                          */
/*                                                                          */
/*      Ambiguous filenames are not allowed as destinations, nor may they   */
/*      appear in or in place of lists of source files to be concatenated.  */
/*      They are used in copying groups of files from one disk or user to   */
/*      another.                                                            */
/*                                                                          */
/*      Files are held on disk and come in two flavors: ASCII and binary.   */
/*      A control-Z in an ASCII file marks the logical end of the file.     */
/*      Control-Z has no special significance in a binary file.  In most    */
/*      cases this distinction is unimportant to PIP.  The only time when   */
/*      it needs to know that it is not dealing with an ASCII file is when  */
/*      it is concatenating non-ASCII files (see 4.x below).  The O option  */
/*      (section 5) should be used in this case.                            */
/*                                                                          */
/*      Files may be dense or sparse.  Suppose the last record in a file    */
/*      has logical record number 7.  If 8 physical records (sectors)       */
/*      (zero through seven) on the disk have been allocated to that file,  */
/*      then the file is dense: there is a physical sector allocated for    */
/*      each logical sector, and the physical size of the file is the same  */
/*      as the logical size.  This is the normal case: files created by the */
/*      editor, object files, command files, and most files created by      */
/*      programs have this structure.  A sparse file, on the other hand,    */
/*      has fewer physical records than logical records.  Thus a file with  */
/*      8 logical sectors might have as few as one physical sector.  Such   */
/*      files can only be created with the CP/M random write system call,   */
/*      and are typically used by specialized programs such as database     */
/*      managers and indexed access file handlers.                          */
/*                                                                          */
/*      PIP automatically accomodates both dense and sparse files and       */
/*      copies only those sectors which are actually allocated in the       */
/*      source when copying a sparse file.  The resulting destination file  */
/*      is also sparse.  PIP does not allow sparse files to be concatenated.*/
/*                                                                          */
/* 3.3 Disks                                                                */
/*                                                                          */
/*      A disk name has the form d: where d is a letter from A through P.   */
/*      Disk names are used by PIP as a form of shorthand to cut down on    */
/*      the number of keystrokes needed to achieve a given effect.  In      */
/*      effect, the disk name stands in for a filename or list of filenames */
/*      which the user has specified elsewhere on the same command line.    */
/*      4.4 and 4.6 below show the two cases where as disk name may be used.*/
/*                                                                          */
/* 4   PIP COMMANDS                                                         */
/*                                                                          */
/*      The general format of PIP commands is                               */
/*                                                                          */
/*              dest[options]=source1[options],source2[options]...          */
/*                                                                          */
/*              where dest      is a disk, a logical device or an unambig-  */
/*                              uous filename (no wildcards)                */
/*                    source1   is a disk, a logical device, or an          */
/*                              ambiguous or unambiguous file name          */
/*                    source2   is a logical device or unabiguous file name */
/*                              (there may be more than one, separated by   */
/*                              commas)                                     */
/*                    options   enclosed in square brackets may appear      */
/*                              after any destination or source             */
/*                                                                          */
/*      Not all possible combinations are valid.  The remainder of this     */
/*      section details those which are.  For details of valid options, see */
/*      the next section.                                                   */
/*                                                                          */
/* 4.1 Device to device copy                                                */
/*                                                                          */
/*      dst:=src:                                                           */
/*                                                                          */
/*      copies data from the logical device src: to the logical device      */
/*      dst:.  Both src: and dst: are three-character logical device names  */
/*      followed by a colon.  Note that a single letter followed by a colon */
/*      is NOT a logical device name, but a disk name (see below.)  For     */
/*      example,                                                            */
/*                                                                          */
/*              LST:=AXI:                                                   */
/*                                                                          */
/*      copies data from the auxiliary input (typically a serial line or    */
/*      tape reader) to the list device, typically a printer.               */
/*                                                                          */
/* 4.2 File to device copy                                                  */
/*                                                                          */
/*      src:=ufn        (ufn is an unambiguous file name)                   */
/*                                                                          */
/*      copies a single file with an unambiguous name to a destination      */
/*      device.  For example to print the file PROGRAM.LST,                 */
/*                                                                          */
/*              PRT:=PROGRAM.LST                                            */
/*                                                                          */
/* 4.3 Device to file copy                                                  */
/*                                                                          */
/*      ufn=dst:                                                            */
/*                                                                          */
/*      copies data from a logical device to a file.  This function is      */
/*      useful for reading disk files from paper tape and for creating      */
/*      short text files from the keyboard.  These two functions would be   */
/*      performed by                                                        */
/*                                                                          */
/*              B:PROGRAM.ASM=AXI:                                          */
/*              COMMAND.SUB=CON:                                            */
/*                                                                          */
/* 4.4 File to file copy                                                    */
/*                                                                          */
/*      ufn=ufn    ufn=d:    d:=ufn                                         */
/*                                                                          */
/*      copies one unambiguously named file to another.  For example, to    */
/*      make a backup copy of a file VALUABLE.DAT in VALUABLE.BAK, use      */
/*                                                                          */
/*              VALUABLE.BAK=VALUABLE.DAT                                   */
/*                                                                          */
/*      File attributes and, in the case of CP/M 3.0 and MP/M, extended file*/
/*      information are copied along with the file.  Thus a copy of a read- */
/*      only file is automatically made read only, and a copy of an MP/M    */
/*      file with password protection on write has similar protection.      */
/*                                                                          */
/*      Special cases of this command can be used to copy an unambiguously  */
/*      named file from one disk to a file of the same name on another      */
/*      disk.  To copy VALUABLE.DAT on drive A to VALUABLE.DAT on drive B,  */
/*      either                                                              */
/*                                                                          */
/*              B:VALUABLE.DAT=A:  or  B:=A:VALUABLE.DAT                    */
/*                                                                          */
/*      does the job.  Both are shorter than the long form                  */
/*                                                                          */
/*              B:VALUABLE.DAT=A:VALUABLE.DAT                               */
/*                                                                          */
/* 4.5 Concatenation                                                        */
/*                                                                          */
/*      dst=src1,src2,...srcn                                               */
/*                                                                          */
/*      The commands of 4.1 to 4.4 (except the special cases of 4.4) can    */
/*      be generalized to concatenate data from a variety of sources into a */
/*      single destination file or device.  Each argument is a device       */
/*      name or unambiguous file name.  PIP reads each source in turn,      */
/*      copying it to the destination.  For example, to append data from    */
/*      paper tape to an existing file:                                     */
/*                                                                          */
/*              BIGGER.SRC=SMALLER.SRC,AXI:                                 */
/*                                                                          */
/* 4.6 Multiple file copy                                                   */
/*                                                                          */
/*      d:=afn    d:[Gn]=afn    (afn is an ambiguous file name)             */
/*                                                                          */
/*      When an ambiguous filename is used as a source, PIP recognizes that */
/*      it must copy each file matched by the source name to a destination  */
/*      file of the same name, but on a different disk or belonging to a    */
/*      different user (the Gn option specifies user n.)  It is an error to */
/*      try copying files belonging to one user on one disk to the same     */
/*      user on the same disk!  As an example, to copy all .TXT files from  */
/*      the current drive to drive B (which must not be the current drive): */
/*                                                                          */
/*              B:=*.TXT                                                    */
/*                                                                          */
/*      To copy all files belonging to the current user on drive B to user  */
/*      7 (who must not be the current user) on drive B:                    */
/*                                                                          */
/*              B:[G7]=B:*.*                                                */
/*                                                                          */
/* 5   OPTIONS                                                              */
/*                                                                          */
/*      A list of options enclosed in square brackets may follow any file   */
/*      or device name.  Each option consists of a letter, possibly followed*/
/*      by a decimal number n or a control-Z terminated string s.  Where an */
/*      option requires neither string nor number, any following number or  */
/*      string is misinterpreted as more options.  An error always results. */
/*      A missing number is taken to be 1.  There need be no separators     */
/*      between options, though spaces are allowed.  Most combinations of   */
/*      options are accepted without objection, although some make little   */
/*      sense and may produce unexpected results.                           */
/*                                                                          */
/*      Any letter is accepted as an option, although some (for example B)  */
/*      have no effect.  The following table lists implemented options      */
/*      and the context (following source or destination in which they make */
/*      sense:                                                              */
/*                                                                          */
/*      Opt  | Src | Dst | Description                                      */
/*      -----+-----+-----+----------------------------------------------    */
/*      A    | Yes | No  | Copy only those disk files which DO NOT have the */
/*           |     |     |  archive bit set in at least one extent          */
/*           |     |     |  (selective back-up of modified files)           */
/*      Dn   | Yes | No  | Delete characters after column n (truncate for   */
/*           |     |     |  narrow paper or screen)                         */
/*      E    | Yes | No  | Echo data to console as it is transfered         */
/*      F    | Yes | No  | Remove form feeds from data as it is copied      */
/*      Gn   | Yes | Yes | Specify (Get) user number for source or          */
/*           |     |     |  destination disk file                           */
/*      H    | Yes | No  | Check that the data is a valid Intel Hex file    */
/*      I    | Yes | No  | Ignore (do not transfer) final :00 record in     */
/*           |     |     |  Intel Hex file (used for concatenating files)   */
/*      L    | Yes | No  | Translate lower case alphabetic to upper         */
/*      Nn   | Yes | No  | Prepend line numbers to output data.  Leading 0's*/
/*           |     |     |  not supressed if n=2                            */
/*      O    | Yes | No  | Object file.  Ignore end-of-file marks           */
/*      Pn   | Yes | No  | Output page eject (form feed) every n lines      */
/*           |     |     |  (default 60)                                    */
/*      Qs   | Yes | No  | Quit (stop) transfering data after the string s  */
/*           |     |     |  has been copied                                 */
/*      R    | Yes | No  | Read system files (system attribute set).  By    */
/*           |     |     |  default such files are not read                 */
/*      S    | Yes | No  | Start transfering data when the string s is      */
/*           |     |     |  found in input data.  s is first data copied    */
/*      Tn   | Yes | No  | Expand tabs at every n columns (default 8)       */
/*      U    | Yes | No  | Translate lower case alphabetic to upper         */
/*      V    | Yes | No  | Check destination disk file for readability (not */
/*           |     |     |  correctness) after transfer complete            */
/*      W    | Yes | No  | Overwrite read-only destination file (supress    */
/*           |     |     |  PIP's normal query to user before overwriting)  */
/*      Z    | Yes | No  | Zero parity bit.  Normally parity bit is copied  */
/*                                                                          */
/* 6   HOW IT WORKS                                                         */
/*                                                                          */
/*      The best way to understand this somewhat knotty program is to start */
/*      at the _main function which is to be found at the end of this       */
/*      listing and work backwards to progressively lower level functions   */
/*      as and when it becomes necessary to know what they do and how they  */
/*      do it.                                                              */
/*                                                                          */
/*      For historical reasons, much of the communication between functions */
/*      is accomplished with global variables rather than parameters,       */
/*      resulting in long-range interdependencies which may be hard to      */
/*      follow.  A cross-reference listing is useful in tracking these      */
/*      down.  All the global variables are global because they have to be  */
/*      that way: if you see one, at least two functions use it.            */
/*                                                                          */
/*                                      Dominic Dunlop, Zilog Inc. 821022   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*         E X T E R N A L   A N D    B D O S   I N T E R F A C E           */
/*         ------------------------------------------------------           */
/*                                                                          */
/****************************************************************************/

/* CP/M-386: the headers below stood in for a C library that is not
   present here; the shim supplies what this program actually uses,
   sbrk included. */

/****************************************************************************/
/*                                                                          */
/*          C P / M - 3 8 6   R U N T I M E   S H I M                       */
/*          -----------------------------------------                       */
/*                                                                          */
/*      Everything below stands in for the CP/M-68K C startup and the       */
/*      portab.h / bdos.h / basepage.h / setjmp.h headers this program was   */
/*      written against.  There is no C library: the only way out of the     */
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

struct fcbtab   *fcb;                           /* Default file control blk */
char            *buff;                          /* Default DMA buffer       */

char            copyright[] =
        " (10/04/82) Portable CP/M PIP vers 1.0 ";


/****************************************************************************/
/*                        Version dependencies                              */
/****************************************************************************/

#define CPM             0x0000                  /* Vanilla flavor CP/M      */
#define MPM             0x1000                  /* MP/M                     */
#define PCPM            0x2000                  /* Portable CP/M            */
#define ONE_X           0x00                    /* Version 1.x              */
#define TWO_X           0x20                    /* Version 2.x              */
#define THREE_X         0x30                    /* Version 3.x              */

#define IS_MPM(x)       (((x) & 0xff00) == MPM)
#define HAS_GET_DFS(x)  ((((x) & 0xff00) != CPM) || (((x) & 0xf0) >= THREE_X))
#define HAS_GSET_SCB(x) (((x) & 0xf0) >= THREE_X)
#define HAS_RETERR(x)   ((((x) & 0xff00) == MPM) || (((x) & 0xf0) >= THREE_X))
#define HAS_SETMSC(x)   ((((x) & 0xff00) == MPM) || (((x) & 0xf0) >= THREE_X))
#define HAS_XFCBS(x)    ((((x) & 0xff00) == MPM) || (((x) & 0xf0) >= THREE_X))
#define VOID_GET_DPB(x) (((x) & 0xff00) == PCPM)


/****************************************************************************/
/*                                                                          */
/*                 G E N E R A L   D E F I N I T I O N S                    */
/*                 -------------------------------------                    */
/*                                                                          */
/*      NOTE: definitions related to particular data structures appear      */
/*            adjacent to the definition of the structure                   */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* Useful charcaters                                                        */
/****************************************************************************/

#define TAB             0x09                    /* Horizontal TAB           */
#define FF              0x0c                    /* Form feed                */
#define CR              0x0d                    /* Carriage return          */
#define LF              0x0a                    /* Line feed                */
#define ENDFILE         0x1a                    /* CP/M end of file mark    */
#define WILD            '?'                     /* Filename wildcard        */


/****************************************************************************/
/* Lengths of filename fields                                               */
/****************************************************************************/

#define L_NAME          8                       /* Length of filename       */
#define L_PASS          8                       /* Length of password       */
#define L_TYPE          3                       /* Length of file type      */


/****************************************************************************/
/* Types for source and destination in copy                                 */
/****************************************************************************/

#define OUTT            0                       /* Output device            */
#define PRNT            1                       /* Printer                  */
#define LSTT            2                       /* List device              */
#define AXOT            3                       /* Auxilary output device   */
#define FILE            4                       /* File type                */
#define CONS            5                       /* Console                  */
#define AXIT            6                       /* Auxilary input device    */
#define INPT            7                       /* Input device             */
#define NULT            8                       /* Null characters          */
#define EOFT            9                       /* EOF character            */
#define ERR             10                      /* Error type               */
#define DISKNAME        11                      /* Diskname letter          */



/****************************************************************************/
/* Constants associated with disk layout etc.                               */
/****************************************************************************/

#define C_MAXMBUF       (C_MAXMCNT * SECSIZE)   /* CPM/3 max transfer length*/
#define C_MAXMCNT       ((UWORD) 128)           /* CP/M-3 max multi sec cnt */
#define EXTMSK          ~0x7f                   /* Ignore low-order bits    */
#define EXTRECS         128                     /* Records per extent       */
#define M_MAXMBUF       (M_MAXMCNT * SECSIZE)   /* CPM/3 max transfer length*/
#define M_MAXMCNT       ((UWORD) 16)            /* MP/M max multi sector cnt*/
#define MP_SHF          8                       /* Converts MP/M multi-     */
                                                /*   sector return to bytes */
#define SECMSK          ~0x7f                   /* Ignore low-order bits    */
#define SECSIZE         128                     /* No of bytes in record    */


/****************************************************************************/
/* File attribute fields in FCB                                             */
/****************************************************************************/

#define F1              fname[0]                /* User flag #1             */
#define F2              fname[1]                /* User flag #2             */
#define F3              fname[2]                /* User flag #3             */
#define F4              fname[3]                /* User flag #4             */
#define ASSIGN_PW       fname[5]                /* Assign password          */
#define NO_WRITE        fname[6]                /* You can't write this file*/
#define USER_0          fname[7]                /* Can be read by all users */
#define R_O             ftype[0]                /* Nobody can write file    */
#define SYSTEM          ftype[1]                /* "Invisible" file         */
#define ARC             ftype[2]                /* Archived file            */


/****************************************************************************/
/* Any other business                                                       */
/****************************************************************************/

#define DEF_TAB         8                       /* Default tab stops        */
#define FORCE_READ      0xffff                  /* Index causes source read */
#define LPP             60                      /* Lines per printer page   */
#define MAXDRIVE        ('P' - 'A' + 1)         /* Max valid drive number   */
#define MAXUSER         0xf                     /* Max valid user number    */
#define NIBBLE          4                       /* Bits in a nibble         */
#define NONE            0                       /* No extended error code   */
#define NULLS           40                      /* Length of null trailer   */
#define SAFETY          1024                    /* Stack margin in sbrk call*/
#define SEARFCB         fcb                     /* Search fcb in multi copy */


/****************************************************************************/
/*                                                                          */
/*                       G L O B A L   D A T A                              */
/*                       ---------------------                              */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* Simple types                                                             */
/****************************************************************************/

BYTE            f1;                             /* F1 user attribute flag    */
BYTE            f2;                             /* F2 user attribute flag    */
BYTE            f3;                             /* F3 user attribute flag    */
BYTE            f4;                             /* F4 user attribute flag    */
BYTE            ro;                             /* Read only attribute flag  */
BYTE            sys;                            /* System attribute flag     */

BOOLEAN         ambig;                          /* File is ambig type       */
BOOLEAN         concat;                         /* Concatination command    */
BOOLEAN         dblbuf;                         /* Double buffering needed  */
BOOLEAN         dfile;                          /* Dest is file type        */
BOOLEAN         endofsrc;                       /* End of source file       */
BOOLEAN         eretry;                         /* Error return flag        */
BOOLEAN         fastcopy;                       /* Copy directly to dbuf    */
BOOLEAN         getpw;                          /* No dst passwd given      */
BOOLEAN         insparc;                        /* In middle of sparse file */
BOOLEAN         made;                           /* Dest file already made   */
BOOLEAN         multcom;                        /* Handling multiple command*/
BOOLEAN         nendcmd;                        /* Not end of command tail  */
BOOLEAN         putnum;                         /* Ready for next line num  */
BOOLEAN         sfile;                          /* Source is file type      */
BOOLEAN         sparfil;                        /* Sparce file being copied */

char            ch;                             /* Last character scanned   */
char            *dbase;                         /* Destination buffer base  */
char            *sbase;                         /* Source buffer base       */
                                                /****************************/
                                                /* Note: size of buffers     */
                                                /*   depends on available   */
                                                /*   memory                 */
                                                /****************************/

UWORD           bufsize;                        /* Multi sector buffer size */
UWORD           cdisk;                          /* Current disk             */
UWORD           column;                         /* Column count for tabs    */
UWORD           concnt;                         /* Counter for abort check  */
UWORD           c_user;                         /* Current user number      */
UWORD           last_user = 0;                  /* Remember last user code  */
UWORD           dblen;                          /* Dest buffer length       */
UWORD           dcnt;                           /* Error/directory code     */
UWORD           exten;                          /* Extended error code      */
UWORD           feedbase;                       /* String search base       */
UWORD           feedlen;                        /* Length of search string  */
UWORD           matchlen;                       /* Lenght of matched string */
UWORD           ndest;                          /* Index of next dest char  */
UWORD           nsbuf;                          /* Next source sector index */
UWORD           nsource;                        /* Index of next src char   */
UWORD           odcnt;                          /* Return from open of dest */
UWORD           page_line;                      /* Line within page         */
UWORD           quitlen;                        /* Length of quit string    */
UWORD           sblen;                          /* Source buffer length     */
UWORD           ver;                            /* CP/M version number      */

long            filsize;                        /* File size (24 bits only) */
long            line_no;                        /* Line count on printer    */

/****************************************************************************/
/* Character arrays                                                         */
/****************************************************************************/

                                                /* Option toggles, one for  */
                                                /*   each letter (although  */
char            cont[26];                       /*   some letters unused)   */

                                                /****************************/
                                                /* Functions of elements in */
                                                /*   cont[].  Most act as   */
                                                /*   boolean, though some   */
                                                /*   hold numeric values    */
                                                /****************************/
#define ARCHIV          cont[0]                 /* File archive             */
#define DELET           cont[3]                 /* Delete characters        */
#define ECHO            cont[4]                 /* Echo console characters  */
#define FORMF           cont[5]                 /* Form filter              */
#define GETU            cont[6]                 /* Get file, user number    */
#define HEXT            cont[7]                 /* Hex file transfer        */
#define IGNOR           cont[8]                 /* Ignore :00 record on file*/
#define KILDS           cont[10]                /* Kill filename display    */
#define LOWER           cont[11]                /* Translate to lower case  */
#define NUMB            cont[13]                /* Number output lines      */
#define OBJ             cont[14]                /* Object file transfer     */
#define PAGCNT          cont[15]                /* Page length              */
#define QUITS           cont[16]                /* Quit copy                */
#define RSYS            cont[17]                /* Read system files        */
#define STARTS          cont[18]                /* Start copy               */
#define TABS            cont[19]                /* TAB set                  */
#define UPPER           cont[20]                /* Upper case translate     */
#define VERIF           cont[21]                /* Verify equal files only  */
#define WRROF           cont[22]                /* Write to r/o file        */
#define ZEROP           cont[25]                /* Zero parity on input     */


/****************************************************************************/
/* Standard and extended error messages                                     */
/****************************************************************************/

char    *errmsg[] =                             /* Standard messages        */
        {
                "DISK READ",                    /*   0                      */
                "DISK WRITE",                   /*   1                      */
                "VERIFY",                       /*   2                      */
                "INVALID DESTINATION",          /*   3                      */
                "INVALID SOURCE",               /*   4                      */
                "USER ABORTED",                 /*   5                      */
                "BAD PARAMETER",                /*   6                      */
                "INVALID USER NUMBER",          /*   7                      */
                "INVALID FORMAT",               /*   8                      */
                "HEX RECORD CHECKSUM",          /*   9                      */
                "FILE NOT FOUND",               /*  10                      */
                "START NOT FOUND",              /*  11                      */
                "QUIT NOT FOUND",               /*  12                      */
                "INVALID HEX DIGIT",            /*  13                      */
                "CLOSE FILE",                   /*  14                      */
                "UNEXPECTED END OF HEX FILE",   /*  15                      */
                "INVALID SEPARATOR",            /*  16                      */
                "NO DIRECTORY SPACE",           /*  17                      */
                "INVALID FORMAT WITH SPARCE FILE",/*18                      */
                "MAKE FILE",                    /*  19                      */
                "OPEN FILE",                    /*  20                      */
                "PRINTER BUSY",                 /*  21                      */
                "CAN'T DELETE TEMP FILE",       /*  22                      */
                "OBSOLETE FEATURE"              /*  23                      */
        };


char    *extmsg[] =                             /* Extended error messages  */
        {
                "",                             /*   0                      */
                "NON-RECOVERABLE",              /*   1                      */
                "R/O DISK",                     /*   2                      */
                "R/O FILE",                     /*   3                      */
                "INVALID DISK SELECT",          /*   4                      */
                "INCOMPATIBLE MODE",            /*   5                      */
                "FCB CHECKSUM",                 /*   6                      */
                "INVALID PASSWORD",             /*   7                      */
                "ALREADY EXISTS",               /*   8                      */
                "INVALID FILENAME",             /*   9                      */
                "LIMIT EXCEEDED",               /*  10                      */
                "INTERNAL LOCK LIMIT EXCEEDED"  /*  11                      */
        };

#define NUMMSGS         (sizeof extmsg / sizeof (char *))


/****************************************************************************/
/* Option - transfer type mapping table                                     */
/****************************************************************************/

#define FAST            0                       /* Fast file to file copy   */
#define CHRT            1                       /* Character transfer option*/
#define DUBL            2                       /* Double buffer required   */

char    optype[26] =
        {
                FAST,                           /* A option                 */
                FAST,                           /* B option                 */
                FAST,                           /* C option                 */
                DUBL,                           /* D option                 */
                CHRT,                           /* E option                 */
                DUBL,                           /* F option                 */
                FAST,                           /* G option                 */
                CHRT,                           /* H option                 */
                DUBL,                           /* I option                 */
                FAST,                           /* J option                 */
                FAST,                           /* K option                 */
                CHRT,                           /* L option                 */
                FAST,                           /* M option                 */
                DUBL,                           /* N option                 */
                FAST,                           /* O option                 */
                DUBL,                           /* P option                 */
                DUBL,                           /* Q option                 */
                FAST,                           /* R option                 */
                DUBL,                           /* S option                 */
                DUBL,                           /* T option                 */
                CHRT,                           /* U option                 */
                FAST,                           /* V option                 */
                FAST,                           /* W option                 */
                FAST,                           /* X option                 */
                FAST,                           /* Y option                 */
                CHRT                            /* Z option                 */
        };


/****************************************************************************/
/* Logical device name table                                                */
/****************************************************************************/

#define DEVLEN          3                       /* Length of device name    */
#define TYPES           (sizeof io / DEVLEN)    /* Number of devices        */

char    io[] [DEVLEN] =                         /* Logical device names     */
        {
                'O', 'U', 'T',
                'P', 'R', 'N',
                'L', 'S', 'T',
                'A', 'X', 'O',
                 0,   0,   0,                   /* Dummy for file type      */
                'C', 'O', 'N',
                'A', 'X', 'I',
                'I', 'N', 'P',
                'N', 'U', 'L',
                'E', 'O', 'F',
        };

/****************************************************************************/
/* Structures                                                               */
/****************************************************************************/

jmp_buf         main_stack;                     /* Used in error recovery */


/****************************************************************************/
/* Console command buffer                                                   */
/****************************************************************************/

struct  {
                BYTE    maxlen;                 /* Max buffer length        */
                BYTE    comlen;                 /* Current length           */
                char    comline[128];           /* Command buffer contents  */
                UWORD   cbp;                    /* Command buffer pointer   */
        } combuf;


/****************************************************************************/
/* File control blocks, expanded to carry password and device type data     */
/****************************************************************************/

struct flctlb
        {
                struct  fcbtab flfcb;
                char    pwnam[L_PASS];
                BYTE    pwmode;
                BYTE    user;
                BYTE    type;
        } source;                               /* Source file description  */


struct flctlb   dest;                           /* Temporary destination    */
                                                /*   file (scratch file)    */
struct flctlb   odest;                          /* True destination file    */

MLOCAL UBYTE    srclrbc;                        /* CP/M-386: source file's  */
                                                /*   last record byte count */

struct fcbtab   dxfcb;                          /* Extended FCB (XFCB) for  */
                                                /*   destination file       */


struct flctlb   empty_fcb =                     /* Empty for initialization */
        {
                0,                              /* Drive, name,             */
                ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                ' ', ' ', ' ',                  /*   type, extent, s1, s2,  */
                0, 0, 0, 0,                     /*   record,  reserved,     */
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0,                              /*   current & random record*/
                ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                1,                              /*   password, passwd mode, */
                0,                              /*   user number,           */
                ERR                             /*   file type              */
        };

/****************************************************************************/
/*                                                                          */
/*                  L O W   L E V E L   F U N C T I O N S                   */
/*                  -------------------------------------                   */
/*                                                                          */
/****************************************************************************/

                /********************************/
                /*                              */
                /*         G E T C H            */
                /*                              */
                /********************************/

#define getch(a)        (a = gnc())             /*  get a char in a with gnc*/


                /********************************/
                /*                              */
                /*           C R L F            */
                /*                              */
                /********************************/

VOID                                            /* Send carriage-return, line*/
crlf()                                          /*   feed to console         */
{
        _conout(LF);
        _conout(CR);
}


                /********************************/
                /*                              */
                /*         P R I N T X          */
                /*                              */
                /********************************/

VOID                                            /* Print a null-terminated   */
printx(a)                                       /*   string on the console   */
register char *a;
{
        while (*a)
                _conout(*a++);
}


                /********************************/
                /*                              */
                /*          P R I N T           */
                /*                              */
                /********************************/

VOID                                            /* Print a null-terminated  */
print(a)                                        /*   string on a new line   */
char *a;                                        /*   on the console         */
{
        crlf();
        printx(a);
}


                /********************************/
                /*                              */
                /*          R D C O M           */
                /*                              */
                /********************************/

VOID                                            /* Read a command line from */
rdcom()                                         /*   the console into combuf*/
{
        combuf.maxlen = sizeof combuf.comline;  /* 128 characters maximum   */
        _conbuf(&combuf);
}


                /********************************/
                /*                              */
                /*          P R N A M E         */
                /*                              */
                /********************************/

VOID                                            /* Print a file name        */
prname(fcba)
struct fcbtab *fcba;
{
        register int    i, c;

        for (i = 0; i < L_NAME + L_TYPE; i++)   /* Do not print spaces      */
        {                                       /* CP/M-386: fname and ftype*/
                                                /*   are separate arrays    */
                c = ((i < L_NAME) ? fcba->fname[i]
                                  : fcba->ftype[i - L_NAME]) & 0x7f;
                if (c != ' ')
                {
                        if (i == L_NAME)        /* '.' separates name & type*/
                                _conout('.');
                        _conout(((c<0x20)||(c>0x7e))?'?':c);
                }
        }
}


                /********************************/
                /*                              */
                /*           O P E N            */
                /*                              */
                /********************************/

UWORD                                           /* Open a file, using passwd*/
open(fcba)                                      /*   on MP/M, CP/M 3        */
struct flctlb   *fcba;                          /* Returns error code       */
{
        if (HAS_XFCBS(ver))                     /* If password required,    */
                _setdma(fcba->pwnam);           /*   show where it is       */
        dcnt = _open(fcba);                     /* Try the open             */
        if ((dcnt != 255)                       /* If it worked, but found  */
           && (fcba->flfcb.USER_0 & 0x80))      /*   a user zero file,      */
        {                                       /*   behave as if it failed */
                _close(fcba);
                dcnt = 255;
        }
        exten = dcnt >> 8;                      /* Separate low & high bytes*/
        return (dcnt &= 0xff);                  /*   of error code          */
}


                /********************************/
                /*                              */
                /*          C L O S E           */
                /*                              */
                /********************************/

UWORD                                           /* Close a file.  Returns   */
close(flcb)                                     /*   error code             */
struct flctlb   *flcb;
{
        dcnt = _close(flcb);
        exten = dcnt >> 8;
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*        S E A R C H           */
                /*                              */
                /********************************/

UWORD                                           /* Search for the first file*/
search(flcb)                                    /*   with a name matching   */
struct flctlb   *flcb;                          /*   that in flcb           */
{                                               /* Delivers directory record*/
        dcnt = _srch_1st(flcb);                 /*   containing match in    */
        exten = dcnt >> 8;                      /*   current DMA buffer     */
        return (dcnt &= 0xff);                  /* Returns index/error code */
}


                /********************************/
                /*                              */
                /*        S E A R C H N         */
                /*                              */
                /********************************/

UWORD                                           /* Search for the next file*/
searchn()                                       /*   with a name matching   */
{                                               /*   that set up by search()*/
                                                /* Delivers directory record*/
        dcnt = _srch_next();                    /*   containing match in    */
        exten = dcnt >> 8;                      /*   current DMA buffer     */
        return (dcnt &= 0xff);                  /* Returns index/error code */
}


                /********************************/
                /*                              */
                /*         D E L E T E          */
                /*                              */
                /********************************/

UWORD                                           /* Delete a file, giving    */
delete(flcb)                                    /*   password if it might   */
struct flctlb   *flcb;                          /*   be necessary           */
{                                               /* Returns error code       */
        if (HAS_XFCBS(ver))
                _setdma(flcb->pwnam);
        dcnt = _delete(flcb);
        exten = dcnt >> 8;
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*         D I S K R D          */
                /*                              */
                /********************************/

UWORD                                           /* Read file sequentially  */
diskrd(flcb)                                    /* Reads no of records set */
struct flctlb   *flcb;                          /*   by last call to       */
{                                               /*   setmsc(), or 1 if     */
        dcnt = _s_read(flcb);                   /*   function not available*/
        exten = dcnt >> 8;                      /* Returns error code      */
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*      D I S K W R I T E       */
                /*                              */
                /********************************/

UWORD                                           /* Write file sequentially */
diskwrite(flcb)                                 /* Writes no of records set*/
struct flctlb   *flcb;                          /*   by last call to       */
{                                               /*   mulsect(), or 1 if    */
        dcnt = _s_write(flcb);                  /*   function not available*/
        exten = dcnt >> 8;                      /* Returns error code      */
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*           M A K E            */
                /*                              */
                /********************************/

UWORD                                           /* Create a new file,       */
make(fcba)                                      /*   assigning password if  */
struct flctlb   *fcba;                          /*   necessary              */
{                                               /* Returns error code       */
        if (HAS_XFCBS(ver))                     /* Password allowed?        */
                if (fcba->pwnam[0] == 0)        /* Yes: wanted?             */
                        fcba->flfcb.ASSIGN_PW &= 0x7f;  /* No               */
                else                            /* Password is wanted       */
                {
                        fcba->flfcb.ASSIGN_PW |= 0x80;
                        _setdma(fcba->pwnam);   /* Show where it is         */
                }
        dcnt = _create(fcba);
        exten = dcnt >> 8;
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*        R E N A M E           */
                /*                              */
                /********************************/

UWORD                                           /* Rename file, giving pass-*/
rename(flcb)                                    /*   word if it might be    */
struct flctlb   *flcb;                          /*   necessary              */
{                                               /* Returns error code       */
        if (HAS_XFCBS(ver))
                _setdma(flcb->pwnam);           /* Say where password is    */
        dcnt = _rename(flcb);
        exten = dcnt >> 8;
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*         S E T A T T          */
                /*                              */
                /********************************/

UWORD                                           /* Set the attributes of a  */
setatt(flcb)                                    /*   file (=== assumes pass-*/
struct flctlb   *flcb;                          /*   word already set up if */
{                                               /*   required===)           */
        dcnt = _set_att(flcb);                  /* Returns error code       */
        exten = dcnt >> 8;
        return (dcnt &= 0xff);
}


                /********************************/
                /*                              */
                /*        S E T U S E R         */
                /*                              */
                /********************************/

VOID                                            /* Set the user code        */
setuser(user)
BYTE    user;
{

        if (last_user != user)                  /* Only call BDOS if user   */
                _gset_ucode(last_user = user);  /*   has changed            */
}


                /********************************/
                /*                              */
                /*        M U L T S E C T       */
                /*                              */
                /********************************/

VOID                                            /* Set multisector count    */
multsect(cnt)
UWORD   cnt;
{
        static last_count = 1;                  /* Remember last count      */

        if (! HAS_SETMSC(ver))                  /* Do nothing if call not   */
                return;                         /*   supported              */
        if (last_count != cnt)                  /* Only call BDOS if new    */
                _setmsc(last_count = cnt);      /*   count differs from last*/
}


                /********************************/
                /*                              */
                /*           M O V E            */
                /*                              */
                /********************************/

VOID                                            /* Move n bytes from source */
move(s, d, n)                                   /*   to destination         */
register char   *s, *d;
register UWORD  n;
{
        if (!n) return;
        do
                *d++ = *s++;
        while (--n);
}

/****************************************************************************/
/*                                                                          */
/*                    E R R O R   R E P O R T I N G                         */
/*                    -----------------------------                         */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*          E R R O R           */
                /*                              */
                /********************************/

error(errtype, exended, retflag, fileadr)       /* Called following errors  */
UWORD           errtype, exended;               /*   which terminate current*/
BOOLEAN         retflag;                        /*   command, prints message*/
struct flctlb   *fileadr;                       /*   cleans up and returns  */
{                                               /*   control to main()      */
        eretry = retflag & ambig;               /* Error retry = retry flag */
        multsect(1);                            /* Forget current msc       */

        if (sfile)                              /* If source is a file,     */
        {                                       /*   close it               */
                setuser(source.user);
                close(&source);
        }

        if (made)                               /* If destination scratch   */
        {                                       /*   file already created,  */
                setuser(odest.user);            /*   delete it              */
                close(&dest);
                delete(&dest);                  /* Delete dest scratch file */
        }

        print("ERROR: ");                       /* Print requested message  */
        printx(errmsg[errtype]);
        if ((exended &= 0x0f) < NUMMSGS)        /*   and extended message if*/
        {                                       /*   there is one           */
                _conout(' ');
                printx(extmsg[exended]);
        }
        if (fileadr != 0)                       /* If error involves a file,*/
        {                                       /*   incriminate it         */
                printx(" - ");
                _conout(fileadr->flfcb.drive + 'A' - 1);
                _conout(':');
                prname(&fileadr->flfcb);
        }

        combuf.comlen = 0;                      /* Trash current cmd line   */
        crlf();
        longjmp(main_stack, TRUE);              /* Restart from top of main()*/
}


                /********************************/
                /*                              */
                /*  N O N F I L E _ E R R O R   */
                /*                              */
                /********************************/

VOID                                            /* Same as above, but with  */
nonfile_error(errtype)                          /*   defaults for all but   */
UWORD errtype;                                  /*   first parameter        */
{
        error(errtype, NONE, FALSE, (struct flctlb *) 0);
}

/****************************************************************************/
/*                                                                          */
/*                F I L E   I N P U T  /  O U T P U T                       */
/*                -----------------------------------                       */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*      S E T U P D E S T       */
                /*                              */
                /********************************/

VOID                                            /* Set up the destination   */
setupdest()                                     /*   file                   */
{
        setuser(odest.user = c_user);           /* Set up destination user  */
        dest = odest;                           /* Temporary destination    */
        move("$$$", dest.flfcb.ftype, L_TYPE);  /*   has original name but  */
                                                /*   "$$$" type             */

        if (HAS_XFCBS(ver))                     /* Are we worrying about    */
        {                                       /*   passwords?             */
                odest.flfcb.ASSIGN_PW |= 0x80;  /* Yes: tell BDOS           */
                odcnt = open(&odest.flfcb);     /* Try to open dest file    */
                                                /*   and save error code    */
                if (odcnt != 255)               /* If it exists, close it   */
                        close(&odest);
                else                            /* If file exists, but we   */
                        if ((exten & 0x0f) != 0)/*   can't open it, error   */
                                error(20, exten, TRUE, &odest);

                if ((delete(&dest) == 255)      /* Try to delete old temp   */
                   && (exten != 0))             /*   file.  If it exists but*/
                                                /*   can't be deleted, error*/
                        error(22, exten, TRUE, &dest);

                if (make(&dest) == 255)         /* Create new temp file     */
                        error(19, exten, FALSE, &dest);/* Make file error   */
        }
        else                                    /* No passwords             */
        {
                delete(&dest);                  /* Remove old temp file     */
                if (make(&dest) == 255)         /* Create a new one         */
                        error(17, NONE, FALSE, &dest);/* No directory space */
        }

        CURREC(&dest.flfcb) = 0;                /* Current record is 0      */
        made = TRUE;                            /* Show temp file now made  */
}


                /********************************/
                /*                              */
                /*    S E T U P S O U R C E     */
                /*                              */
                /********************************/

VOID                                            /* Prepare to use a source  */
setupsource()                                   /*   file                   */
{
        register int    i;

        setuser(source.user);                   /* Set up source user       */
        if (HAS_XFCBS(ver))                     /* Do we have to worry about*/
                source.flfcb.ASSIGN_PW |= 0x80; /*   passwords?             */

        CURREC(&source.flfcb) = LRBC_ASK;       /* CP/M-386: ask for the    */
                                                /*   exact length as we open*/
        open(&source.flfcb);                    /* Open source file         */
        srclrbc = CURREC(&source.flfcb);
        CURREC(&source.flfcb) = 0;              /* Back to record 0 to read */
        if ((! RSYS)                            /* Do we read system files? */
           && (source.flfcb.SYSTEM & 0x80))     /* Is this one?             */
                dcnt = 255;                     /* Yes: pretend open failed */

        if (dcnt == 255)                        /* Was there an error?      */
                                                /* Report error, retry if   */
                                                /*   extended               */
                error(20, exten, ((exten & 0x0f) != 0), &source);

        if (HAS_XFCBS(ver) && getpw)            /* Did user give dest passwd*/
        {                                       /* No: get it from source   */
                dxfcb = source.flfcb;
                if (_get_xfcb(&dxfcb) == 255)   /* Get source file's XFCB   */
                        dest.pwnam[0] = 0;      /* Not found: no password   */
                else                            /* Has a password           */
                {                               /* Get "decrypted" copy     */
                        for (i = 0; i < L_PASS; i++ )
                                dest.pwnam[i] =
                                  dxfcb.resvd[L_PASS - 1 - i] ^
                                    dxfcb.s1;
                        dest.pwmode = dxfcb.extent;/* Copy password mode    */
                }
        }

        f1 = source.flfcb.fname[0] & 0x80;      /* Save source attributes   */
        f2 = source.flfcb.fname[1] & 0x80;
        f3 = source.flfcb.fname[2] & 0x80;
        f4 = source.flfcb.fname[3] & 0x80;
        ro = source.flfcb.ftype[0] & 0x80;
        sys = source.flfcb.ftype[1] & 0x80;

        _filsiz(&source);                       /* Find size of source file */
        filsize = ranrec(&source.flfcb);        /* Virtual size in records  */
        source.flfcb.record = 0;                /* Clear random record no.  */
        nsource = FORCE_READ;                   /* Force read from source   */
}                                               /*   before write to dest   */


                /********************************/
                /*                              */
                /*       W R I T E D E S T      */
                /*                              */
                /********************************/

VOID                                            /* Write data from output   */
writedest()                                     /*   buffer to destination  */
{                                               /*   file.  On entry, ndest */
        BOOLEAN         dataok;                 /*   indexes the byte beyond*/
        register UWORD  j, tdest, n;            /*   the last to be written.*/
        UWORD           chunk;                  /* Writes whole sectors only*/

        if (! made)                             /* Create output file if not*/
                setupdest();                    /*   already done           */

        if ((n = ndest & SECMSK) == 0)          /* Return if nothing to do  */
                return;

        tdest = 0;                              /* Bytes written so far     */
        setuser(odest.user);                    /* Set destination user     */

        if (sparfil |= insparc)                 /* Is this a sparse file    */
        {                                       /*   (fewer physical than   */
                                                /*    virtual records)?     */
                multsect(1);                    /* Yes: write one sector at */
                _setdma(&dbase[tdest]);         /*   a time (random record  */
                if (_b_write(&dest) == 255)     /*   addr already set up)   */
                        error(1, exten, FALSE, &dest);/* Disk write error   */
        }
        else                                    /* Not sparse:              */
                _set_rand(&dest.flfcb);         /* Set base rec for verify  */

        if (HAS_SETMSC(ver) && fastcopy)        /* Can we write more than   */
        {                                       /*   on sector at a time?   */
                bufsize = (IS_MPM(ver)) ?       /* Yes.                     */
                          M_MAXMBUF : C_MAXMBUF;
                multsect((IS_MPM(ver)) ? M_MAXMCNT : C_MAXMCNT);
        }
        else                                    /* No.  Too bad.            */
        {
                bufsize = SECSIZE;
                multsect(1);
        }

                                                /* Strange code overcomes   */
                                                /*   unsigned compare bug   */
        while ((0xffff & (long) (chunk = n - tdest)) >= (long) SECSIZE)
                                                /* Write loop               */
        {
                if (HAS_SETMSC(ver)             /* Is this the last chunk in*/
                   && fastcopy                  /*   a copy using multiple  */
                   && ((0xffff & (long) chunk) < (long) bufsize))
                {
                        bufsize = chunk;        /* Reduce multi sector count*/
                        multsect(bufsize / SECSIZE);/* to correct value     */
                }

                _setdma(&dbase[tdest]);         /* Adjust DMA start address */

                if (diskwrite(&dest) != 0)      /* Write the data           */
                        error(1, exten, FALSE, &dest);  /* Disk write error */
                tdest += bufsize;
        }

        if (VERIF)                              /* Read-after write verify  */
        {                                       /*   required?              */
                tdest = 0;                      /* Yes: start from the      */
                multsect(1);                    /*   beginning of data just */
                _setdma(buff);                  /*   written.  Read one sec */
                                                /*   at a time into default */
                                                /*   DMA buffer             */

                while ((0xffff & (long) tdest) < (long) n)
                                                /* Verification loop        */
                {                               /* Read random record       */
                        dataok = (_b_read(&dest.flfcb) == 0);
                                                /* Move to next random rec  */
                        setranrec(&dest.flfcb, ranrec(&dest.flfcb) + 1);
                                                /* Perform comparison       */
                        for (j = 0; j < SECSIZE; j++)
                                if (dataok |= (buff[j] == dbase[tdest + j]))
                                        break;

                        tdest += SECSIZE;
                        if (! dataok)
                                error(2, NONE, FALSE, &dest);/* Verify error*/
                }
                diskrd(&dest);                  /* Done: move to next       */
        }                                       /*   sequential record      */

                                                /* Move unwritten tail down */
                                                /*   to buffer start        */
        move(&dbase[tdest], dbase, (UWORD)(BYTE)(ndest -= tdest));
}


                /********************************/
                /*                              */
                /*      F I L L S O U R C E     */
                /*                              */
                /********************************/

VOID                                            /* Fill the source buffer   */
fillsource()                                    /*   from the current source*/
{                                               /*   file                   */
        BYTE    extsave;

        if (HAS_SETMSC(ver) && fastcopy)        /* Are we able to read more */
        {                                       /*   than one sector?       */
            bufsize = (IS_MPM(ver)) ?           /* Yes: try to get bufferful*/
                        M_MAXMBUF : C_MAXMBUF;  /*   (size depends whether  */
            multsect((IS_MPM(ver)) ?            /*    MP/M or CP/M-3)       */
                        M_MAXMCNT : C_MAXMCNT);
        }
        else                                    /* No: just get one         */
        {
            bufsize = SECSIZE;
            multsect(1);
        }

        setuser(source.user);                   /* Source user number set   */
        nsource = nsbuf;

        while ((0xffff & (long) (sblen - nsbuf)) >= (long) SECSIZE)
                                                /* Read loop:  exit when    */
        {                                       /*   buff full or file ends */

            if (fastcopy                        /* Are we filling whole buff*/
               && ((0xffff & (long) (sblen - nsbuf)) < (long) bufsize))
                                                /*Is buffer bigger than     */
            {                                   /*   remainder of file?     */
                                                /* If so, decrease count    */
                bufsize = (sblen - nsbuf) & SECMSK;
                multsect(bufsize / SECSIZE);
            }

            _setdma(&sbase[nsbuf]);             /* Say where data goes      */
            extsave = source.flfcb.extent;      /* Save current extent field*/

            if (diskrd(&source) == 0)           /* Read.  Was there an error*/
                nsbuf += bufsize;               /* No: tally bytes read     */
            else
            {                                   /* Error:                   */
                if (dcnt != 1)                  /* End of file error?       */
                    error(0, exten, FALSE, &source);/* Something else: abort*/

                                                /* End of file: clean up    */
                if (HAS_SETMSC(ver) && fastcopy)/* Add no. sectors copied   */
                    nsbuf += (IS_MPM(ver)) ?
                             (exten >> NIBBLE) * SECSIZE : exten;

                                                /****************************/
                                                /*                          */
                                                /* Following corrects for   */
                                                /* bug in BDOS by zeroing   */
                                                /* current record if it has */
                                                /* a new value of 0x80      */
                                                /*                          */
                                                /****************************/
                if ((source.flfcb.extent != extsave)
                   && (CURREC(&source.flfcb) == 0x80))
                    CURREC(&source.flfcb) = 0;

                _set_rand(&source);             /* Get next record number   */
                                                /* Is this sparse file (less*/
                                                /*   data than virtual size */
                                                /*   suggests is in file)?  */
                if (insparc = (ranrec(&source.flfcb) != filsize))
                                                /* Yes.  May not be allowed */
                    {if (concat || (! fastcopy))
                        error(18, 0, FALSE, &source);
                } else                          /* End of non-sparse file   */
                    close(&source);             /* Flag the fact            */
                endofsrc = TRUE;
                sbase[nsbuf] = ENDFILE;         /* Plug in end of file mark */
                return;                         /* That's all, folks!       */
            } ;                                 /* End of error handling    */
        }                                       /* End of read loop         */
}

/****************************************************************************/
/*                                                                          */
/*             N O N - F I L E   D A T A   H A N D L I N G                  */
/*             -------------------------------------------                  */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*         P U T D C H          */
                /*                              */
                /********************************/

VOID                                            /* Write the character b to */
putdch(b)                                       /*   device given by        */
register int    b;                              /*   odest.type             */
{
        if ((b >= ' ')                          /* Tally non-control chars  */
           && (++column > DELET) && (DELET > 0))/*   in column count.  Take */
                return;                         /*   no further action if   */
                                                /*   beyond right margin    */

        if (ECHO)                               /* Need to echo?            */
                _conout(b);                     /* Yes                      */

        switch (odest.type)                     /* Send char to right place */
        {
          case OUTT:                            /* OUT: device not supported*/
                nonfile_error(23);
                break;

          case PRNT:                            /* PRN: & LST: same action  */
          case LSTT:                            /*   (tabs expanded for     */
                _lstout(b);                     /*    PRN: by putdstc())    */
                break;

          case AXOT:                            /* Auxilliary (punch) output*/
                if (! IS_MPM(ver))
                {
                        _punch(b);
                }
                else                            /* MP/M does not support    */
                        nonfile_error(3);
                break;

          case FILE:                            /* Output is a file         */
                if (ndest >= dblen)             /* If buffer full, write out*/
                        writedest();
                dbase[ndest++] = b;             /* Add char to buffer       */
                break;

          case CONS:                            /* Console output           */
                _conout(b);
                break;
        }
}


                /********************************/
                /*                              */
                /*        P U T D S T C         */
                /*                              */
                /********************************/

VOID                                            /* Write out destination    */
putdstc(b)                                      /*   character, expanding   */
char    b;                                      /*   tabs                   */
{
        register int    i;

        if ((b != TAB) || (TABS == 0))          /* Not a tab or not expan-  */
                putdch(b);                      /*   ding: just output      */
        else                                    /* Expand a tab             */
        {
                i = TABS - (column % TABS);     /* How many spaces needed?  */
                while (i--)
                        putdch(' ');
        }
        if (b == CR)                            /* Zero column on CR        */
                column = 0;
}


                /********************************/
                /*                              */
                /*        N E W L I N E         */
                /*                              */
                /********************************/

VOID
newline()                                       /* Output a new line number */
{                                               /*   on a new line (six     */
        register LONG   factor, number;         /*   digits are printed,    */
        register int    digit;                  /*   leading 0's suppressed)*/
        BOOLEAN         zeroprint;

        factor = 1000000;
        number = ++line_no % factor;            /* Truncate to 6 digits     */
        zeroprint = (NUMB != 1);                /* NUMB always nonzero here */
        while (factor /= 10)                    /* Print 6 digit positions  */
        {
                digit = number / factor;        /* Get digit for this place */
                zeroprint |= ((digit != 0)      /* Suppresed leading zero?  */
                             || (factor == 1));
                _conout((zeroprint) ?           /* Print digit or space     */
                        digit + '0' : ' ');
                number %= factor;               /* Discard current place    */
        }
        printx((NUMB == 1) ? ": " : "\t");      /* NUMB value selects       */
}                                               /*   separator              */


                /********************************/
                /*                              */
                /*        P U T D E S T         */
                /*                              */
                /********************************/

VOID                                            /* Write a destination      */
putdest(b)                                      /*   character, checking    */
int     b;                                      /*   tabs and newlines      */
{
        if (FORMF && (b == FF))                 /* Ignore form feeds if cmd */
            return;                             /*   option tells us to     */

        if (putnum)                             /* New/first line of file   */
        {
            if (b == FF)                        /* Form feed?               */
                page_line = 0;                  /* Start a new page         */
            else                                /* Not a form feed          */
            {
                if (PAGCNT != 0)                /* Page eject selected?     */
                {                               /* Yes: end of current page?*/
                    if (++page_line >=
                        ((PAGCNT == 1) ? LPP : PAGCNT))
                    {                           /* Yes: behave as if form   */
                        page_line = 0;          /*   feed received          */
                        putdstc(FF);
                    }
                }
            }
            if (NUMB)                           /* Printing line numbers?   */
                newline();                      /* Yes                      */
            putnum = FALSE;                     /* New line processing done */
        }
        putdstc(b);                             /* Now we can put the char  */
        putnum = (b == LF);                     /* Next to go on new line?  */
}


                /********************************/
                /*                              */
                /*          U T R A N           */
                /*          L T R A N           */
                /*                              */
                /********************************/

int                                             /* Translate character to   */
utran(b)                                        /*   upper case if it is    */
int     b;                                      /*   lower case             */
{
        return ((('a' <= b) && (b <= 'z')) ? b & 0x5f : b);
}

int                                             /* Translate character to   */
ltran(b)                                        /*   lower case if it is    */
int     b;                                      /*   upper case             */
{
        return ((('A' <= b) && (b <= 'Z')) ? b | 0x20 : b);
}


                /********************************/
                /*                              */
                /*        G E T S R C C         */
                /*                              */
                /********************************/

int                                             /* Gets a character from the*/
getsrcc()                                       /*   current source device  */
{
        int     b;
        BOOLEAN conchk;

        conchk = TRUE;                          /* Console status chk below */
        switch (source.type)                    /* Get data from current src*/
        {
          case OUTT:                            /* These devices are not    */
          case PRNT:                            /*   valid sources          */
          case LSTT:
          case AXOT:
                nonfile_error(4);
                break;

          case FILE:                            /* Source is a file         */
                if (nsource >= sblen)           /* Is source buffer empty?  */
                {
                        if (dblbuf || (! dfile))/* If source & dest buffers */
                                                /*   separate, or dest not a*/
                                nsbuf = 0;      /*   file, just clear index */
                        else                    /* Source buff is dest buff:*/
                        {                       /* Empty it unless read from*/
                                                /*   first read from source */
                                if (nsource != FORCE_READ)
                                        writedest();   /* XXX - no parens?? */
                                        nsbuf = ndest; /* XXX - this a bug? */
                        }
                        fillsource();           /* Fill source buffer       */
                }
                b = sbase[nsource++];           /* Get a character          */
                break;

          case CONS:                            /* Source is console        */
                conchk = FALSE;                 /* Don't chk console status */
                b = _conin();
                break;

          case AXIT:                            /* Auxiliary input          */
                if (IS_MPM(ver))                /* MP/M doesn't have one    */
                        nonfile_error(4);
                b = _reader() & 0x7f;           /* Not MP/M: get character  */
                break;

          case INPT:                            /* User-defined input: not  */
                nonfile_error(23);              /*   supported              */
        }

        if (conchk)                             /* Check if user wants to   */
        {                                       /*   end transfer           */
                                                /* On object files, check   */
                                                /*   every 256 bytes, on    */
                                                /*   ASCII every line       */
                conchk = (OBJ) ? ((++concnt & 0xff) == 0) : (b == LF);

                if (conchk && _constat())       /* Did user enter character */
                {                               /* If end of file, pretend  */
                        if (_conin() == ENDFILE)/*   it came from source    */
                                return (ENDFILE);
                        nonfile_error(5);       /* Not ENDFILE: user abort  */
                }
        }

        if (ZEROP)                              /* Clear parity bit if      */
                b &= 0x7f;                      /*   necessary              */
        if (UPPER)                              /* Return character, trans- */
                return (utran(b));              /*   lated to upper or lower*/
        return ((LOWER) ? ltran(b) : b);        /*   case if requested      */
}


                /********************************/
                /*                              */
                /*           M A T C H          */
                /*                              */
                /********************************/

BOOLEAN                                         /* Match start, quit strings*/
match(b, ch)                                    /*   (used in transferring  */
int     b;                                      /*   that part of a file    */
int     ch;                                     /*   between delimiter pair)*/
{                                               /* b indexes start of match */
        int     c;                              /*   string in command buff */
                                                /* ch is current char in    */
                                                /*  file, not cmd line.     */
                                                /* Have we reached end of   */
                                                /*   match string?          */

        if ((c = combuf.comline[b += matchlen]) == ENDFILE
            || c == ']' || c == CR )            /* Allow ] or CR to end str */
        {                                       /* Yes: success!  Save char */
                combuf.comline[b] = ch;         /*   for output following   */
                return (TRUE);                  /*   match string, return   */
        }

        if (c == ch)                            /* OK so far?               */
                matchlen++;
        else
                matchlen = 0;

        return (FALSE);                         /* No cigar (so far)        */
}


                /********************************/
                /*                              */
                /*         G E T S R C          */
                /*                              */
                /********************************/

char                                            /* Get next source character*/
getsrc()                                        /* May be from source device*/
{                                               /*   or from start match    */
        int     c;                              /*   string.  Skips chars   */
                                                /*   before start match or  */
                                                /*   after end match        */

        if (quitlen > 0)                        /* This has the effect of   */
                                                /*   generating CR, LF,     */
                                                /*   ENDFILE after end match*/
            return (--quitlen == 1) ? LF : ENDFILE;/* (see below)           */

        FOREVER                                 /* Until start string found */
        {                                       /*   (if user specified one)*/
            if (feedlen > 0) {                  /* Start string just found: */
                                                /*   return character from  */
                feedlen--;                      /*   matching sequence      */
                return (combuf.comline[feedbase++]);
            }
            if ((c = getsrcc()) == ENDFILE)     /* If source char is ENDFILE*/
                return (ENDFILE);               /*   look no further        */

            if (STARTS > 0)                     /* Looking for start string?*/
            {
                if (match(STARTS,c))            /* Complete match?          */
                {                               /* Yes: we're in business!  */
                    feedbase = STARTS;          /* Arrange for output of    */
                    feedlen = matchlen + 1;     /*   matching sequence      */
                    STARTS = matchlen = 0;      /* Show start found and not */
                                                /*   searching for start    */
                }
                continue;                       /* Back to FOREVER          */
            }
                                                /* No interest in start     */

            if ((QUITS > 0)                     /* Looking for quit string? */
               && match(QUITS,c))               /* Complete match?          */
            {                                   /* Yes: arrange for CR, LF  */
                QUITS = 0;                      /*   ENDFILE to be returned */
                quitlen = 2;                    /*   by this and next two   */
                return (CR);                    /*   calls                  */
            }
            else                                /* Character of no special  */
                return (c);                     /*   interest: deliver it   */
        }                                       /* End of FOREVER           */
}


                /********************************/
                /*                              */
                /*         R D _ E O F          */
                /*                              */
                /********************************/

BOOLEAN                                         /* Get a character, return  */
rd_eof()                                        /*   TRUE if end of file    */
{
        ch = getsrc();                          /* On an object file, last  */
        if (OBJ)                                /*   byte in last bufferful?*/
                return (endofsrc && (nsource > nsbuf));

        return (ch == ENDFILE);                 /* On ASCII, is char ENDFILE*/
}

/****************************************************************************/
/*                                                                          */
/*     F U N C T I O N S   H A N D L I N G   I N T E L   H E X   D A T A    */
/*     -----------------------------------------------------------------    */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*          C K H E X           */
                /*                              */
                /********************************/

int                                             /* Check that a is a valid  */
ckhex(a)                                        /*   hex digit.  If it is   */
int a;                                          /*   return corresponding   */
{                                               /*   4-bit value            */
        if (('0' <= a) && (a <= '9'))
                return (a - '0');
        if (('A' <= a) && (a <= 'F'))
                return (a - 'A' + 0xa);
        error(13, 0, FALSE, &source);           /* Invalid hex digit: abort */
}


                /********************************/
                /*                              */
                /*      H E X R E C O R D       */
                /*                              */
                /********************************/

VOID                                            /* Transfer a whole file in */
hexrecord()                                     /*   Intel hex format,      */
{                                               /*   validating  checksums  */
        BOOLEAN zerorec, inrec;
        UWORD   length, count;
        BYTE    byte;
        int     chr, lastchr;
        UWORD   checksum;

        zerorec = inrec = FALSE;                /* Not last record in file, */
                                                /*   not inside record      */

        while ((chr = getsrc()) != ENDFILE)     /* Transfer data until end  */
        {                                       /*   of file reached        */
            if (! inrec)                        /* Perform no checks on data*/
            {                                   /*   outside records        */
                if (inrec = (chr == ':'))       /*Record start?     */
                    checksum = count = 0;
                else                            /* Not record start: copy to*/
                    putdest(chr);               /*   dest                   */
                continue;                       /* Back to while...         */
            }
                                                /* Inside a record          */

            if (++count & 1)                    /* Odd numbered chars are   */
            {                                   /*   high nibble of byte.   */
                lastchr = chr;                  /* Save for next time around*/
                continue;                       /* Go get next character    */
            }
                                                /* Even numbered char:      */
                                                /*   assemble a byte        */
            byte = (ckhex(lastchr) << 4) + ckhex(chr);
            checksum += byte;                   /* Tally in checksum        */

            if (count == 2)                     /* Record length            */
            {
                length = 10 + (byte * 2);       /* Calculate character count*/

                if (! ((zerorec = (byte == 0))  /* If this is not an ignored*/
                      && IGNOR))                /*   final record, transfer */
                {                               /*   byte count to dest     */
                    putdest(':');
                    putdest(lastchr);
                }
            }
            else                                /* Checksum at record end?  */
            {
                if (count == length)            /* Yes: low 8 bits of our   */
                {                               /*   checksum should be zero*/
                    if ((checksum & 0xff) != 0) /* Error if not             */
                        error(9, NONE, FALSE, &source);
                    else
                        inrec = FALSE;          /* No longer in record      */
                }
            }

            if (! (zerorec && IGNOR))           /* Unless an ignored final  */
                putdest(chr);                   /*   record has been found, */
        }                                       /*   transfer character     */

        if (inrec || (! zerorec))               /* End of file.  Expected?  */
            error(15, NONE, FALSE, &source);    /* Error if not             */
}

/****************************************************************************/
/*                                                                          */
/*        S T A R T ,   E N D   O F   F I L E   F U N C T I O N S           */
/*        -------------------------------------------------------           */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*     C K _ S T R I N G S      */
                /*                              */
                /********************************/

VOID                                            /* Complain if start or end */
ck_strings()                                    /*   string not found in    */
{                                               /*   source data            */
        if (STARTS > 0)                         /* Looking for start, but   */
                nonfile_error(11);              /*   didn't find it?        */
        if (QUITS > 0)                          /* Similarly for end...     */
                nonfile_error(12);
}


                /********************************/
                /*                              */
                /*      C L O S E D E S T       */
                /*                              */
                /********************************/

VOID                                            /* Close the destination    */
closedest()                                     /*   file, flushing buffer  */
{                                               /*   & renaming scratch file*/
        UBYTE   lrbc;                           /* CP/M-386: exact length of*/
                                                /*   the copy               */
        ck_strings();                           /* Started, ended OK?       */

                                                /* A straight single-file    */
                                                /*   transfer is the source  */
                                                /*   byte for byte, so it    */
                                                /*   keeps the source's exact*/
                                                /*   length.  Anything else -*/
                                                /*   concatenation, a device,*/
                                                /*   a translating option -  */
                                                /*   is PIP's own output, so */
                                                /*   measure what was written*/
        lrbc = (fastcopy && ! concat && sfile)
                 ? srclrbc : (UBYTE) (ndest % SECSIZE);

        while ((ndest % SECSIZE) != 0)          /* Fill to end of final     */
            putdest(ENDFILE);                   /*   sector with ENDFILE    */
        writedest();                            /* Flush buffer             */

        setuser(odest.user);                    /* Set up destination user  */
        close(&dest);                           /* Close scratch file       */
        if (dcnt == 255)
            error(14, exten, TRUE, &dest);      /* Can't close file!        */

        if (! HAS_XFCBS(ver)                    /* If we have not done so   */
           && ((odcnt = open(&odest)) != 255))  /*   already, find out if   */
            close(&odest);                      /*   true dest file exists  */

        if (odcnt != 255)                       /* File exists              */
        {
            if ((odest.flfcb.R_O & 0x80)        /* Is it read only and if so*/
               && (! WRROF))                    /*   do we want to know?    */
            {                                   /* Yes: pass buck to user   */
                while ((dcnt != 'Y') && (dcnt != 'N'))
                {
                    print ("DESTINATION IS R/O, DELETE (Y/N)?");
                    dcnt = utran((int) _conin());
                }
                if (dcnt == 'N')                /* User doesn't want file   */
                {                               /*   deleted                */
                     print("**NOT DELETED**");  /* Confirm wise decision    */
                     crlf();
                     delete(&dest);             /* Delete temp file and exit*/
                     return;
                }
                crlf();
            }

            odest.flfcb.R_O &= 0x7f;            /* OK to delete old dest    */
            odest.flfcb.SYSTEM &= 0x7f;         /*   file: clear attributes */
            setatt(&odest);                     /*   so it can be deleted   */
            delete(&odest);
        }
                                                /* Rename the temporary dest*/
                                                /*   file (replace $$$ type)*/
        move((char *) &odest.flfcb, dest.flfcb.resvd, 16);
        rename(&dest);
                                                /* Set dest attributes same */
                                                /*   as source              */

        odest.flfcb.F1 = (odest.flfcb.F1 & 0x7f) | f1;
        odest.flfcb.F2 = (odest.flfcb.F2 & 0x7f) | f2;
        odest.flfcb.F3 = (odest.flfcb.F3 & 0x7f) | f3;
        odest.flfcb.F4 = (odest.flfcb.F4 & 0x7f) | f4;
        odest.flfcb.USER_0 &=  0x7f;
        odest.flfcb.R_O = (odest.flfcb.R_O & 0x7f) | ro;
        odest.flfcb.SYSTEM = (odest.flfcb.SYSTEM & 0x7f) | sys;
        odest.flfcb.ARC &= 0x7f;
        setatt(&odest);
                                                /* CP/M-386: and give it the*/
                                                /*   right exact length     */
        set_lrbc((int) odest.flfcb.drive, (BYTE *) odest.flfcb.fname, lrbc);

        if (ARCHIV)                             /* Should we archive source */
        {                                       /* Yes:                     */
                setuser(source.user);
                source.flfcb.ftype[2] |= 0x80;
                source.flfcb.extent = 0;
                setatt(&source);
        }
}


                /********************************/
                /*                              */
                /*    S I Z E _ M E M O R Y     */
                /*                              */
                /********************************/

VOID                                            /* Allocate space for the  */
size_memory()                                   /*   source and destination*/
{                                               /*   buffers               */
        static UWORD    allocated = 0;
        static BYTE     *base;

        if (! allocated)                        /* First time through, grab*/
        {                                       /*   as much memory as we  */
                base = sbrk(0);                 /*   can (even multiple of */
                                                /*   SECSIZE)              */
                                                /* CP/M-386: the gap up to */
                                                /*   the stack is measured */
                                                /*   in megabytes here, so */
                                                /*   ask the shim for what */
                                                /*   a UWORD can index     */
                allocated = (_membytes() - SAFETY) & (SECMSK << 1);
                if (sbrk(allocated) == (BYTE *) -1)
                {
                        print("Can't allocate memory");
                        _exit(1);
                }
        }

        if (dblbuf)                             /* Are two buffers needed?  */
        {                                       /* Yes: divide the available*/
                sblen = dblen = allocated / 2;  /*   space equally          */
                dbase = (char *) base;
                sbase = (char *) base + dblen;
                if (ndest >= dblen) writedest();
                nsbuf = 0;
        }
        else                                    /* Just one buffer          */
        {
                sblen = dblen = allocated;
                sbase = dbase = (char *) base;
        }
}


                /********************************/
                /*                              */
                /*       S E T U P E O B        */
                /*                              */
                /********************************/

VOID                                            /* Finds the first ENDFILE  */
setupeob()                                      /*   in the last sector of  */
{                                               /*   the source file just   */
        register int    i;                      /*   read and adjusts nsbuf */
                                                /*   to index the character */

        if (OBJ)                                /* No-op for object files   */
                return;

        nsbuf -= SECSIZE + 1;                   /* Index char before last   */
        for (i = 0; i < SECSIZE; i++)           /*   sector and examine a   */
                if (sbase[++nsbuf] == ENDFILE)  /*   sector's worth of data */
                        return;
        nsbuf++;                                /* Point beyond last sector */
}


                /********************************/
                /*                              */
                /*       C H K R A N D O M      */
                /*                              */
                /********************************/

VOID                                            /* Find and read the next   */
chkrandom()                                     /*   record containing data */
{                                               /*   in a sparse file (file */
        BYTE    current;                        /*   where not all records  */
                                                /*   contain data)          */

        setuser(source.user);                   /* Set up source user       */
        _set_rand(&source);                     /* Set up record number     */
        multsect(1);                            /* Only read one record     */
        _setdma(buff);                          /*   into default buffer    */

        FOREVER                                 /* Until record containing  */
        {                                       /*   data found             */
            switch ((int) _b_read(&source))     /* Data in this record?     */
            {                                   /* Yes: copy record no to   */
              case 0:                           /*   destination FCB (do not*/
                                                /*   disturb current record)*/
                current = ((BYTE *) &dest.flfcb.record)[0];
                dest.flfcb.record = source.flfcb.record;
                ((BYTE *) &dest.flfcb.record)[0] = current;
                endofsrc = FALSE;
                return;                         /* Mission accomplished     */

              case 1:                           /* No data in this record:  */
                                                /*   go to the next         */
                setranrec(&source.flfcb, ranrec(&source.flfcb) + 1);
                break;

              case 4:                           /* No data in this extent!  */
                                                /* Move to the next         */
                setranrec(&source.flfcb,
                          (ranrec(&source.flfcb) + EXTRECS) & EXTMSK);
                break;

              default:                          /* Something went wrong: say*/
                error(0, NONE, FALSE, &source); /*   its a disk read error  */
            }
        }
}

/****************************************************************************/
/*                                                                          */
/*               D A T A   C O P Y I N G   F U N C T I O N S                */
/*               -------------------------------------------                */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*     S I M P L E C O P Y      */
                /*                              */
                /********************************/

VOID                                            /* Copy a single input to  */
simplecopy()                                    /*   an output (concaten-  */
{                                               /*   ation handled in      */
        register int    i;                      /*   _main function!)      */

        fastcopy = (sfile && dfile);            /* Is this file-file copy? */
        endofsrc = dblbuf = sparfil = FALSE;    /* Set initial conditions  */

        for (i = 0; i < sizeof cont; i++)       /* Check options which     */
        {                                       /*   affect copy method    */
            if (cont[i]) {
                switch(optype[i])
                    {
                      case CHRT:                /* Character by character  */
                        fastcopy = FALSE;
                        break;

                       case DUBL:               /* May need double buffer  */
                        dblbuf = (sfile && dfile);
                        fastcopy = FALSE;
                        break;
                }
            }
        }

        size_memory();                          /* Set up buffers           */
        if (sfile)                              /* Open source if it is a   */
            setupsource();                      /*   file                   */

                                                /* Now ready to do copy     */
        if (fastcopy)                           /* Quick file-file copy?    */
        {
            while (! endofsrc)                  /* Read the whole source    */
            {
                fillsource();                   /* Fill source buffer       */
                if (endofsrc && concat)         /* End of source file?      */
                {
                    setupeob();                 /* Find true end of file    */
                    ndest = nsbuf;
                    if (nendcmd)                /* More to concatenate?     */
                        return;                 /* Yes: don't flush buffer  */
                }
                ndest = nsbuf;
                writedest();                    /* Write buffer out to      */
                nsbuf = ndest;                  /*   scratch file           */
                if (endofsrc && insparc)        /* If this has turned out to*/
                    chkrandom();                /*   be sparse file, move to*/
            }                                   /*   next record containing */
        }                                       /*   data                   */
        else                                    /* Character by character   */
        {
            if (HEXT || IGNOR)                  /* Transfer Intel hex data  */
                hexrecord();
            else
                while (! rd_eof())              /* Copy characters between  */
                    putdest(ch);                /*   start & end strings    */

            if (concat && nendcmd)              /* End of file: more to come*/
            {                                   /* Yes: mark buffer, but    */
                nsbuf = ndest;                  /*   don't wrap up yet      */
                return;
            }
        }

                                                /* All done:                */
        if (dfile)                              /* If destination is a file,*/
            closedest();                        /*   close it               */
}


                /********************************/
                /*                              */
                /*          A R C H K           */
                /*                              */
                /********************************/

BOOLEAN                                         /* Check if archive bit is  */
archck()                                        /*   clear in any extent of */
{                                               /*   source file (used by   */
                                                /*   [A] option in deciding */
                                                /*   which files to copy)   */
                                                /* Returns TRUE if file NOT */
                                                /*   archived               */
        if (! ARCHIV)                           /* Only check if option     */
                return (TRUE);                  /*   selected               */

        setuser(source.user);                   /* Set up source user       */
        source.flfcb.extent = WILD;             /* Look for all extents of  */
        search(&source);                        /*   the source file        */

        while (dcnt != 255)                     /* While more extents       */
        {                                       /* Copy name from directory */
                move(&buff[((dcnt & 3) * 32) + 1], source.flfcb.fname, 15);

                if (! (source.flfcb.ARC & 0x80))/* Is archive bit clear?    */
                        return (TRUE);
                searchn();                      /* No, it's set: try again  */
        }
        return (FALSE);                         /* All extents archived     */
}


                /********************************/
                /*                              */
                /*      N E X T _ F I L E       */
                /*                              */
                /********************************/

/****************************************************************************/
/* NOTE: This function could be made much more efficient if it were         */
/*       rewritten to use _gset_scb() to save and restore search context    */
/*       between calls if BDOS has CP/M 3.0 features                        */
/****************************************************************************/

int                                             /* Find the first extent of */
next_file(beyond)                               /*   next file after direct-*/
int beyond;                                     /*   ory entry indexed by   */
{                                               /*   beyond which matches   */
        register int    ndcnt;                  /*   the name in SEARFCB.   */
                                                /* Returns new beyond value */
        setuser(source.user);                   /* Set up source user       */
        _setdma(buff);                          /* Get directory record into*/
        SEARFCB->extent = 0;                    /*   default DMA buffer     */
        search(SEARFCB);
        ndcnt = 0;
        while ((dcnt != 255)                    /* Find the match requested */
              && (ndcnt++ < beyond))
                searchn();

        if (dcnt != 255)                        /* Copy name from directory */
        {                                       /*   to source & dest FCB's */

                move(&buff[((dcnt & 3) * 32) + 1], odest.flfcb.fname, 15);
                move(odest.flfcb.fname, source.flfcb.fname, 15);
        }

        return(ndcnt);                          /* Return index to match    */
}


                /********************************/
                /*                              */
                /*       M U L T C O P Y        */
                /*                              */
                /********************************/

VOID                                            /* Copy mutiple files to a  */
multcopy()                                      /*   a named drive or user  */
{
        int     nextdir, ncopied;

        if (! eretry)                           /* Unless retrying following*/
            nextdir = ncopied = 0;              /*   error, clear counters  */

        FOREVER                                 /* Until no more matches    */
        {
            nextdir = next_file(nextdir);       /* Find a file to copy      */

            if (dcnt == 255)                    /* No more files?           */
            {
                if (ncopied == 0)               /* No matches at all?       */
                    error(10, 0, FALSE, SEARFCB);/* File not found          */

                if (! KILDS)                    /* Terminate list of files  */
                    crlf();

                return;                         /* All done                 */
            }

                                                /* We have a filename match */
            if (! archck())                     /* Ignore it if archived and*/
                continue;                       /*   A option in force      */

            if ((odest.flfcb.SYSTEM & 0x80)     /* Ignore if a system file  */
               && ! RSYS)                       /*   and not allowed to copy*/
                continue;

            made = FALSE;                       /* Destination file not made*/
                                                /* Start from first extent  */
            odest.flfcb.extent = source.flfcb.extent = 0;

            if (! KILDS)                        /* Need to print its name?  */
            {
                if (ncopied == 0)               /* Print heading first time */
                    print("COPYING -");

                crlf();
                prname(&odest.flfcb);
            }

            ncopied++;                          /* We have a file to copy   */
            simplecopy();                       /* Do the copy              */
        }
}

/****************************************************************************/
/*                                                                          */
/*             C O M M A N D   P A R S I N G   F U N C T I O N S            */
/*             -------------------------------------------------            */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*        C K _ D I S K         */
                /*                              */
                /********************************/

ck_disk()                                       /* Abort if source and dest */
{                                               /*   disks, source and dest */
        if ((odest.user == source.user)         /*   users the same         */
           && (odest.flfcb.drive == source.flfcb.drive))
                nonfile_error(8);
}


                /********************************/
                /*                              */
                /*            G N C             */
                /*                              */
                /********************************/

int                                             /* Get next character from  */
gnc()                                           /*   command line or CR at  */
{                                               /*   end of line            */
                                                /* Also translates returned */
                                                /*   value to upper case.   */
        return ((++combuf.cbp >= (combuf.comlen & 0xff)) ?
                CR : (utran(combuf.comline[combuf.cbp])));
}


                /********************************/
                /*                              */
                /*         D E B L A N K        */
                /*                              */
                /********************************/

VOID                                            /* Skip until non-blank     */
deblank()                                       /*   command line character */
{
        if (ch == CR) return;
        while (getch(ch) == ' ') ;
}


                /********************************/
                /*                              */
                /*         C K _ E O L          */
                /*                              */
                /********************************/

VOID                                            /* Complain if further input*/
ck_eol()                                        /*   follows a complete cmd */
{
        deblank();
        if (ch != CR)
                nonfile_error(8);
}


                /********************************/
                /*                              */
                /*       D E L I M I T E R      */
                /*                              */
                /********************************/

BOOLEAN                                         /* Return TRUE if the curent*/
delimiter(c)                                    /*   character is delimiter */
int     c;
{
        static char     del[] = "* =.:;,!=\r<[]";
        register int    i;

        for (i = 0; i < sizeof del - 1; i++)
                if (c == del[i])
                        return (TRUE);
        return (FALSE);
}


                /********************************/
                /*                              */
                /*         A _ T O _ I          */
                /*                              */
                /********************************/

UWORD                                           /* Convert ASCII to decimal */
a_to_i()                                        /* Returns result           */
{
        register UWORD  decimal;

        decimal = 0;
        while (('0' <= getch(ch))               /* Until non-digit found    */
              && ((ch) <= '9'))
                decimal = 10 * decimal + (ch - '0');

        combuf.cbp--;                           /* "Push back" terminator   */
        return (decimal);
}


                /********************************/
                /*                              */
                /*       S C A N _ P A R        */
                /*                              */
                /********************************/

VOID                                            /* Scan option parameters   */
scanpar(fcba)                                   /*   enclosed in square     */
struct flctlb *fcba;                            /*   brackets               */
{
        register int    usr;
        register int    n;                      /* Int returned by a_to_i() */
        char            chsave;                 /* Because of C side-effect */
                                                /* crock - see a_to_i call. */
        while ((getch(ch) != CR)
              && (ch != ']'))
        {
            if ((ch = utran(ch))  == ' ')       /* Skip spaces between      */
                continue;                       /*   options                */

            if (!(('A' <= ch) && (ch <= 'Z')))  /* Non-alpha option?        */
                nonfile_error(6);               /* Yes: error               */

            cont[ch - 'A'] = 1;                 /* Show option selected     */

            switch (chsave = ch)                /* Check for modifiers after*/
            {                                   /*   option                 */
              default:                          /* D, N, P, T take an intgr */
                if( (n = a_to_i()) != 0)        /*  arg (optional w/ N.)    */
                    cont[chsave - 'A'] = n;     /* Plug in value (note lack */
                break;                          /*   of range check)        */
                                                /* NOTE: a_to_i() steps on  */
                                                /* the global variable "ch",*/
                                                /* and the subscript is eval*/
                                                /* uated after the call.    */
              case 'G':                         /* Get user number          */
                if ((usr = a_to_i()) > MAXUSER) /* Complain if invalid user */
                    nonfile_error(6);

                fcba->user = usr;               /* Plug user no. into FCB   */
                break;

              case 'Q':                         /* Start or quit string:    */
              case 'S':                         /* Put string index in      */
                cont[ch - 'A'] = combuf.cbp + 1;/*   option buffer          */

                while ((n = gnc()) != ENDFILE
                        && n != ']' && n != CR);/* Skip string              */
                break;
            }
        }
}


                /********************************/
                /*                              */
                /*           T O K E N          */
                /*                              */
                /********************************/

int                                             /* Finds the length of the  */
token()                                         /*   next command token,    */
{                                               /*   that is the number of  */
        register int    len;                    /*   chars, including the   */
                                                /*   one indexed by cbp on  */
        for (len = 0; ! delimiter(ch); len++)   /*   entry, up to but not   */
              getch(ch);                        /*   including the next     */
                                                /*   delimiter.  ch contains*/
        return (len);                           /*   and cbp indexes, the   */
}                                               /*   delimiter on exit      */



                /********************************/
                /*                              */
                /*       G E T _ D E V          */
                /*                              */
                /********************************/

int                                             /* If the current token     */
get_dev()                                       /*   matches a valid device,*/
{                                               /*   return index in io to  */
        register int    i, j;                   /*   device name.  If no    */
                                                /*   match, return invalid  */
                                                /*   index                  */
        for (i = 0; i < TYPES; i++)             /* For each valid name      */
        {
            for ( j = 0; j < 3; j++)            /* Try to match 3 characters*/
            {
                if (io[i] [j] != utran(combuf.comline[combuf.cbp - 3 + j]))
                    break;                      /* Not this device.  Next?  */

                if (j == 2)                     /* Complete match?          */
                      return (i);               /* Yes                      */
            }
        }
        return (TYPES);                         /* No match                 */
}


                /********************************/
                /*                              */
                /*           S C A N            */
                /*                              */
                /********************************/

VOID                                            /* Scan a file/device name  */
scan(fcba)                                      /* If file name, put in fcba*/
struct flctlb *fcba;                            /* Set up any options       */
{
        register UWORD  i, length;

        *fcba = empty_fcb;                      /* Clear out FCB            */
        fcba->flfcb.drive = cdisk + 1;          /* Initialize to current    */
        fcba->user = c_user;                    /*   disk and current user  */
        fcba->type = FILE;
                                                /* Clear out options        */
        for (i = 0; i < sizeof cont; cont[i++] = 0);

        feedlen = matchlen = quitlen = 0;       /* Initialize string match  */
                                                /*   control variables      */
        ambig = FALSE;                          /* Filename not ambiguous   */

        deblank();                              /* Skip to nonblank char    */

                                                /* Initialization done:     */
                                                /*   parse                  */
        length = token();                       /* Find a token             */

        if (ch == ':')                          /* Did it end with ':'?     */
        {                                       /* Yes.  How long was it?   */
            switch (length)
            {
              case 1:                           /* Looks like a drive name  */
                                                /* Put in FCB, go on if OK  */

                if (! ('A' <= (ch = utran(combuf.comline[combuf.cbp - 1]))
                   && ((fcba->flfcb.drive = ch - 'A' + 1) <= MAXDRIVE)))
                    goto parse_error;           /* Not valid disk name      */

                deblank();                      /* Blanks may follow drive  */

                if((length = token()) == 0)     /* Get another token.  If   */
                                                /*   it's null, and not     */
                    if (ch != '*') {            /*   an asterisk, device is */
                        fcba->type = DISKNAME;  /*   disk name only         */
                                                /* Maybe options follow??   */
                        if (ch == '[') {        /* Yes: collect options     */
                            scanpar(fcba);
                            if (ch != ']'
                                && ch != CR)    /* Make sure the options    */
                                                /*  End? If Not format error*/
                              nonfile_error(6);  /* skip error? */
                            return;             /*  Return w/ ch == ']'     */
                        }
                        combuf.cbp--;           /* Step back to last char   */
                        return;                 /* All done                 */
                    }
                break;                          /* Non null token: filename */

              case 3:                           /* Looks like device name   */
                                                /* Matches known name?      */
                if ((fcba->type = get_dev()) >= TYPES)
                    goto parse_error;           /* No: invalid device       */

                if (gnc() == '[') {             /* Yes: collect options     */
                    scanpar(fcba);
                    if (ch != ']'
                        && ch != CR)            /* Make sure the options    */
                        nonfile_error(6);       /*   End if Not format error*/
                    else deblank();
                }
                combuf.cbp--;                   /* Step back to last char   */
                return;                         /* We're done               */

              default:
                goto parse_error;               /* Unidentified token       */
            }
        }                                       /* Past the colon (if any): */
                                                /*   should be positioned at*/
                                                /*   filename               */

        if (length > L_NAME)                    /* Too long for             */
            goto parse_error;                   /*   filename?              */

        if ((ch == '*') && (length == 0))       /* Wildcard in filename?    */
        {                                       /* Yes: fill with ?'s       */
            move("????????", fcba->flfcb.fname, L_NAME);
            getch(ch);
            token();                            /* Skip to next delimiter   */
            ambig = TRUE;                       /* Name is ambiguous        */
        }
        else                                    /* Not wildcard:            */
        {                                       /*   copy to FCB            */
            for( i = combuf.cbp - length; i < combuf.cbp; i++)
                combuf.comline[i] =             /* Translate filename to    */
                    utran(combuf.comline[i]);   /*   upper case           */
            move(&combuf.comline[combuf.cbp - length],
                 fcba->flfcb.fname, length);
        }

        if (ch == '.')                          /* Does a type follow name? */
        {                                       /* Yes: read it             */
            getch(ch);
            if ((length = token()) > L_TYPE)    /* Error if too long        */
                goto parse_error;

            if ((ch == '*') && (length == 0))   /* Wildcard in type?        */
            {                                   /* Yes: fill with ?'s       */
                move("???", fcba->flfcb.ftype, L_TYPE);
                getch(ch);
                token();                        /* Skip to next delimiter   */
                ambig = TRUE;                   /* Name is ambiguous        */
            }
            else {                              /* O K: copy to FCB         */
                for( i = combuf.cbp - length; i < combuf.cbp; i++)
                    combuf.comline[i] =         /* Translate filename to    */
                        utran(combuf.comline[i]);/*   upper case            */
                move(&combuf.comline[combuf.cbp - length],
                     fcba->flfcb.ftype, length);
            }
        }

        if (HAS_XFCBS(ver)                      /* Past the filename.  There*/
           && (ch == ';'))                      /*   could be a password    */
        {
            getch(ch);
            if ((length = token()) > L_PASS)    /* Error if too long        */
                goto parse_error;

            if (length != 0)                    /* O K: copy to FCB if not  */
            {                                   /*   zero length            */
                move(&combuf.comline[combuf.cbp - length],
                     fcba->pwnam, length);

                if (fcba == &odest)             /* If parsing dest name,    */
                        getpw = FALSE;          /*   show non-default passwd*/
            }                                   /*   it to be used          */
        }

                                                /* Maybe options follow??   */
        if (ch == '[') {                        /* Yes: collect options     */
            scanpar(fcba);
            if (ch != ']' && ch != CR)          /* Make sure the options    */
                nonfile_error(6);               /*  End? If Not format error*/
            else deblank();
        }
        combuf.cbp--;                           /* Align the input          */
        return;                                 /* That's all, folks        */

parse_error:
        combuf.cbp -= length;                   /* Step back to first char  */
        return;                                 /*   of bad token           */
}

/****************************************************************************/
/*                                                                          */
/*        U T I L I T Y   F U N C T I O N S   F O R   M A I N ( )           */
/*        -------------------------------------------------------           */
/*                                                                          */
/****************************************************************************/


                /********************************/
                /*                              */
                /*        G E T _ C M D         */
                /*                              */
                /********************************/

BOOLEAN                                         /* Get the next command line*/
get_cmd()                                       /* Return TRUE if not empty */
{                                               /* Also set up variables    */

        line_no = 0;                            /* Line no for printer      */
        concnt = column = 0;                    /* Abort check, printer col */
        ndest = nsbuf = 0;                      /* Src, dest buffer indices */
        made = FALSE;                           /* Destination file not made*/
        concat = FALSE;                         /* Single source file       */
        putnum = TRUE;                          /* Need line no before print*/
        dfile = sfile = TRUE;                   /* Src, dest are files      */
        nendcmd = TRUE;                         /* Command end not reached  */
        page_line = (UWORD) -2;                 /* Form feed before print   */
        combuf.cbp = (UWORD) -1;                /* Command buff index is    */
                                                /*   incremented to zero    */
        getpw = (HAS_XFCBS(ver));               /* Files may need passwords */

        if (multcom)                            /* If in interactive mode,  */
        {                                       /*   get a command line     */
                _conout('*');                   /* Prompt for it            */
                rdcom();
                crlf();
                dcnt = 0;                       /* Clear error flag         */
        }
        ch = ' ';                               /* To solve the CR at eol   */
                                                /* bug                      */
        return ((UWORD) (combuf.comlen & 0xff) > 1);
                                                /* Tell caller if cmd empty */
}


                /********************************/
                /*                              */
                /*       D S T _ D I S K        */
                /*                              */
                /********************************/

VOID                                            /* Destination is a disk:   */
dst_disk()                                      /*   copy file(s) to it     */
{
        BYTE    dd, du;

        ck_eol();                               /* Nothing can follow name  */
        ck_disk();                              /* Dest user or disk must   */
                                                /*   differ from source     */
        odest.type = FILE;                      /* Dest is really a file    */

        if (ambig)                              /* Form is X:=Y:ambiguous   */
        {
                *SEARFCB = source.flfcb;        /* Prepare for search       */
                multcopy();                     /* Copy matching files      */
        }
        else                                    /* Form is X:=Y:unique      */
        {                                       /* Copy source FCB to dest, */
                dd = odest.flfcb.drive;         /*   preserving dest drive  */
                du = odest.user;                /*   and user number        */
                odest = source;
                odest.flfcb.drive = dd;
                odest.user = du;
                simplecopy();                   /* Copy the file            */
        }
}



                /********************************/
                /*                              */
                /*   D I S K _ T O _ F I L E    */
                /*                              */
                /********************************/

VOID                                            /* Action command with form */
disk_to_file()                                  /*   unique=X:              */
{
        BYTE    sd, su;

        ck_eol();                               /* Nothing can follow name  */
        ck_disk();                              /* Dest user or disk must   */
                                                /*   differ from source     */
        source.type = FILE;                     /* Source is really a file  */

        sd = source.flfcb.drive;                /* Copy dest FCB to source, */
        su = source.user;                       /*   preserving source drive*/
        source = odest;                         /*   and user number        */
        source.flfcb.drive = sd;
        source.user = su;
        simplecopy();                           /* Copy the file            */
}


                /********************************/
                /*                              */
                /*    C O P Y _ S O U R C E     */
                /*                              */
                /********************************/

VOID                                            /* Transfer the current     */
copy_source()                                   /*   source file to the     */
{                                               /*   destination            */
        register int    i;

        if (odest.type != FILE) dfile = FALSE;  /* So we won't fastcopy     */

        if (odest.type == PRNT)                 /* If the destination is    */
        {                                       /*   formatted printer set  */
                NUMB = TRUE;                    /*   up default tabs and    */
                if (TABS == 0)                  /*   page length if options */
                        TABS = 8;               /*   have not already set   */
                if (PAGCNT == 0)                /*   them up                */
                        PAGCNT = 1;
        }

        switch (source.type)                    /* How we behave depends on */
        {                                       /*   source type            */
          case FILE:                            /* It's a file              */
                if (ambig)                      /* Name cannot be abiguous  */
                        nonfile_error(4);
                break;                          /* Name is unique: go to it!*/

          case CONS:                            /* It's valid, but not a    */
          case AXIT:                            /*   file                   */
          case INPT:
                sfile = FALSE;
                break;

          case NULT:                            /* Null trailer: generate it*/
                for (i = NULLS; i--; putdest(0));
                return;                         /* Transfer complete        */

          case EOFT:                            /* End of file mark: send it*/
                putdest(ENDFILE);
                return;                         /* Transfer complete        */

          default:                              /* None of the above: error */
                nonfile_error(4);
        }

        simplecopy();                           /* Transfer the data        */
        ck_strings();                           /* Abort if start, end      */
                                                /*   string not found       */
}

/****************************************************************************/
/*                                                                          */
/*                     M A I N   F U N C T I O N                            */
/*                     -------------------------                            */
/*                                                                          */
/* NOTE: this function is named _main, not main, so as to avoid the         */
/*       standard C prolog function (also called _main) being pulled in     */
/*       from the library at link-edit time.  The library function would    */
/*       add a considerable amount of dead wood - notably several parts     */
/*       of the standard I/O package - which are not required by PIP        */
/*                                                                          */
/****************************************************************************/


VOID
_main()
{
        BOOLEAN copydone;                       /* Needed for case          */
                                                /*  source.type == DISKNAME */
                                                /* Set up destination addr  */
        if (setjmp(main_stack) == 0)            /*   for non-local goto     */
        {                                       /* Following code not reex- */
                                                /*   ecuted after error     */
            buff = _base->buff;                 /* Set up pointers into     */
            fcb = &_base->fcb1;                 /*   basepage               */
                                                /* Get command line tail    */
            move(buff, (char *) &combuf.comlen, sizeof (combuf.comline) + 1);

            if (HAS_RETERR(ver = _version()))   /* Get CP/M version, set    */
                _ret_errors(0xff);              /*   error return mode      */

            if (multcom = (combuf.comlen == 0)) /* If cmd line tail empty,  */
                                                /*   interactive mode:      */
                {                               /*   announce ourselves     */
                print(signon);
                crlf();
            }
            c_user = last_user = _gset_ucode(0xff);
                                                /* Get current user         */
            cdisk = _ret_cdisk();               /*   and current disk       */
            eretry = FALSE;                     /* First time around can't  */
        }                                       /*   be error retry!        */

/****************************************************************************/
/* Restart here following longjmp at end of error().  Eretry shows whether  */
/*   current transfer should be retried or not (MP/M only)                  */
/****************************************************************************/

        if (eretry)
            multcopy();                         /* Retry failed transfer    */

        while (get_cmd())                       /* Until empty command      */
        {
            copydone = FALSE;                   /* Nothing done yet         */

            scan(&odest);                       /* Parse destination name   */
            if (ambig)                          /* Cannot be ambiguous      */
                error(3, NONE, FALSE, &odest);

            deblank();                          /* Should be assignment next*/
            switch(ch)                          /* Parse failures or bad    */
            {                                   /*   command line will force*/
              case '=':                         /*   error at this point    */
              case '<':
                break;                          /* Valid assignment operator*/

              default:                          /* Invalid: error           */
                nonfile_error(8);
            }

            scan(&source);                      /* Parse first source name  */

            switch (odest.type)                 /* We are now in a position */
            {                                   /*   to handle special cases*/
              case CONS:
              case OUTT:
              case AXOT:
                break;                          /* These cases not special  */

              case PRNT:                        /* Formatted list device:   */
              case LSTT:                        /* List device: if MP/M,    */
                if (IS_MPM(ver)                 /*   check device is avail- */
                   && (_cond_lst() == 0xff))    /*   able and abort if not  */
                    nonfile_error(8);
                break;

              case FILE:                        /* Form may be unique=X:    */
                if (source.type == DISKNAME)
                {                               /* Yes, it is               */
                    disk_to_file();
                    combuf.comlen = multcom;    /* Zero command length if   */
                                                /*   single command         */
                    continue;                   /* Done with current command*/
                }
                break;                          /* Not a special case       */

              case DISKNAME:                    /* Form is X: = something   */
                if (source.type != FILE)        /* Error if "something" not */
                    nonfile_error(8);           /*   a filename             */

                dst_disk();                     /* O K: go do the work      */
                copydone = TRUE;                /* So we won't do the       */
                break;                          /* copy_source() below      */

              default:                          /* Should have been one of  */
                nonfile_error(3);               /*   the above.  Error      */
            }

            while (nendcmd)                     /* While still more to do   */
            {
                deblank();                      /* Skip blanks in cmd line  */
                switch(ch)                      /* What have we found?      */
                {
                  case CR:                      /* The end of the line      */
                    nendcmd = FALSE;
                    break;

                  case ',':                     /* An indication that       */
                    concat = nendcmd = TRUE;    /*   another file follows   */
                    break;

                  default:                      /* An illegal separator...   */
                    nonfile_error(16);
                }

                if( ! copydone )                /* If not done already...   */
                    copy_source();              /* ...perform transfer      */

                if (nendcmd)                    /* If more files named,     */
                    scan(&source);              /*   go parse another       */
            }

            combuf.comlen = multcom;            /* Zero command length if   */
                                                /*   single command         */
        }                                       /* End of while (get_cmd()) */

        setuser(c_user);                        /* Restore current user     */
        _exit(0);                               /* Normal exit from PIP     */
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
