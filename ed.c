/* ed.c - DRI ED (CP/M-3 PL/M) ported to C for CP/M-386 */

/* Copyright (c) 1976-1982 Digital Research */

/*
 * No support passwords/XFCB/SCB page size; uses 128K
 * Uses edit buffer and PIT sleep (BDOS 226) for Z/wait.
 * No wildcard support yet.
 */

/*
 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
 * XXX                                                 XXX
 * XXX  AI slop alert!! This file was translated from  XXX
 * XXX  CP/M 3 ED.PLM to C using AI tooling and needs  XXX
 * XXX  a LOT of fixing and work! It will most likely  XXX
 * XXX  be trashed-or totally reworked-in the future.  XXX
 * XXX                                                 XXX
 * XXX  This, so far, is the only AI slop in CP/M-386  XXX
 * XXX  and I will NOT be introducing any more of it.  XXX
 * XXX                                                 XXX
 * XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
 */

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned long ULONG;
typedef int BOOL;

#include "absaddr.h"

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

#define TRUE 1
#define FALSE 0
#define CR 13
#define LF 10
#define TAB 9
#define ESC 0x1B
#define ENDFILE 0x1A
#define RUBOUT 0x7F
#define CTLH 8
#define CTLL 0x0C
#define CTLR 0x12
#define CTLU 0x15
#define CTLX 0x18
#define CTRL_Y 0x19
#define WHAT 63
#define POUND 0x23
#define FORWARD 1
#define BACKWARD 0
#define SECTSIZE 128
#define FS 36
#define MACSIZE 128
#define SCRSIZE 100
#define EX 12
#define NR 32
#define RO 9
#define SY 10

/* Edit buffer: MEMORY[0]=LF sentinel; usable 1..MAXM */
#define MEM_SIZE 131072UL
#define NBUF 31 /* source/dest multi-sector buffers (NBUF+1 sectors) */
#define BUFFLENGTH ((NBUF + 1) * SECTSIZE)

static UBYTE memory[MEM_SIZE];
static UBYTE sbuff[BUFFLENGTH];
static UBYTE dbuff[BUFFLENGTH];
static UBYTE xbuff[SECTSIZE];
static UBYTE dma_buff[SECTSIZE];

static ULONG maxm, hmax, max_end;
static ULONG front, back, first, lastc;
static ULONG baseline, relline;
static ULONG distance, tdist;
static ULONG nsource, ndest;
static ULONG sbuffadr, dbuffadr;

static UBYTE sfcb[FS], dfcb[FS], rfcb[FS], xfcb[FS], tmpfcb[FS];
static UBYTE dtype[3];
static UBYTE libfcb[12];
static UBYTE tempfl[3];
static UBYTE backup[3];

static UBYTE nbuf;
static UBYTE xbp, rbp;
static UBYTE xfcbext, xfcbrec;
static UBYTE dcnt;
static UBYTE column, scolumn, tcolumn, qcolumn;
static UBYTE maxlen, comlen, combuff[128], cbp;
static UBYTE macro[MACSIZE], scratch[SCRSIZE];
static UBYTE wbp, wbe, wbj, flag, mp, mi, xp;
static ULONG mt;
static UBYTE direction, ch;
static UBYTE lpp;

static BOOL newfile, onefile, xferon, reading, printsuppress;
static BOOL sysfile, inserting, readbuff, translate, upper, lineset, tail;
static BOOL dot_found;
static const char *err_msg;

static void *jbuf[5];

void _start (void) __attribute__ ((section (".text._start")));

/* ---- BDOS ---- */
static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func), "i"(BDOS_INT),
                      "d"((unsigned long)info)
                    : "memory", "cc");
  return ret;
}

static void
boot (void)
{
  bdos (0, 0);
}

static UBYTE
readchar (void)
{
  return (UBYTE)bdos (1, 0);
}

/* Raw console in (no echo): CP/M 3 BDOS 6 with E=0FDh / 0FFh */
static UBYTE
rawchar (void)
{
  return (UBYTE)bdos (6, 0xFD);
}

static void
printchar (UBYTE c)
{
  if (!printsuppress)
    {
      bdos (2, (LONG)c);
    }
}

static void
setdma (void *a)
{
  bdos (26, (LONG)(ULONG)a);
}

static UBYTE
diskread (UBYTE *fcb)
{
  return (UBYTE)bdos (20, (LONG)(ULONG)fcb);
}

static UBYTE
diskwrite (UBYTE *fcb)
{
  return (UBYTE)bdos (21, (LONG)(ULONG)fcb);
}

static void
rename_f (UBYTE *fcb)
{
  bdos (23, (LONG)(ULONG)fcb);
}

static void
set_attribute (UBYTE *fcb)
{
  bdos (30, (LONG)(ULONG)fcb);
}

static UWORD
open_file (UBYTE *fcb)
{
  return bdos (15, (LONG)(ULONG)fcb);
}

static void
close_f (UBYTE *fcb)
{
  dcnt = (UBYTE)bdos (16, (LONG)(ULONG)fcb);
}

static void
delete_f (UBYTE *fcb)
{
  dcnt = (UBYTE)bdos (19, (LONG)(ULONG)fcb);
}

static void
mem_move_bytes (unsigned c, const UBYTE *s, UBYTE *d)
{
  while (c--)
    {
      *d++ = *s++;
    }
}

static void
fill (UBYTE *s, UBYTE f, unsigned c)
{
  while (c--)
    {
      *s++ = f;
    }
}

/* ---- console ---- */
static void
ttychar (UBYTE c)
{
  if (c >= ' ')
    {
      column++;
    }

  if (c == LF)
    {
      column = 0;
    }

  printchar (c);
}

static void
backspace_tty (void)
{
  if (column == 0)
    {
      return;
    }

  ttychar (CTLH);
  ttychar (' ');
  ttychar (CTLH);
  column -= 2;
}

static void
printabs (UBYTE c)
{
  int i, j;

  i = (c == TAB) ? (int)(7 - (column & 7)) : 0;

  if (c == TAB)
    {
      c = ' ';
    }

  for (j = 0; j <= i; j++)
    {
      ttychar (c);
    }
}

