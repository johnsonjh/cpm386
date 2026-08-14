/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 00997242-8309-11f1-880f-80ee73e9b8e7
 */

/*****************************************************************************/

/* stat.c - STAT utility for CP/M-386 */

/*
 * Ported from Zilog CP/M-Z8000 STAT v1.0C 01/03/84
 *
 * Usage:
 *   STAT               - Free space on all logged drives
 *   STAT d:            - Free space on drive d:
 *   STAT DSK:          - Disk parameters (current drive)
 *   STAT d:DSK:        - Disk parameters (drive d:)
 *   STAT USR:          - Users with files on current drive
 *   STAT d:USR:        - Users with files on drive d:
 *   STAT DEV:          - Show iobyte device mapping
 *   STAT VAL:          - Show valid command forms
 *   STAT d:=RO         - Set drive read-only
 *   STAT d:=RW         - Set drive read-write
 *   STAT filespec      - File size/attributes
 *   STAT filespec SIZE - File size+logical record count
 *   STAT filespec ATTR - Set file attribute (RO/RW/SYS/DIR)
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned long ULONG;

/*****************************************************************************/

#define TRUE 1
#define FALSE 0

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

/* BDOS call */
static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret = 0;

  (void)func;
  (void)info;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func),
                      "i"(BDOS_INT),
                      "d"((ULONG)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

/* I/O primitives */
static void
conout (char c)
{
  bdos (2, (LONG)(unsigned char)c);
}

/*****************************************************************************/

static int
constat (void)
{
  return (int)bdos (11, 0);
}

/*****************************************************************************/

static int
conin (void)
{
  return (int)bdos (1, 0);
}

/*****************************************************************************/

static void
printx (const char *s)
{
  while (*s)
    {
      conout (*s++);
    }
}

/*****************************************************************************/

static void
crlf (void)
{
  conout ('\r');
  conout ('\n');

  /* abort on keypress */
  if (constat ())
    {
      conin ();
      crlf ();
      printx ("* Aborted *");
      crlf ();

      bdos (0, 0);
    }
}

/*****************************************************************************/

static void
print (const char *s) /* crlf and THEN string */
{
  crlf ();
  printx (s);
}

/*****************************************************************************/

static void
blanks (int n)
{
  while (n-- > 0)
    {
      conout (' ');
    }
}

/*****************************************************************************/

/* Integer printing */

/*
 * Print unsigned value v with precision digits (prec=1,10,100,...)
 * leading-zero suppressed if zerosup != 0.
 * XXX Migrate to BDOS 211 XXX
 */

static void
pdecimal (UWORD v, UWORD prec, int zerosup)
{
  while (prec)
    {
      int d = v / prec;
      v %= prec;

      if ((prec /= 10) != 0 && zerosup)
        {
          if (d == 0)
            {
              conout (' ');
            }
          else
            {
              zerosup = 0;
              conout ((char)('0' + d));
            }
        }
      else
        {
          conout ((char)('0' + d));
        }
    }
}

/*****************************************************************************/

/* Print ULONG with comma separation, 7-digit max */
static void
p_long (ULONG value)
{
  UWORD thous [4];
  int j, zerosup;

  zerosup = TRUE;

  for (j = 3; j >= 0; j--)
    {
      thous [j] = (UWORD)(value % 1000);
      value /= 1000;
    }

  for (j = 0; j < 4; j++)
    {
      switch (j)
        {
        case 0:

          break; /* billions: skip */

        case 1:
          if (thous [j] == 0)
            {
              printx ("  ");
            }
          else
            {
              pdecimal (thous [j], 1, zerosup);
              conout (',');
              zerosup = FALSE;
            }

          break;

        case 3:
          pdecimal (thous [j], 100, zerosup);
          zerosup = FALSE;

          break;

        default:
          if (thous [j])
            {
              pdecimal (thous [j], 100, zerosup);
              conout (',');
              zerosup = FALSE;
            }
          else
            {
              printx ("    ");
            }

          break;
        }
    }
}

/*****************************************************************************/

static void
p_unl (ULONG value)
{
  crlf ();
  p_long (value);
  printx (": ");
}

/*****************************************************************************/

/* CP/M-386 basepage constants (TPA-relative offsets) */
#define SPKSHF 3 /* log2(sectors per K) = 3 => 8 secs/K */

/*****************************************************************************/

/* Global state */
static int cdisk;          /* current disk (0=A) */
static int user_code;      /* current user number */
static UWORD rodisk;       /* read-only disk vector */
static int sizeset;        /* TRUE if SIZE requested */
static int set_attribute;  /* TRUE if setting attribute */
static int word_blks;      /* TRUE if 16-bit block addresses */
static int kpb;            /* kbytes per block */
static int scase1, scase2; /* attribute cases */
static UWORD nfcbs;        /* total FCB extent count */
static UWORD fcbn;         /* files collected so far */
static UWORD fcbmax;       /* max files we can hold */
static int error_free;     /* no duplicate block errors */
#if 0
static UWORD dcnt;         /* directory search result */
#endif

/*****************************************************************************/

/* DPB fields (from BDOS 31) */
static UBYTE dpb_raw [32];
#define DPB_BSH (dpb_raw [2])
#define DPB_EXM (dpb_raw [4])
#define DPB_DSM ((UWORD)(dpb_raw [6]  | ((UWORD)dpb_raw  [7] << 8)))
#define DPB_DRM ((UWORD)(dpb_raw [8]  | ((UWORD)dpb_raw  [9] << 8)))
#define DPB_CKS ((UWORD)(dpb_raw [12] | ((UWORD)dpb_raw [13] << 8)))
#define DPB_OFS ((UWORD)(dpb_raw [14] | ((UWORD)dpb_raw [15] << 8)))
#define DPB_SPT ((UWORD)(dpb_raw [0]  | ((UWORD)dpb_raw  [1] << 8)))

/*****************************************************************************/

static UBYTE dma [128]; /* DMA buffer */

/*****************************************************************************/

/* Token strings */
#define T_SIZE 4
static char token [T_SIZE];

/*****************************************************************************/

/* Command-line buffer pointer (basepage offset 0x80, TPA-relative) */
static UBYTE *buff;

/*****************************************************************************/

/* Basepage FCB (offset 0x5C) */
static UBYTE *bpfcb;

/*****************************************************************************/

/* File collection: up to MAX_FILES files */
#define MAX_FILES 64

/*****************************************************************************/

struct fileent
{
  char name [8];
  char typ [3];
  UBYTE ro;
  UBYTE sys;
  UBYTE arch;
  UBYTE extent_count; /* FCB extent counter */
  UWORD records;      /* sum of rc fields   */
  UWORD kcnt;         /* allocated kbytes   */
  UBYTE used;
};

/*****************************************************************************/

static struct fileent files [MAX_FILES];

/*****************************************************************************/

/* Disk allocation bitmap (for duplicate-block checking) */
#define ALLOC_BYTES 512 /* enough for DSM <= 4095 */
static UBYTE alloc_map [ALLOC_BYTES];

/*****************************************************************************/

/* 4-char entries: CON:, AXI:, AXO:, LST:, DEV:, VAL:, USR:, DSK: */
static const char devl [] = "CON:AXI:AXO:LST:DEV:VAL:USR:DSK:";
#define L_SIZE ((int)((sizeof devl - 1) / T_SIZE))
#define OPT_DEV 5
#define OPT_VAL 6
#define OPT_USR 7
#define OPT_DSK 8

/*****************************************************************************/

/* 4-char attribute entries: RO  , RW  , SIZE, SYS , DIR  */
static const char attribl [] = "RO  RW  SIZESYS DIR ";
#define A_SIZE ((int)((sizeof attribl - 1) / T_SIZE))
#define OPT_RO 1
#define OPT_RW 2
#define OPT_SIZE 3
#define OPT_SYS 4
#define OPT_DIR 5

/*****************************************************************************/

/* Physical device names for iobyte display */
static const char devr []
    = "TTY:CRT:BAT:UC1:TTY:PTR:UR1:UR2:TTY:PTP:UP1:UP2:TTY:CRT:LPT:UL1:";
#define P_SIZE ((int)((sizeof devr - 1) / T_SIZE))

/*****************************************************************************/

/* Messages */
static const char readonly   [] = "Read Only (RO)";
static const char readwrite  [] = "Read Write (RW)";
static const char entries    [] = " Directory Entries";
static const char filename   [] = "d:filename.typ";
static const char use_str    [] = "Use: STAT ";
static const char invalid    [] = "Invalid Assignment";
static const char set_to     [] = " set to ";
static const char record_msg [] = "128 Byte Record";
static const char sattrib    [] = "[RO] [RW] [SYS] or [DIR]";
static const char drivename  [] = " Drive ";

/*****************************************************************************/

/* case-insensitive toupper */
static char
uc (char c)
{
  if (c >= 'a' && c <= 'z')
    {
      return (char)(c - 32);
    }

  return c;
}

/*****************************************************************************/

static int ibp; /* index into buff */

/*****************************************************************************/

static void
fill_token (void)
{
  int k;

  for (k = 0; k < T_SIZE; k++)
    {
      token [k] = ' ';
    }
}

/*****************************************************************************/

static void
scan (void)
{
  int b, scandex;

  while (buff [ibp] == ' ')
    {
      ibp++;
    }

  if (buff [ibp] == '[')
    {
      ibp++;
    }

  scandex = 0;
  fill_token ();

  while ((b = buff [ibp]) > 1)
    {
      switch (b)
        {
        case ' ':
        case ',':
        case ':':
        case '[':
        case '=':
          buff [ibp] = 1;

          break;

        default:
          if (b < ' ')
            {
              buff [ibp] = 1;
            }
          else
            {
              ibp++;
            }
        }

      switch (b)
        {
        case '/':
        case '_':
        case ']':
        case ',':
          break;

        default:
          if (scandex < T_SIZE)
            {
              token [scandex] = (char)uc ((char)b);
            }

          scandex++;
        }
    }

  if (b != 0)
    {
      ibp++;
    }
}

/*****************************************************************************/

static int
parse_next (void)
{
  scan ();

  if (token [0] == ' ')
    {
      scan ();

      if (token [0] == ' ')
        {
          return FALSE;
        }
    }

  return TRUE;
}

/*****************************************************************************/

static int
parse_assign (void)
{
  scan ();
  if (token [0] != '=')
    {
      return FALSE;
    }

  scan ();

  return TRUE;
}

/*****************************************************************************/

/* Match token against table of T_SIZE-char entries. Returns 1-based index. */
static int
match (const char *va, int vl)
{
  int sync, k;
  int j = 0;

  for (sync = 1; sync <= vl; sync++)
    {
      int found = TRUE;

      for (k = 0; k < T_SIZE;)
        {
          if (va [j] == ' ' && found)
            {
              break;
            }

          if (va [j++] != token [k++])
            {
              found = FALSE;
            }
        }

      if (found)
        {
          return sync;
        }
    }

  return 0;
}

/*****************************************************************************/

static void
load_dpb (void)
{
  bdos (31, (LONG)(ULONG)dpb_raw);
}

/*****************************************************************************/

static void
set_kpb (void)
{
  load_dpb ();
  kpb = 1 << (DPB_BSH - SPKSHF);
  word_blks = (DPB_DSM > 255) ? 1 : 0;
}

/*****************************************************************************/

static void
do_select (int d)
{
  rodisk = bdos (29, 0); /* BDOS 29 = get read-only vector */
  cdisk = d;
  bdos (14, (LONG)d); /* BDOS 14 = select disk */
}

/*****************************************************************************/

static void
select_disk (int d)
{
  do_select (d);
  set_kpb ();
}

/*****************************************************************************/

static UWORD
count_free (void)
{
  /*
   * BDOS 46 = get free space (CP/M 3 / DOS+).
   * Returns free 128-byte record count in DMA buffer (LONG).
   */

  ULONG free_recs = 0;

  bdos (26, (LONG)(ULONG)dma); /* set DMA */
  bdos (46, (LONG)cdisk);
  {
    const UBYTE *p = dma;
    free_recs = (ULONG)p [0] | ((ULONG)p [1] << 8) | ((ULONG)p [2] << 16);
  }

  return (UWORD)(free_recs >> SPKSHF);
}

/*****************************************************************************/

static void
show_dv (void)
{
  conout ((char)(cdisk + 'A'));
  conout (':');
}

/*****************************************************************************/

static void
show_drive (void)
{
  show_dv ();
  conout (' ');
}

/*****************************************************************************/

static void
show_usr (int user)
{
  printx ("User :");
  pdecimal ((UWORD)user, 100, TRUE);
}

/*****************************************************************************/

static void
printfn (const UBYTE *de) /* print drive:name.typ from a dir entry */
{
  int k;

  show_dv ();

  for (k = 0; k < 8; k++)
    {
      if (k == 0)
        {
          ; /* nothing before name */
        }

      conout ((char)(de [1 + k] & 0x7f));
    }

  conout ('.');

  for (k = 0; k < 3; k++)
    {
      conout ((char)(de [9 + k] & 0x7f));
    }
}

/*****************************************************************************/

static void
prname (const char *a) /* print up to and including ':' */
{
  do
    {
      conout (*a);
    }
  while (*a++ != ':');
}

/*****************************************************************************/

static void
dots (int n)
{
  crlf ();

  while (n--)
    {
      conout ('.');
    }
}

/*****************************************************************************/

/* Drive status display */
static void
drivestatus (void)
{
  ULONG space;

  space = (ULONG)(DPB_DSM + 1) * (ULONG)kpb;
  print ("        ");
  show_drive ();
  printx ("Drive Characteristics");
  p_unl (space * 8);
  printx (record_msg);
  printx (" Capacity");
  p_unl (space);
  printx ("Kilobyte Drive Capacity");
  p_unl ((ULONG)DPB_DRM + 1);
  printx ("32 Byte");
  printx (entries);
  p_unl ((ULONG)DPB_CKS * 4);
  printx ("Checked");
  printx (entries);
  p_unl (((ULONG)DPB_EXM + 1) * 128);
  printx (record_msg);
  printx ("s / Directory Entry");
  p_unl ((ULONG)1 << DPB_BSH);
  printx (record_msg);
  printx ("s / Block");
  p_unl ((ULONG)DPB_SPT);
  printx (record_msg);
  printx ("s / Track");
  p_unl ((ULONG)DPB_OFS);
  printx ("Reserved Tracks");
  crlf ();
}

/*****************************************************************************/

static void
prcount (void)
{
  ULONG free_k = count_free ();

  p_long (free_k);
  conout ('k');
}

/*****************************************************************************/

static void
pralloc (void)
{
  crlf ();
  show_drive ();
  printx (((rodisk >> cdisk) & 1) ? "RO" : "RW");
  printx (", Free Space: ");
  prcount ();
}

/*****************************************************************************/

static void
prstatus (void)
{
  UWORD login;
  int d;

  login = bdos (24, 0); /* BDOS 24 = return login vector */
  d = 0;

  while (login)
    {
      if (login & 1)
        {
          select_disk (d);
          pralloc ();
          login -= 1;
        }

      login >>= 1;
      d++;
    }

  crlf ();
}

/*****************************************************************************/

static void
diskstatus (void)
{
  UWORD login;
  int d;

  login = bdos (24, 0);
  d = 0;
  do
    {
      if (login & 1)
        {
          select_disk (d);
          drivestatus ();
        }

      d++;
    }
  while (login >>= 1);
}

/*****************************************************************************/

static void
userstatus (void)
{
  int i;
  UBYTE user [16];
  UWORD r;

  for (i = 0; i < 16; i++)
    {
      user [i] = 0;
    }

  crlf ();
  show_drive ();
  printx ("Active ");
  show_usr (user_code);
  crlf ();
  show_drive ();
  printx ("Active Files:");

  bdos (26, (LONG)(ULONG)dma);
  /* search all users: use fcb with drive=0, name=all-? */
  {
    UBYTE sfcb [36];

    for (i = 0; i < 36; i++)
      {
        sfcb [i] = 0;
      }

    sfcb [0] = '?'; /* user wildcard via drive byte */

    for (i = 1; i <= 11; i++)
      {
        sfcb [i] = '?';
      }

    r = bdos (17, (LONG)(ULONG)sfcb);
  }

  while (r != 255)
    {
      const UBYTE *de = &dma [(r & 3) * 32];
      int u = de [0] & 0xff;

      if (u < 16 && u != 0xe5)
        {
          user [u & 0x0f] = 1;
        }

      r = bdos (18, 0);
    }

  for (i = 0; i < 16; i++)
    {
      if (user [i])
        {
          pdecimal ((UWORD)i, 100, TRUE);
        }
    }

  crlf ();
}

/*****************************************************************************/

static void
devstatus (void)
{
  UWORD iobyte;
  int j, k;

  iobyte = bdos (7, 0); /* BDOS 7 = get iobyte */
  j = 0;

  for (k = 0; k < 4; k++)
    {
      prname (&devl [k * 4]);
      printx (" is ");
      prname (&devr [((iobyte & 3) * 4) + j]);
      j += 16;
      iobyte >>= 2;
      crlf ();
    }
}

/*****************************************************************************/

static void
values (void)
{
  int j, k;

  print ("File Status   : ");
  printx (filename);
  printx (" [SIZE]");
  print ("Read Only Disk: d:=RO");
  print ("Set Attribute : ");
  printx (filename);
  printx (sattrib);
  print ("Disk Status   : DSK: d:DSK:");
  print ("User Status   : USR: d:USR:");
  print ("Iobyte Value  : DEV:");
  print ("Iobyte Assign :");

  for (j = 0; j < 4; j++)
    {
      crlf ();
      prname (&devl [j * 4]);
      printx (" =");

      for (k = 0; k <= 12; k += 4)
        {
          conout (' ');
          prname (&devr [(j * 16) + k]);
        }
    }

  crlf ();
}

/*****************************************************************************/

static void
prdrive (const char *a)
{
  print (&drivename [1]);
  show_dv ();
  printx (set_to);
  printx (a);
}

/*****************************************************************************/

static void
setdrivestatus (void)
{
  switch (match (attribl, A_SIZE))
    {
    case OPT_RO:
      bdos (28, 0); /* BDOS 28 = write protect disk */
      prdrive (readonly);

      break;

    case OPT_RW:
      if (bdos (37, (LONG)(1UL << cdisk)) != 0) /* BDOS 37 = reset drive */
        {
          print ("Disk Reset Denied");
        }
      else
        {
          prdrive (readwrite);
        }

      break;

    default:
      print (invalid);
      print (use_str);
      printx ("d:=RO");
    }
}

/*****************************************************************************/

/* Check if name in dir entry de matches files [idx] */
static int
name_eq_de (int idx, const UBYTE *de)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      if ((files [idx].name [i] & 0x7f) != (de [1 + i] & 0x7f))
        {
          return 0;
        }
    }

  for (i = 0; i < 3; i++)
    {
      if ((files [idx].typ [i] & 0x7f) != (de [9 + i] & 0x7f))
        {
          return 0;
        }
    }

  return 1;
}