static BOOL
graphic (UBYTE c)
{
  if (c >= ' ')
    {
      return TRUE;
    }

  return c == CR || c == LF || c == TAB;
}

static void
printc (UBYTE c)
{
  if (!graphic (c))
    {
      printabs ('^');
      c = (UBYTE)(c + '@');
    }

  printabs (c);
}

static void
crlf (void)
{
  printc (CR);
  printc (LF);
}

static void
printm (const char *a)
{
  bdos (9, (LONG)(ULONG)a);
}

static void
print (const char *a)
{
  crlf ();
  printm (a);
}

static void
perror_ed (const char *a)
{
  print ("\tERROR - $");
  printm (a);
  crlf ();
}

static void
read_line (UBYTE *buf)
{
  /* buf[0]=maxlen, buf[1]=len filled by BDOS 10 */
  bdos (10, (LONG)(ULONG)buf);
}

static void
readcom (void)
{
  maxlen = 128;
  {
    UBYTE line[130];
    int i;
    line[0] = 128;
    read_line (line);
    comlen = line[1];

    for (i = 0; i < (int)comlen && i < 128; i++)
      {
        combuff[i] = line[2 + i];
      }
  }
}

static BOOL
break_key (void)
{
  if (bdos (11, 0))
    {
      if (readchar () == CTRL_Y)
        {
          return TRUE;
        }
    }

  return FALSE;
}

/* ---- sleep (Z / page wait) via BDOS 225/226 ---- */
struct cpm_ticks
{
  ULONG lo, hi, hz;
};

static void
wait_half (void)
{
  struct cpm_ticks t;
  ULONG half, nlo;
  int i;

  for (i = 0; i < 20; i++)
    {
      if (break_key ())
        {
          flag = POUND;
          __builtin_longjmp (jbuf, 1);
        }

      if (bdos (225, (LONG)(ULONG)&t) == 0 && t.hz)
        {
          half = t.hz / 40; /* ~25ms */
          nlo = t.lo + half;

          if (nlo < t.lo)
            {
              t.hi++;
            }

          t.lo = nlo;
          bdos (226, (LONG)(ULONG)&t);
        }
      else
        {
          volatile ULONG k;

          for (k = 0; k < 50000UL; k++)
            {
              ;
            }
        }
    }
}

/* ---- errors ---- */
static void
reboot (void)
{
  if (xferon)
    {
      delete_f (libfcb);
    }

  boot ();
}

static void
abort_ed (const char *a)
{
  perror_ed (a);
  reboot ();
}

static void
ferr (void)
{
  close_f (dfcb);
  abort_ed ("DIRECTORY FULL$");
}

static void
delete_file (UBYTE *afcb)
{
  delete_f (afcb);
}

static void
rename_file (UBYTE *afcb)
{
  delete_file (afcb + 16);
  rename_f (afcb);
}

static void
make_file (UBYTE *afcb)
{
  delete_file (afcb);
  dcnt = (UBYTE)bdos (22, (LONG)(ULONG)afcb);
}

static void
settype (UBYTE *afcb, const UBYTE *t)
{
  mem_move_bytes (3, t, afcb + 9);
}

static void
setxdma (void)
{
  setdma (xbuff);
}

static void
fillsource (void)
{
  UBYTE i;

  nsource = 0;

  for (i = 0; i <= nbuf; i++)
    {
      setdma (sbuff + nsource);
      dcnt = diskread (sfcb);

      if (dcnt != 0)
        {
          if (dcnt > 1)
            {
              ferr ();
            }

          sbuff[nsource] = ENDFILE;

          break;
        }

      nsource += SECTSIZE;
    }

  nsource = 0;
}

static UBYTE
getsource (void)
{
  UBYTE b;

  if (newfile)
    {
      return ENDFILE;
    }

  if (nsource >= BUFFLENGTH)
    {
      fillsource ();
    }

  b = sbuff[nsource];

  if (b != ENDFILE)
    {
      nsource++;
    }

  return b;
}

static BOOL
erase_bak (void)
{
  if (onefile && newfile)
    {
      mem_move_bytes (FS, dfcb, tmpfcb);
      settype (tmpfcb, backup);
      delete_file (tmpfcb);

      if (dcnt != 255)
        {
          return TRUE;
        }
    }

  return FALSE;
}

/*
 * Flush complete 128-byte records from the dest buffer to the $$$ file.
 * Keeps any partial record (ndest & 0x7f) at the start of dbuff.
 * (Original only flushed when the multi-sector buffer filled; bare "W"
 * on a small buffer never hit disk until "E".)
 */

static void
writedest (void)
{
  ULONG n, i, save_ndest, rem, done;

  n = ndest >> 7;

  if (n == 0)
    {
      return;
    }

  save_ndest = ndest;
  done = 0;

  for (i = 0; i < n; i++)
    {
    retry:
      setdma (dbuff + done);

      if (diskwrite (dfcb) != 0)
        {
          if (erase_bak ())
            {
              goto retry;
            }

          /* slide unwritten bytes down */
          if (done != 0)
            {
              mem_move_bytes ((unsigned)(save_ndest - done), dbuff + done,
                              dbuff);
            }

          ndest = save_ndest - done;
          flag = 'F';
          err_msg = "DISK FULL$";
          __builtin_longjmp (jbuf, 1);
        }

      done += SECTSIZE;
    }

  rem = save_ndest - done; /* 0..127 */

  if (rem)
    {
      mem_move_bytes ((unsigned)rem, dbuff + done, dbuff);
    }

  ndest = rem;
}

static void
putdest (UBYTE b)
{
  if (ndest >= BUFFLENGTH)
    {
      writedest ();
    }

  dbuff[ndest++] = b;
}

/* After W: push full records now so the temp file grows on disk. */
static void
flush_dest_sectors (void)
{
  writedest ();
}

static void
putxfer (UBYTE c)
{
  if (xbp >= SECTSIZE)
    {
    retry:
      setxdma ();
      xfcb[EX] = xfcbext;
      xfcb[NR] = xfcbrec;

      if (diskwrite (xfcb) != 0)
        {
          if (erase_bak ())
            {
              goto retry;
            }

          flag = 'F';
          err_msg = "DISK FULL$";
          __builtin_longjmp (jbuf, 1);
        }

      xbp = 0;
    }

  xbuff[xbp++] = c;
}

static void
close_xfer (void)
{
  UBYTE i;

  for (i = xbp; i <= SECTSIZE; i++)
    {
      putxfer (ENDFILE);
    }

  close_f (xfcb);
}

static BOOL
compare_xfer (void)
{
  int i;

  for (i = 0; i < 12; i++)
    {
      if (xfcb[i] != rfcb[i])
        {
          return FALSE;
        }
    }

  return TRUE;
}

static void
append_xfer (void)
{
  xfcb[EX] = xfcbext;

  if (open_file (xfcb) == 255)
    {
      flag = 'O';
      err_msg = "File not found$";
      __builtin_longjmp (jbuf, 1);
    }

  xfcb[NR] = xfcbrec;
  setxdma ();

  if (diskread (xfcb) == 0)
    {
      xfcbrec = xfcb[NR];

      for (xbp = 0; xbp <= SECTSIZE; xbp++)
        {
          if (xbuff[xbp] == ENDFILE)
            {
              return;
            }
        }
    }
}

static void
finis (void)
{
  while ((ndest & 0x7f) != 0)
    {
      putdest (ENDFILE);
    }

  writedest ();

  if (!newfile)
    {
      close_f (sfcb);
    }

  close_f (dfcb);

  if (dcnt == 255)
    {
      ferr ();
    }

  if (sysfile)
    {
      dfcb[SY] |= 0x80;
      set_attribute (dfcb);
    }

  if (onefile)
    {
      mem_move_bytes (16, sfcb, sfcb + 16);
      settype (sfcb + 16, backup);
      rename_file (sfcb);
    }

  mem_move_bytes (16, dfcb, dfcb + 16);
  settype (dfcb + 16, dtype);
  rename_file (dfcb);
}

/* ---- char helpers ---- */
static void
printnmac (UBYTE c)
{
  if (mp != 0)
    {
      return;
    }

  printc (c);
}

static BOOL
lowercase (UBYTE c)
{
  return c >= 'a' && c <= 'z';
}

static UBYTE
ucase (UBYTE c)
{
  if (lowercase (c))
    {
      return (UBYTE)(c & 0x5f);
    }

  return c;
}

static UBYTE
utran (UBYTE c)
{
  if (c == ESC)
    {
      c = ENDFILE;
    }

  if (translate)
    {
      return ucase (c);
    }

  return c;
}

static void
printvalue (ULONG v)
{
  ULONG k = 10000;
  BOOL zero = FALSE;
  UBYTE d;

  while (k != 0)
    {
      d = (UBYTE)(v / k);
      v = v % k;
      k /= 10;

      if (zero || d != 0)
        {
          zero = TRUE;
          printc ((UBYTE)('0' + d));
        }
      else
        {
          printc (' ');
        }
    }
}

static void
printline (ULONG v)
{
  if (!lineset)
    {
      return;
    }

  printvalue (v);
  printc (':');
  printc (' ');
  printc (inserting ? ' ' : '*');
}

static void
printbase (void)
{
  printline (baseline);
}

static void
printnmbase (void)
{
  if (mp != 0)
    {
      return;
    }

  printbase ();
}

static UBYTE ncmd;

static UBYTE
getcmd (void)
{
  UBYTE *buff = CMD_TAIL;

  if (buff[ncmd + 1] != 0)
    {
      return buff[++ncmd];
    }

  return CR;
}

static UBYTE
readc (void)
{
  if (mp > 0)
    {
      if (break_key ())
        {
          flag = POUND;
          __builtin_longjmp (jbuf, 1);
        }

      if (xp >= mp)
        {
          if (mt != 0)
            {
              if (--mt == 0)
                {
                  flag = POUND;
                  __builtin_longjmp (jbuf, 1);
                }
            }

          xp = 0;
        }

      return utran (macro[xp++]);
    }

  if (inserting)
    {
      /*
       * Interactive insert uses raw BDOS 6 (no echo) so Ctrl-Z is a real
       * 0x1A and ends insert via SCANNING.  Echo like BDOS 1 for others.
       * ESC also ends insert (classic ED UTRAN maps ESC -> ENDFILE).
       */
      UBYTE r = rawchar ();

      if (r == ENDFILE || r == ESC)
        {
          if (r == ENDFILE && mp == 0 && !printsuppress)
            {
              printchar ('^');
              printchar ('Z');
            }

          return ENDFILE;
        }

      /* Rubout/BS: insert loop handles display */
      if (r != CTLH && r != RUBOUT)
        {
          printchar (r);
        }

      return utran (r);
    }

  if (readbuff)
    {
      readbuff = FALSE;
      if (lineset && column == 0)
        {
          if (back >= maxm)
            {
              printline (0);
            }
          else
            {
              printbase ();
            }
        }
      else
        {
          printc ('*');
        }

      readcom ();
      cbp = 0;
      printc (LF);
      column = 0;
    }

  if ((readbuff = (cbp == comlen)))
    {
      combuff[cbp] = CR;
    }

  return utran (combuff[cbp++]);
}

static void
get_uc (void)
{
  if (tail)
    {
      ch = ucase (getcmd ());
    }
  else
    {
      ch = ucase (readc ());
    }
}