/*****************************************************************************/

#if 0
/* Check if bpfcb (search pattern) matches dir entry de */
static int
pattern_match_de (const UBYTE *pat, const UBYTE *de)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      char p = (char)(pat [1 + i] & 0x7f);

      if (p != '?' && p != (char)(de [1 + i] & 0x7f))
        {
          return 0;
        }
    }

  for (i = 0; i < 3; i++)
    {
      char p = (char)(pat [9 + i] & 0x7f);

      if (p != '?' && p != (char)(de [9 + i] & 0x7f))
        {
          return 0;
        }
    }

  return 1;
}
#endif

/*****************************************************************************/

/* Count allocated 16-bit blocks in extent */
static UWORD
count_blks_16 (const UBYTE *de)
{
  int i;
  UWORD n = 0;

  for (i = 0; i < 8; i++)
    {
      UWORD b = (UWORD)de [16 + i * 2] | ((UWORD)de [17 + i * 2] << 8);

      if (b)
        {
          n++;
        }
    }

  return n;
}

/*****************************************************************************/

/* Count allocated 8-bit blocks in extent */
static UWORD
count_blks_8 (const UBYTE *de)
{
  int i;
  UWORD n = 0;

  for (i = 0; i < 16; i++)
    {
      if (de [16 + i])
        {
          n++;
        }
    }

  return n;
}