/* ---- parse FCB ---- */
static BOOL
parse_fcb (UBYTE *afcb)
{
  int i = 0, delimiter = 0;
  BOOL pflag = FALSE;
  static const char del[] = { CR,  ENDFILE, ' ', ',', '.', ';', ':', '=',
                              '<', '>',     '_', '[', ']', '*', '?' };

  flag = TRUE;
  dot_found = FALSE;
  get_uc ();

  if (ch == CR || ch == ENDFILE)
    {
      return FALSE;
    }

  fill (afcb + 12, 0, 24);
  fill (afcb + 1, ' ', 11);

  while (ch == ' ')
    {
      get_uc ();
    }

  for (;;)
    {
      int d;
      BOOL isdel = FALSE;
      for (d = 0; d < (int)sizeof (del); d++)
        {
          if (ch == (UBYTE)del[d])
            {
              delimiter = d;
              isdel = TRUE;

              if (d > 12)
                {
                  perror_ed ("Cannot Edit Wildcard Filename$");
                }

              break;
            }
        }

      if (isdel)
        {
          break;
        }

      i = 0;

      for (;;)
        {
          isdel = FALSE;

          for (d = 0; d < (int)sizeof (del); d++)
            {
              if (ch == (UBYTE)del[d])
                {
                  delimiter = d;
                  isdel = TRUE;

                  break;
                }
            }

          if (isdel)
            {
              break;
            }

          if (i > 7)
            {
              goto err;
            }

          afcb[++i] = ch;
          pflag = TRUE;
          get_uc ();
        }

      if (ch == ':')
        {
          if (i != 1)
            {
              goto err;
            }

          {
            UBYTE drv = (UBYTE)(afcb[1] - 'A' + 1);

            if (drv > 16)
              {
                goto err;
              }

            afcb[0] = drv;
          }

          afcb[1] = ' ';
          get_uc ();

          continue;
        }

      if (ch == '.')
        {
          i = 8;
          dot_found = TRUE;
          get_uc ();

          for (;;)
            {
              isdel = FALSE;

              for (d = 0; d < (int)sizeof (del); d++)
                {
                  if (ch == (UBYTE)del[d])
                    {
                      delimiter = d;
                      isdel = TRUE;

                      break;
                    }
                }

              if (isdel)
                {
                  break;
                }

              if (i > 10)
                {
                  goto err;
                }

              afcb[++i] = ch;
              pflag = TRUE;
              get_uc ();
            }
        }

      break;
    }

  if (delimiter > 3)
    {
      goto err;
    }

  if (!pflag)
    {
      goto err;
    }

  return TRUE;

err:
  perror_ed ("Invalid Filename$");
  flag = FALSE;

  return FALSE;
}

static void
setdest (void)
{
  if (!tail)
    {
      print ("Enter Output file: $");
      readcom ();
      cbp = 0;
      readbuff = FALSE;
      crlf ();
      crlf ();
    }

  if (parse_fcb (dfcb))
    {
      onefile = FALSE;

      if (dfcb[1] == ' ')
        {
          mem_move_bytes (15, sfcb + 1, dfcb + 1);
        }
    }
  else
    {
      mem_move_bytes (16, sfcb, dfcb);
    }

  mem_move_bytes (3, dfcb + 9, dtype);
}

static void
setrdma (void)
{
  setdma (dma_buff);
}

static UBYTE
readfile (void)
{
  if (rbp >= SECTSIZE)
    {
      setrdma ();

      if (diskread (rfcb) != 0)
        {
          return ENDFILE;
        }

      rbp = 0;
    }

  return utran (dma_buff[rbp++]);
}

static void
setup (void)
{
  sfcb[EX] = 0;
  sfcb[14] = 0;
  sfcb[NR] = 0;
  {
    UWORD ec = open_file (sfcb);
    dcnt = (UBYTE)(ec & 0xff);
  }
  if (onefile)
    {
      if (sfcb[RO] & 0x80)
        {
          abort_ed ("FILE IS READ/ONLY$");
        }

      if (sfcb[SY] & 0x80)
        {
          if (sfcb[8] & 0x80) /* user0 bit in original us=8 */
            {
              dcnt = 255;
            }
          else
            {
              sysfile = TRUE;
            }
        }
    }

  if (dcnt == 255)
    {
      if (!onefile)
        {
          abort_ed ("File not found$");
        }

      newfile = TRUE;
      print ("NEW FILE$");
      crlf ();
    }

  settype (dfcb, tempfl);
  dfcb[EX] = 0;
  make_file (dfcb);

  if (dcnt == 255)
    {
      ferr ();
    }

  dfcb[EX] = 0;
  dfcb[NR] = 0;
  nsource = BUFFLENGTH;
  ndest = 0;
  baseline = 1;
}

/* ---- buffer management ---- */
static void
setff (void)
{
  distance = 0xFFFFUL;
}

static BOOL
distzero (void)
{
  return distance == 0;
}

static void
zerodist (void)
{
  distance = 0;
}

static BOOL
distnzero (void)
{
  if (!distzero ())
    {
      distance--;

      return TRUE;
    }

  return FALSE;
}

static void
setlimits (void)
{
  ULONG i, l, k, m;
  BOOL middle, looping;

  relline = 1;

  if (direction == BACKWARD)
    {
      distance++;
      i = front;
      l = 0;
      k = (ULONG)-1;
    }
  else
    {
      i = back;
      l = maxm;
      k = 1;
    }

  looping = TRUE;
  while (looping)
    {
      while ((middle = (i != l)) && memory[(m = i + k)] != LF)
        {
          i = m;
        }
      looping = ((distance = distance - 1) != 0);

      if (!middle)
        {
          looping = FALSE;
          i = i - k;
        }
      else
        {
          relline--;

          if (looping)
            {
              i = m;
            }
        }
    }

  if (direction == BACKWARD)
    {
      first = i;
      lastc = front - 1;
    }
  else
    {
      first = back + 1;
      lastc = i + 1;
    }
}

static void
incbase (void)
{
  baseline++;
}

static void
decbase (void)
{
  baseline--;
}

static void
incfront (void)
{
  front++;
}

static void
incback (void)
{
  back++;
}

static void
decfront (void)
{
  front--;

  if (memory[front] == LF)
    {
      decbase ();
    }
}

static void
decback (void)
{
  back--;
}

static void
mem_move_ed (BOOL moveflag)
{
  UBYTE c;

  if (direction == FORWARD)
    {
      while (back < lastc)
        {
          incback ();

          if (moveflag)
            {
              c = memory[back];

              if (c == LF)
                {
                  incbase ();
                }

              memory[front] = c;
              incfront ();
            }
        }
    }
  else
    {
      while (front > first)
        {
          decfront ();

          if (moveflag)
            {
              memory[back] = memory[front];
              decback ();
            }
        }
    }
}

static void
mover (void)
{
  mem_move_ed (TRUE);
}

static void
setptrs (void)
{
  mem_move_ed (FALSE);
}

static void
movelines (void)
{
  /*
   * Forward L with nothing after the gap is a no-op; if there is text
   * before the gap, step backward so "L" after paging still moves.
   */

  if (direction == FORWARD && back >= maxm && front > 1)
    {
      direction = BACKWARD;
    }

  setlimits ();
  mover ();
}

static void
setfront (ULONG newfront)
{
  while (front != newfront)
    {
      decfront ();
    }
}

static void
setclimits (void)
{
  if (direction == BACKWARD)
    {
      lastc = back;

      if (distance > front)
        {
          first = 1;
        }
      else
        {
          first = front - distance;
        }
    }
  else
    {
      first = front;

      if (distance >= max_end - back)
        {
          lastc = maxm;
        }
      else
        {
          lastc = back + distance;
        }
    }
}

static void
readline_ed (void)
{
  UBYTE b;

  for (;;)
    {
      if (front >= back)
        {
          flag = '>';
          __builtin_longjmp (jbuf, 1);
        }

      b = getsource ();

      if (upper)
        {
          b = ucase (b);
        }

      if (b == ENDFILE)
        {
          zerodist ();

          return;
        }

      memory[front] = b;
      incfront ();

      if (b == LF)
        {
          incbase ();

          return;
        }
    }
}

static void
writeline (void)
{
  UBYTE b;

  for (;;)
    {
      if (back >= maxm)
        {
          zerodist ();

          return;
        }

      incback ();
      b = memory[back];
      putdest (b);

      if (b == LF)
        {
          incbase ();

          return;
        }
    }
}

static void
wrhalf (void)
{
  setff ();
  while (distnzero ())
    {
      if (hmax >= (maxm - back))
        {
          zerodist ();
        }
      else
        {
          writeline ();
        }
    }
}

static void
writeout (void)
{
  direction = BACKWARD;
  first = 1;
  lastc = back;
  mover ();

  if (distzero ())
    {
      wrhalf ();
    }

  while (distnzero ())
    {
      writeline ();
    }

  if (back < lastc)
    {
      direction = FORWARD;
      mover ();
    }

  /* Commit full CP/M records to the open $$$ destination now. */
  flush_dest_sectors ();
}

static void
clearmem (void)
{
  setff ();
  writeout ();
}

static void
terminate (void)
{
  clearmem ();

  if (!newfile)
    {
      while ((ch = getsource ()) != ENDFILE)
        {
          putdest (ch);
        }
    }

  finis ();
}

static void
insert (void)
{
  if (front == back)
    {
      flag = '>';
      __builtin_longjmp (jbuf, 1);
    }

  memory[front] = ch;
  incfront ();

  if (ch == LF)
    {
      incbase ();
    }
}

static BOOL
scanning (void)
{
  ch = readc ();

  return !(ch == ENDFILE || (ch == CR && !inserting));
}

static void
collect (void)
{
  while (scanning ())
    {
      if (ch == CTLL)
        {
          ch = CR;

          if (wbe >= SCRSIZE)
            {
              flag = '>';
              __builtin_longjmp (jbuf, 1);
            }

          scratch[wbe++] = ch;
          ch = LF;
        }

      if (ch == 0)
        {
          flag = WHAT;
          __builtin_longjmp (jbuf, 1);
        }

      if (wbe >= SCRSIZE)
        {
          flag = '>';
          __builtin_longjmp (jbuf, 1);
        }

      scratch[wbe++] = ch;
    }
}

static BOOL
find_str (UBYTE pa, UBYTE pb)
{
  ULONG j = back;
  UBYTE k, match = FALSE;

  while (!match && maxm > j)
    {
      lastc = j = j + 1;
      k = pa;
      while (scratch[k] == memory[lastc] && !(match = (k == pb)))
        {
          k++;
          lastc++;
        }
    }

  if (match)
    {
      lastc--;
      mover ();
    }

  return match;
}

static void
setfind (void)
{
  wbe = 0;
  collect ();
  wbp = wbe;
}

static void
chkfound (void)
{
  if (!find_str (0, wbp))
    {
      flag = POUND;
      __builtin_longjmp (jbuf, 1);
    }
}

static BOOL
parse_lib (UBYTE *fcbadr)
{
  BOOL b = parse_fcb (fcbadr);

  if (!flag)
    {
      flag = 'O';
      __builtin_longjmp (jbuf, 1);
    }

  if (fcbadr[9] == ' ' && !dot_found)
    {
      mem_move_bytes (3, libfcb + 9, fcbadr + 9);
    }

  if (fcbadr[1] == ' ')
    {
      mem_move_bytes (8, libfcb + 1, fcbadr + 1);
    }

  return b;
}

static void
printrel (void)
{
  printline (baseline + relline);
}

static void
typelines (void)
{
  ULONG i;
  UBYTE c;
  BOOL save_ins = inserting;
  ULONG save_dist = distance;

  /*
   * Gap buffer: text after the cursor is at back+1..maxm; text before is
   * 1..front-1.  After P/W/etc. everything can sit on the left, so a
   * forward T/P would print nothing — fall back to backward then.
   */
  if (direction == FORWARD && back >= maxm && front > 1)
    {
      direction = BACKWARD;
      distance = save_dist ? save_dist : 1;
    }

  setlimits ();
  inserting = TRUE; /* line header uses space not '*' during typeout */

  if (direction == FORWARD)
    {
      relline = 0;
      i = front;
    }
  else
    {
      i = first;
    }

  if (first > lastc || first == 0 || lastc > maxm)
    {
      inserting = save_ins;

      return;
    }

  c = memory[i - 1];

  if (c == LF)
    {
      if (column != 0)
        {
          crlf ();
        }
    }
  else
    {
      relline++;
    }

  for (i = first; i <= lastc; i++)
    {
      if (c == LF)
        {
          printrel ();
          relline++;

          if (break_key ())
            {
              inserting = save_ins;
              flag = POUND;
              __builtin_longjmp (jbuf, 1);
            }
        }

      c = memory[i];
      printc (c);
    }

  inserting = save_ins;
}