/*****************************************************************************/

/* Check/mark blocks in alloc_map; return FALSE on conflict */
static int
allocate_blks (const UBYTE *de)
{
  int i, max;
  UWORD block;
  UWORD vbyte, amask;

  max = word_blks ? 8 : 16;
  for (i = 0; i < max; i++)
    {
      if (word_blks)
        {
          block = (UWORD)de [16 + i * 2] | ((UWORD)de [17 + i * 2] << 8);
        }
      else
        {
          block = de [16 + i] & 0xff;
        }

      if (block == 0)
        {
          continue;
        }

      vbyte = block / 8;
      amask = (UWORD)(1 << (block % 8));

      if (vbyte >= ALLOC_BYTES)
        {
          continue;
        }

      if (amask & alloc_map [vbyte])
        {
          if (error_free)
            {
              error_free = FALSE;
              print ("Bad Directory on ");
              show_dv ();
              print ("Space Allocation Conflict:");
            }

          crlf ();
          printfn (de);

          return FALSE;
        }

      alloc_map [vbyte] |= (UBYTE)amask;
    }

  return TRUE;
}

/*****************************************************************************/

/* Simple insertion sort on files [] by name */
static void
sort_files (void)
{
  int i;

  for (i = 1; i < (int)fcbn; i++)
    {
      struct fileent tmp = files [i];
      int j = i - 1;

      while (j >= 0)
        {
          int k, cmp = 0;
          for (k = 0; k < 8 && !cmp; k++)
            {
              cmp = (files [j].name [k] & 0x7f) - (tmp.name [k] & 0x7f);
            }

          for (k = 0; k < 3 && !cmp; k++)
            {
              cmp = (files [j].typ [k] & 0x7f) - (tmp.typ [k] & 0x7f);
            }

          if (cmp <= 0)
            {
              break;
            }

          files [j + 1] = files [j];
          j--;
        }

      files [j + 1] = tmp;
    }
}