static void
setlpp (void)
{
  distance = lpp;
}

static void
savedist (void)
{
  tdist = distance;
}

static void
restdist (void)
{
  distance = tdist;
}

static void
page (void)
{
  UBYTE i;
  ULONG back_before;

  savedist ();
  setlpp ();
  back_before = back;
  movelines ();
  i = direction;

  /*
   * After paging forward past the last lines, the gap's right side is
   * empty and a plain TYPELINES would print nothing.  Show the page we
   * just moved onto (backward from the gap) so bare "P" works on short
   * files (e.g. README after 0A).
   */

  if (direction == FORWARD && back >= maxm && back_before < maxm)
    {
      direction = BACKWARD;
      setlpp ();
      typelines ();
    }
  else
    {
      direction = FORWARD;
      setlpp ();
      typelines ();
    }

  direction = i;

  if (lastc == maxm || first == 1)
    {
      zerodist ();
    }
  else
    {
      restdist ();
    }
}

static void
setforward (void)
{
  direction = FORWARD;
  distance = 1;
}

static void
apphalf (void)
{
  setff ();

  while (distnzero ())
    {
      if (front >= hmax)
        {
          zerodist ();
        }
      else
        {
          readline_ed ();
        }
    }
}

static void
inscrlf (void)
{
  ch = CR;
  insert ();
  ch = LF;
  insert ();
}

static void
ins_error_chk (void)
{
  if (tcolumn == 255 || front == 1)
    {
      flag = 'E';
      __builtin_longjmp (jbuf, 1);
    }
}

static void
testcase (void)
{
  BOOL t;

  translate = TRUE;
  t = lowercase (ch);
  ch = utran (ch);
  translate = upper || !t;
}

static void
readctran (void)
{
  translate = FALSE;
  ch = readc ();
  testcase ();
}

static BOOL
singlecom (UBYTE c)
{
  UBYTE i;

  if (ch != c || mp != 0)
    {
      return FALSE;
    }

  /* Allow trailing blanks so "Q " / "E " still count as single commands. */
  for (i = cbp; i < comlen; i++)
    {
      if (combuff[i] != ' ')
        {
          return FALSE;
        }
    }

  return TRUE;
}

static BOOL
singlercom (UBYTE c)
{
  UBYTE i;

  if (!singlecom (c))
    {
      return FALSE;
    }

  for (;;)
    {
      crlf ();
      printchar (c);
      printm ("-(Y/N)?$");
      /* Raw input: avoids echo/Ctrl-Z quirks leaving us stuck in the prompt. */
      i = ucase (rawchar ());

      if (i >= ' ')
        {
          printchar (i);
        }

      crlf ();

      if (i == 'N')
        {
          flag = 0;
          mp = 0;
          __builtin_longjmp (jbuf, 1);
        }

      if (i == 'Y')
        {
          return TRUE;
        }
    }
}

static BOOL
digit (void)
{
  return (UBYTE)(ch - '0') <= 9;
}

static void
number (void)
{
  distance = 0;

  while (digit ())
    {
      distance = distance * 10 + (ULONG)(ch - '0');
      readctran ();
    }
}

static void
reldistance (void)
{
  if (distance > baseline)
    {
      direction = FORWARD;
      distance = distance - baseline;
    }
  else
    {
      direction = BACKWARD;
      distance = baseline - distance;
    }
}