/*****************************************************************************/

/* Print name.typ from fileent */
static void
print_name (int idx)
{
  int k;

  show_dv ();

  for (k = 0; k < 8; k++)
    {
      conout ((char)(files [idx].name [k] & 0x7f));
    }

  conout ('.');

  for (k = 0; k < 3; k++)
    {
      conout ((char)(files [idx].typ [k] & 0x7f));
    }
}

/*****************************************************************************/

static void
set_sattrib_on (struct fileent *f, int scase)
{
  switch (scase)
    {
    case OPT_RO:
      f->ro = 1;
      printx (readonly);

      break;

    case OPT_RW:
      f->ro = 0;
      printx (readwrite);

      break;

    case OPT_SYS:
      f->sys = 1;
      printx ("System (Sys)");

      break;

    case OPT_DIR:
      f->sys = 0;
      printx ("Directory (Dir)");

      break;

    default:
      print (invalid);
    }
}

/*****************************************************************************/

/* Build an FCB from fileent to call BDOS 30 (set file attributes) */
static void
write_attrib (int idx)
{
  UBYTE fcb [36];
  int k;

  for (k = 0; k < 36; k++)
    {
      fcb [k] = 0;
    }

  fcb [0] = 0; /* current drive */

  for (k = 0; k < 8; k++)
    {
      fcb [1 + k] = (UBYTE)(files [idx].name [k] & 0x7f);
    }

  for (k = 0; k < 3; k++)
    {
      fcb [9 + k] = (UBYTE)(files [idx].typ [k] & 0x7f);
    }

  if (files [idx].ro)
    {
      fcb [9] |= 0x80;
    }

  if (files [idx].sys)
    {
      fcb [10] |= 0x80;
    }

  bdos (30, (LONG)(ULONG)fcb); /* BDOS 30 = set file attributes */
}

/*****************************************************************************/

/* Parse attribute modifier after filename */
static int
setfstatus (void)
{
  if (!parse_next ())
    {
      return FALSE;
    }

  if (token [0] == '=')
    {
      scan ();
    }

  scase1 = match (attribl, A_SIZE);

  if (scase1 == OPT_SIZE)
    {
      sizeset = TRUE;

      return FALSE;
    }

  if (scase1 != 0)
    {
      scase2 = 0;

      if (parse_next ())
        {
          int sc2 = match (attribl, A_SIZE);
          int diff = sc2 - scase1;

          if (diff < 0)
            {
              diff = -diff;
            }

          if (sc2 != 0 && diff > 1)
            {
              scase2 = sc2;

              return TRUE;
            }
        }
      else
        {
          return TRUE;
        }
    }

  print (invalid);
  print (use_str);
  printx (filename);
  printx (" [SIZE] ");
  printx (sattrib);
  crlf ();

  bdos (0, 0);

  return FALSE;
}

/*****************************************************************************/

/* Collect all matching directory entries, display or set attributes */
static void
getfile (void)
{
  UBYTE sfcb [36];
  int i, k;
  UWORD r;

  set_attribute = setfstatus ();

  if (set_attribute && scase1 == 0)
    {
      return;
    }

  /* Clear state */
  for (i = 0; i < ALLOC_BYTES; i++)
    {
      alloc_map [i] = 0;
    }

  fcbn = 0;
  error_free = TRUE;
  nfcbs = 0;

  /* Build search FCB: copy from basepage FCB, force all extents */
  for (i = 0; i < 36; i++)
    {
      sfcb [i] = bpfcb [i];
    }

  sfcb [12] = '?'; /* all extents */
  sfcb [14] = '?'; /* all s2 */

  bdos (26, (LONG)(ULONG)dma);
  r = bdos (17, (LONG)(ULONG)sfcb); /* search first */

  fcbmax = MAX_FILES;

  while (r != 255)
    {
      const UBYTE *de = &dma [(r & 3) * 32];
      int u = de [0] & 0xff;

      /* Skip deleted, XFCBs (drive >= 0x20) */
      if (u < 0x20 && u != 0xe5)
        {
          /* Only current user */
          if ((u & 0x0f) == user_code)
            {
              int idx = -1;

              /* find existing entry */
              for (i = 0; i < (int)fcbn; i++)
                {
                  if (name_eq_de (i, de))
                    {
                      idx = i;

                      break;
                    }
                }

              if (idx < 0)
                {
                  /* new file */
                  if (fcbn < fcbmax)
                    {
                      idx = (int)fcbn++;

                      for (k = 0; k < 8; k++)
                        {
                          files [idx].name [k] = (char)(de [1 + k] & 0x7f);
                        }

                      for (k = 0; k < 3; k++)
                        {
                          files [idx].typ [k] = (char)(de [9 + k] & 0x7f);
                        }

                      files [idx].ro = (de [9] & 0x80) ? 1 : 0;
                      files [idx].sys = (de [10] & 0x80) ? 1 : 0;
                      files [idx].arch = (de [11] & 0x80) ? 1 : 0;
                      files [idx].extent_count = 0;
                      files [idx].records = 0;
                      files [idx].kcnt = 0;
                      files [idx].used = 1;
                    }
                }

              if (idx >= 0)
                {
                  UBYTE rc = de [15];
                  UWORD blks
                      = word_blks ? count_blks_16 (de) : count_blks_8 (de);
                  files [idx].records += rc;
                  files [idx].kcnt += (UWORD)(blks * (UWORD)kpb);
                  files [idx].extent_count++;

                  if (de [9] & 0x80)
                    {
                      files [idx].ro = 1;
                    }

                  if (de [10] & 0x80)
                    {
                      files [idx].sys = 1;
                    }

                  nfcbs++;
                  allocate_blks (de);
                }
            }
        }

      r = bdos (18, 0); /* search next */
    }

  if (!error_free)
    {
      bdos (0, 0);
    }

  if (fcbn == 0)
    {
      print ("File Not Found");

      return;
    }

  sort_files ();

  if (set_attribute)
    {
      for (i = 0; i < (int)fcbn; i++)
        {
          crlf ();
          print_name (i);
          printx (set_to);
          set_sattrib_on (&files [i], scase1);

          if (scase2)
            {
              printx (", ");
              set_sattrib_on (&files [i], scase2);
            }

          write_attrib (i);
        }

      return;
    }

  /* Display file listing */
  {
    UWORD tall = 0;
    print (drivename);
    show_drive ();
    blanks (17);
    show_usr (user_code);

    if (sizeset)
      {
        print ("     Size ");
      }
    else
      {
        crlf ();
      }

    printx (" Recs  Bytes FCBs Attrib   Name");

    for (i = 0; i < (int)fcbn; i++)
      {
        UWORD bytes_k = (files [i].records + 7) / 8;
        tall += files [i].kcnt;
        crlf ();

        if (sizeset)
          {
            /* physical record count as "size" */
            p_long ((ULONG)files [i].records);
            conout (' ');
          }

        pdecimal (files [i].records, 10000, TRUE);
        conout (' ');
        pdecimal (bytes_k, 10000, TRUE);
        printx ("k ");
        pdecimal (files [i].extent_count, 1000, TRUE);
        printx (files [i].sys ? " Sys " : " Dir ");
        printx (files [i].ro ? "RO " : "RW ");
        print_name (i);
      }

    dots (39);
    print ("Total:");

    if (sizeset)
      {
        blanks (10);
      }

    pdecimal (tall, 10000, TRUE);
    conout ('k');
    pdecimal (nfcbs, 10000, TRUE);
    printx (" (");
    pdecimal (fcbn, 1000, TRUE);
    printx (fcbn == 1 ? " file)" : " files)");
    pralloc ();
  }
}