/* ---- main ---- */
void
_start (void)
{
  int i;

  /* init tables */
  fill (rfcb, 0, FS);
  mem_move_bytes (8, (const UBYTE *)"        ", rfcb + 1);
  mem_move_bytes (3, (const UBYTE *)"LIB", rfcb + 9);
  fill (xfcb, 0, FS);
  mem_move_bytes (8, (const UBYTE *)"X$$$$$$$", xfcb + 1);
  mem_move_bytes (3, (const UBYTE *)"LIB", xfcb + 9);
  fill (libfcb, 0, 12);
  mem_move_bytes (8, (const UBYTE *)"X$$$$$$$", libfcb + 1);
  mem_move_bytes (3, (const UBYTE *)"LIB", libfcb + 9);
  mem_move_bytes (3, (const UBYTE *)"$$$", tempfl);
  mem_move_bytes (3, (const UBYTE *)"BAK", backup);

  newfile = FALSE;
  onefile = TRUE;
  xferon = FALSE;
  reading = FALSE;
  printsuppress = FALSE;
  sysfile = FALSE;
  translate = FALSE;
  upper = FALSE;
  lineset = TRUE;
  tail = TRUE;
  lpp = 23;
  scolumn = 8;
  column = 0;
  ncmd = 0;
  err_msg = 0;
  cbp = 0;

  nbuf = NBUF;
  max_end = MEM_SIZE - 1;
  maxm = max_end - 1;
  hmax = maxm / 2;
  memory[max_end] = 0;
  sbuffadr = (ULONG)(ULONG)(void *)sbuff;
  dbuffadr = (ULONG)(ULONG)(void *)dbuff;
  (void)sbuffadr;
  (void)dbuffadr;

  /* Source file: default FCB (0x5C) or prompt */
  for (i = 0; i < FS; i++)
    {
      sfcb[i] = 0;
    }

  if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
    {
      print ("Enter Input  file: $");
      readcom ();
      crlf ();
      tail = FALSE;
      readbuff = FALSE;
      cbp = 0;

      if (!parse_fcb (sfcb))
        {
          reboot ();
        }

      onefile = TRUE;
      setdest ();
    }
  else
    {
      for (i = 0; i < 16; i++)
        {
          sfcb[i] = DEF_FCB[i];
        }

      for (i = 16; i < FS; i++)
        {
          sfcb[i] = 0;
        }

      /* Optional dest in 2nd default FCB at 0x6C */
      {
        UBYTE *f2 = (UBYTE *)abs_ptr (0x6C);

        if (f2[1] != ' ' && f2[1] != 0)
          {
            for (i = 0; i < 16; i++)
              {
                dfcb[i] = f2[i];
              }

            for (i = 16; i < FS; i++)
              {
                dfcb[i] = 0;
              }

            onefile = FALSE;
            mem_move_bytes (3, dfcb + 9, dtype);
          }
        else
          {
            onefile = TRUE;
            mem_move_bytes (16, sfcb, dfcb);
            mem_move_bytes (3, dfcb + 9, dtype);
          }
      }
    }

  tail = FALSE;

  if ((sfcb[0] != dfcb[0]) || !onefile)
    {
      if (open_file (dfcb) != 255)
        {
          abort_ed ("Output File Exists, Erase It$");
        }
    }

restart:
  setup ();
  memory[0] = LF;
  front = 1;
  back = maxm;
  column = 0;

start:
  readbuff = TRUE;
  mp = 0;

  for (;;)
    {
      inserting = FALSE;

      if (__builtin_setjmp (jbuf) != 0)
        {
          /* RESET */
          printsuppress = FALSE;

          if (flag == 0)
            {
              goto start; /* N on O/Q confirm */
            }

          print ("\tBREAK \"$");
          printc (flag);
          printm ("\" AT $");

          if (ch == CR || ch == LF)
            {
              printm ("END OF LINE$");
            }
          else
            {
              printc (ch);
            }

          if (err_msg)
            {
              perror_ed (err_msg);
              err_msg = 0;
            }

          crlf ();
          goto start;
        }

      readctran ();
      flag = 'E';
      mi = cbp;

      if (singlecom ('E'))
        {
          terminate ();
          reboot ();
        }
      else if (singlecom ('H'))
        {
          terminate ();
          newfile = FALSE;

          if (onefile)
            {
              UBYTE t = dfcb[0];
              dfcb[0] = sfcb[0];
              sfcb[0] = t;
            }
          else
            {
              settype (dfcb, dtype);
              mem_move_bytes (16, dfcb, sfcb);
              onefile = TRUE;
            }

          goto restart;
        }
      else if (ch == 'I')
        {
          inserting = (cbp == comlen) && (mp == 0);

          if (inserting)
            {
              tcolumn = 255;
              distance = 0;
              direction = BACKWARD;

              if (memory[front - 1] == LF)
                {
                  printbase ();
                }
              else
                {
                  typelines ();
                }
            }

          while (scanning ())
            {
              while (ch != 0)
                {
                  if (ch == CTLU || ch == CTLX || ch == CTLR)
                    {
                      if (ch == CTLR)
                        {
                          crlf ();
                          typelines ();
                        }
                      else
                        {
                          setlimits ();
                          setptrs ();

                          if (ch == CTLU)
                            {
                              crlf ();
                              printnmbase ();
                            }
                          else
                            {
                              while (column > scolumn)
                                {
                                  backspace_tty ();
                                }
                            }
                        }
                    }
                  else if (ch == CTLH)
                    {
                      ins_error_chk ();
                      tcolumn = column;

                      if (tcolumn > 0)
                        {
                          printnmac (' ');
                        }

                      decfront ();

                      if (tcolumn > scolumn)
                        {
                          printsuppress = TRUE;
                          column = 0;
                          typelines ();
                          printsuppress = FALSE;
                          qcolumn = column;

                          if (qcolumn < scolumn)
                            {
                              qcolumn = scolumn;
                            }

                          column = tcolumn;

                          while (column > qcolumn)
                            {
                              backspace_tty ();
                            }
                        }
                      else
                        {
                          if (memory[front - 1] == CR)
                            {
                              decfront ();
                            }

                          crlf ();
                          typelines ();
                        }

                      ch = 0;
                    }
                  else if (ch == RUBOUT)
                    {
                      ins_error_chk ();
                      decfront ();
                      ch = memory[front];
                      printc (ch);
                      ch = 0;
                    }
                  else if (ch == LF && memory[front - 1] != CR)
                    {
                      printc (CR);
                      inscrlf ();
                    }
                  else
                    {
                      if (!graphic (ch))
                        {
                          printnmac ('^');
                          printnmac ((UBYTE)(ch + '@'));
                        }

                      if (ch == CTLL && !inserting)
                        {
                          inscrlf ();
                        }
                      else
                        {
                          if (mp == 0)
                            {
                              if (ch >= ' ')
                                {
                                  column++;
                                }
                              else if (ch == TAB)
                                {
                                  column
                                      = (UBYTE)(column + (8 - (column & 7)));
                                }
                            }

                          insert ();
                        }
                    }

                  if (ch == LF)
                    {
                      printnmbase ();
                    }

                  if (ch == CR)
                    {
                      ch = LF;
                      printnmac (ch);
                    }
                  else
                    {
                      ch = 0;
                    }

                  tcolumn = 0;
                }
            }
          if (ch != ENDFILE)
            {
              inscrlf ();
              column = 0;
            }

          if (inserting && lineset)
            {
              crlf ();
            }
        }
      else if (singlercom ('O'))
        {
          close_f (sfcb);
          goto restart;
        }
      else if (ch == 'R')
        {
          setrdma ();
          flag = parse_lib (rfcb) ? 1 : 0;

          if (!reading)
            {
              if (!flag)
                {
                  mem_move_bytes (12, xfcb, rfcb);
                }

              rfcb[12] = 0;
              rfcb[32] = 0;
              rbp = SECTSIZE;

              if (open_file (rfcb) == 255)
                {
                  flag = 'O';
                  err_msg = "File not found$";
                  __builtin_longjmp (jbuf, 1);
                }

              reading = TRUE;
            }

          while ((ch = readfile ()) != ENDFILE)
            {
              insert ();
            }

          reading = FALSE;
          close_f (rfcb);
        }
      else if (singlercom ('Q'))
        {
          mp = 0;
          printsuppress = FALSE;
          delete_file (dfcb);

          if (newfile || !onefile)
            {
              settype (dfcb, dtype);
              delete_file (dfcb);
            }

          reboot ();
        }
      else
        {
          /* direction/distance commands */
          setforward ();

          if (ch == '-')
            {
              readctran ();
              direction = BACKWARD;
            }

          if (ch == POUND)
            {
              setff ();
              readctran ();
            }
          else if (digit ())
            {
              number ();

              if (ch == ':')
                {
                  ch = 'L';
                  reldistance ();
                }
            }
          else if (ch == ':')
            {
              readctran ();
              number ();
              reldistance ();

              if (direction == FORWARD)
                {
                  distance++;
                }
            }

          if (distzero ())
            {
              direction = BACKWARD;
            }

          if (ch == 'B')
            {
              direction = 1 - direction;
              first = 1;
              lastc = maxm;
              mover ();
            }
          else if (ch == 'C')
            {
              setclimits ();
              mover ();
            }
          else if (ch == 'D')
            {
              setclimits ();
              setptrs ();
            }
          else if (ch == 'K')
            {
              setlimits ();
              setptrs ();
            }
          else if (ch == 'L')
            {
              movelines ();
            }
          else if (ch == 'P')
            {
              if (distzero ())
                {
                  direction = FORWARD;
                  setlpp ();
                  typelines ();
                }
              else
                {
                  while (distnzero ())
                    {
                      page ();
                      wait_half ();
                    }
                }
            }
          else if (ch == 'T')
            {
              typelines ();
            }
          else if (ch == 'U')
            {
              upper = (direction == FORWARD);
            }
          else if (ch == 'V')
            {
              if (distzero ())
                {
                  printvalue (back - front);
                  printc ('/');
                  printvalue (maxm);
                  crlf ();
                }
              else if ((lineset = (direction == FORWARD)))
                {
                  scolumn = 8;
                }
              else
                {
                  scolumn = 0;
                }
            }
          else if (ch == CR)
            {
              if (mi == 1 && mp == 0)
                {
                  movelines ();
                  setforward ();
                  typelines ();
                }
            }
          else if (direction == FORWARD || distzero ())
            {
              if (ch == 'A')
                {
                  direction = FORWARD;
                  first = front;
                  lastc = maxm;
                  mover ();

                  if (distzero ())
                    {
                      apphalf ();
                    }

                  while (distnzero ())
                    {
                      readline_ed ();
                    }

                  direction = BACKWARD;
                  mover ();
                }
              else if (ch == 'F')
                {
                  setfind ();

                  while (distnzero ())
                    {
                      chkfound ();
                    }
                }
              else if (ch == 'J')
                {
                  ULONG t;
                  setfind ();
                  collect ();
                  wbj = wbe;
                  collect ();

                  while (distnzero ())
                    {
                      chkfound ();
                      mi = (UBYTE)(wbp - 1);

                      while ((UBYTE)(++mi) < wbj)
                        {
                          ch = scratch[mi];
                          insert ();
                        }

                      t = front;

                      if (!find_str (wbj, wbe))
                        {
                          flag = POUND;
                          __builtin_longjmp (jbuf, 1);
                        }

                      first = front - (wbe - wbj);
                      direction = BACKWARD;
                      mover ();
                      setfront (t);
                    }
                }
              else if (ch == 'M' && mp == 0)
                {
                  xp = 255;

                  if (distance == 1)
                    {
                      zerodist ();
                    }

                  while ((macro[++xp] = readc ()) != CR)
                    {
                      ;
                    }
                  mp = xp;
                  xp = 0;
                  mt = distance;
                }
              else if (ch == 'N')
                {
                  setfind ();

                  while (distnzero ())
                    {
                      while (!find_str (0, wbp))
                        {
                          if (break_key ())
                            {
                              flag = POUND;
                              __builtin_longjmp (jbuf, 1);
                            }

                          savedist ();
                          clearmem ();
                          apphalf ();
                          direction = BACKWARD;
                          first = 1;
                          mover ();
                          restdist ();
                          direction = FORWARD;

                          if (back >= maxm)
                            {
                              flag = POUND;
                              __builtin_longjmp (jbuf, 1);
                            }
                        }
                    }
                }
              else if (ch == 'S')
                {
                  setfind ();
                  collect ();

                  while (distnzero ())
                    {
                      chkfound ();
                      mi = wbp;
                      setfront (front - mi);

                      while (mi < wbe)
                        {
                          ch = scratch[mi++];
                          insert ();
                        }
                    }
                }
              else if (ch == 'W')
                {
                  /*
		   * nW / 0W / #W: write lines into open $$$; flush records.
                   * Final name appears only after E (rename $$$ -> file)
		   */

                  writeout ();
                }
              else if (ch == 'X')
                {
                  flag = parse_lib (rfcb);
                  xbp = 0;

                  if (distzero ())
                    {
                      xferon = FALSE;
                      delete_f (rfcb);

                      if (dcnt == 255)
                        {
                          perror_ed ("File not found$");
                        }
                    }
                  else
                    {
                      ULONG ii;

                      if (xferon && compare_xfer ())
                        {
                          append_xfer ();
                        }
                      else
                        {
                          xferon = TRUE;
                          mem_move_bytes (12, rfcb, xfcb);
                          xfcbext = xfcbrec = xfcb[EX] = xfcb[NR] = 0;
                          make_file (xfcb);

                          if (dcnt == 255)
                            {
                              flag = 'F';
                              err_msg = "DIRECTORY FULL$";
                              __builtin_longjmp (jbuf, 1);
                            }
                        }

                      setlimits ();

                      for (ii = first; ii <= lastc; ii++)
                        {
                          putxfer (memory[ii]);
                        }

                      close_xfer ();
                    }
                }
              else if (ch == 'Z')
                {
                  if (distzero ())
                    {
                      if (readchar () == ENDFILE)
                        {
                          flag = POUND;
                          __builtin_longjmp (jbuf, 1);
                        }
                    }

                  while (distnzero ())
                    {
                      wait_half ();
                    }
                }
              else if (ch != 0)
                {
                  flag = WHAT;
                  __builtin_longjmp (jbuf, 1);
                }
            }
          else
            {
              flag = WHAT;
              __builtin_longjmp (jbuf, 1);
            }
        }
    }
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