/*****************************************************************************/

static void
parse_it (void)
{
  switch (match (devl, L_SIZE))
    {
    case OPT_USR:
      userstatus ();

      break;

    case OPT_DSK:
      drivestatus ();

      break;

    default:
      getfile ();
    }
}

/*****************************************************************************/

static int
devreq (void)
{
  int first = TRUE;

  for (;;)
    {
      int j = match (devl, L_SIZE);

      if (j == 0)
        {
          if (!first)
            {
              goto error;
            }

          return FALSE;
        }

      first = FALSE;

      switch (j)
        {
        case OPT_DEV:
          devstatus ();

          break;

        case OPT_VAL:
          values ();

          break;

        case OPT_USR:
          userstatus ();

          bdos (0, 0);

          break;

        case OPT_DSK:
          diskstatus ();

          break;

        default:
          /* iobyte assignment */
          {
            UWORD iomask;
            int k2;
            k2 = (j - 1) * 16;

            if (!parse_assign ())
              {
                goto error;
              }

            {
              int idx = match (&devr [k2], 4) - 1;

              if (idx < 0)
                {
                  goto error;
                }

              iomask = ~3;
              {
                int jj = j - 1;
                int kk = idx;

                while (jj--)
                  {
                    iomask = (UWORD)((iomask << 2) | 3);
                    kk <<= 2;
                  }

                bdos (8, (LONG)((bdos (7, 0) & iomask)
                                | kk)); /* BDOS 8 set iobyte */
              }
            }
          }
        }

      if (!parse_next ())
        {
          return TRUE;
        }
    }

error:
  print (invalid);

  return TRUE;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  bpfcb = DEF_FCB;
  buff = CMD_TAIL;

  /* make a local copy of the command tail, scan() modifies it */
  {
    static UBYTE cmd_copy [128];
    int tail_len = buff [0];
    int i;

    if (tail_len > 126)
      {
        tail_len = 126;
      }

    for (i = 0; i < tail_len; i++)
      {
        cmd_copy [i + 1] = (UBYTE)uc ((char)buff [i + 1]);
      }

    cmd_copy [tail_len + 1] = 0;
    cmd_copy [0] = (UBYTE)tail_len;
    buff = cmd_copy;
  }

  ibp = 1;
  sizeset = FALSE;
  set_attribute = FALSE;
  scase1 = scase2 = 0;

  cdisk = (int)bdos (25, 0);        /* BDOS 25 = return current disk */
  user_code = (int)bdos (32, 0xFF); /* BDOS 32 = get/set user, 0xFF=get */

  set_kpb ();

  if (!parse_next ())
    {
      prstatus ();

      bdos (0, 0);
    }

  /* Drive specifier? */
  if (token [1] == ':')
    {
      char d = token [0];
      if (d >= 'A' && d <= 'P')
        {
          select_disk (d - 'A');
        }

      if (!parse_next ())
        {
          pralloc ();
          crlf ();

          bdos (0, 0);
        }

      if (token [0] == '=')
        {
          scan ();
          setdrivestatus ();
          crlf ();

          bdos (0, 0);
        }

      parse_it ();
      crlf ();

      bdos (0, 0);
    }

  if (!devreq ())
    {
      getfile ();
    }

  bdos (0, 0);
}

/*****************************************************************************/

#if defined(__clang__) || defined(__clang_version__)
void *
memcpy(void *dst, const void *src, unsigned long n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}
#endif

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
