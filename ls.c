/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: d66b04da-82b4-11f1-89db-80ee73e9b8e7
 */

/*****************************************************************************/

/* ls.c - directory lister for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/*****************************************************************************/

#define MAX_FILES 128
#define MAX_PATS 8

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

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

/*****************************************************************************/

static void
putch (char c)
{
  bdos (2, (LONG)(unsigned char)c);
}

/*****************************************************************************/

static void
puts (const char *s)
{
  while (*s)
    {
      putch (*s++);
    }
}

/*****************************************************************************/

static void
putu (unsigned long n)
{
  char b[12];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 12)
    {
      b[i++] = (char)('0' + n % 10);
      n /= 10;
    }

  while (i)
    {
      putch (b[--i]);
    }
}

/*****************************************************************************/

static void
putu_w (unsigned long n, int w)
{
  char b[12];
  int i = 0, j;

  if (!n)
    {
      b[i++] = '0';
    }
  else
    {
      while (n && i < 12)
        {
          b[i++] = (char)('0' + n % 10);
          n /= 10;
        }
    }

  for (j = i; j < w; j++)
    {
      putch (' ');
    }

  while (i)
    {
      putch (b[--i]);
    }
}

/*****************************************************************************/

/* DOS-PLUS exact size from total records + LRBC of last extent */
static unsigned long
exact_size (unsigned long records, UBYTE lrbc)
{
  if (records == 0)
    {
      return 0;
    }

  if (lrbc == 0)
    {
      return records * 128UL;
    }

  return (records - 1) * 128UL + (unsigned long)lrbc;
}

/*****************************************************************************/

struct fileent
{
  char name[8];
  char typ[3];
  unsigned long records; /* sum of rc over extents */
  unsigned long blocks;  /* allocated map blocks */
  unsigned fcbs;         /* extent count */
  UBYTE lrbc;            /* last extent s1 (DOS-PLUS) */
  UBYTE last_ex;         /* highest extent seen */
  UBYTE last_s2;
  UBYTE sys;
  UBYTE ro;
  UBYTE used;
};

/*****************************************************************************/

static struct fileent files[MAX_FILES];
static int nfiles;

/*****************************************************************************/

static int
name_eq (const struct fileent *f, const UBYTE *de)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      if ((f->name[i] & 0x7f) != (de[1 + i] & 0x7f))
        {
          return 0;
        }
    }

  for (i = 0; i < 3; i++)
    {
      if ((f->typ[i] & 0x7f) != (de[9 + i] & 0x7f))
        {
          return 0;
        }
    }

  return 1;
}

/*****************************************************************************/

static unsigned
count_map_blocks (const UBYTE *de, int word_mode)
{
  unsigned n = 0;
  int i;

  if (word_mode)
    {
      for (i = 0; i < 8; i++)
        {
          UWORD b = (UWORD)de[16 + i * 2] | ((UWORD)de[17 + i * 2] << 8);

          if (b)
            {
              n++;
            }
        }
    }
  else
    {
      for (i = 0; i < 16; i++)
        {
          if (de[16 + i])
            {
              n++;
            }
        }
    }

  return n;
}

/*****************************************************************************/

static int
later_extent (UBYTE s2, UBYTE ex, UBYTE os2, UBYTE oex)
{
  if ((s2 & 0x3f) > (os2 & 0x3f))
    {
      return 1;
    }

  if ((s2 & 0x3f) < (os2 & 0x3f))
    {
      return 0;
    }

  return (ex & 0x1f) >= (oex & 0x1f);
}

/*****************************************************************************/

static void
add_dirent (const UBYTE *de, int word_mode)
{
  int i, idx = -1;
  UBYTE ex = de[12];
  UBYTE s1 = de[13];
  UBYTE s2 = de[14];
  UBYTE rc = de[15];

  if (de[0] >= 16) /* empty / XFCB / etc. */
    {
      return;
    }

  for (i = 0; i < nfiles; i++)
    {
      if (files[i].used && name_eq (&files[i], de))
        {
          idx = i;

          break;
        }
    }

  if (idx < 0)
    {
      if (nfiles >= MAX_FILES)
        {
          return;
        }

      idx = nfiles++;

      for (i = 0; i < 8; i++)
        {
          files[idx].name[i] = (char)(de[1 + i] & 0x7f);
        }

      for (i = 0; i < 3; i++)
        {
          files[idx].typ[i] = (char)(de[9 + i] & 0x7f);
        }

      files[idx].records = 0;
      files[idx].blocks = 0;
      files[idx].fcbs = 0;
      files[idx].lrbc = 0;
      files[idx].last_ex = 0;
      files[idx].last_s2 = 0;
      files[idx].sys = 0;
      files[idx].ro = 0;
      files[idx].used = 1;
    }

  files[idx].records += rc;
  files[idx].blocks += count_map_blocks (de, word_mode);
  files[idx].fcbs++;

  if (de[9] & 0x80)
    {
      files[idx].ro = 1;
    }

  if (de[10] & 0x80)
    {
      files[idx].sys = 1;
    }

  if (later_extent (s2, ex, files[idx].last_s2, files[idx].last_ex))
    {
      files[idx].last_ex = ex;
      files[idx].last_s2 = s2;
      files[idx].lrbc = s1; /* DOS-PLUS LRBC on last extent */
    }
}

/*****************************************************************************/

static int
cmp_name (const struct fileent *a, const struct fileent *b)
{
  int i, d;

  for (i = 0; i < 8; i++)
    {
      d = (a->name[i] & 0x7f) - (b->name[i] & 0x7f);

      if (d)
        {
          return d;
        }
    }

  for (i = 0; i < 3; i++)
    {
      d = (a->typ[i] & 0x7f) - (b->typ[i] & 0x7f);

      if (d)
        {
          return d;
        }
    }

  return 0;
}

/*****************************************************************************/

static void
sort_files (int sort_by, int reverse, int ignore_lrbc)
{
  int i, j;
  struct fileent t;

  if (!sort_by && !reverse)
    {
      return;
    }

  if (!sort_by && reverse)
    {
      sort_by = 1;
    }

  for (i = 0; i < nfiles; i++)
    {
      for (j = i + 1; j < nfiles; j++)
        {
          int c = 0;

          if (sort_by == 2)
            {
              unsigned long sa = exact_size (files[i].records,
                                             ignore_lrbc ? 0 : files[i].lrbc);
              unsigned long sb = exact_size (files[j].records,
                                             ignore_lrbc ? 0 : files[j].lrbc);
              if (sa > sb)
                {
                  c = 1;
                }
              else if (sa < sb)
                {
                  c = -1;
                }
              else
                {
                  c = cmp_name (&files[i], &files[j]);
                }
            }
          else
            {
              c = cmp_name (&files[i], &files[j]);
            }

          if ((!reverse && c > 0) || (reverse && c < 0))
            {
              t = files[i];
              files[i] = files[j];
              files[j] = t;
            }
        }
    }
}

/*****************************************************************************/

static void
print_name (const struct fileent *f, int pack)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      if (pack && f->name[i] == ' ')
        {
          break;
        }

      putch (f->name[i] & 0x7f);
    }

  if (f->typ[0] == ' ' && f->typ[1] == ' ' && f->typ[2] == ' ')
    {
      if (!pack)
        {
          putch (' ');
        }
    }
  else
    {
      putch ('.');
    }

  for (i = 0; i < 3; i++)
    {
      if (pack && f->typ[i] == ' ')
        {
          break;
        }

      putch (f->typ[i] & 0x7f);
    }

  if (!pack)
    {
      /* pad name.typ to fixed 12 visible columns roughly */
      /* already printed variable; for non-pack fill to 12 chars */
    }
}

/*****************************************************************************/

static void
print_name_fixed (const struct fileent *f)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      putch ((f->name[i] && f->name[i] != ' ') ? (f->name[i] & 0x7f) : ' ');
    }

  putch ((f->typ[0] != ' ' || f->typ[1] != ' ' || f->typ[2] != ' ') ? '.'
                                                                    : ' ');
  for (i = 0; i < 3; i++)
    {
      putch ((f->typ[i] && f->typ[i] != ' ') ? (f->typ[i] & 0x7f) : ' ');
    }
}

/*****************************************************************************/

static int
getch_wait (void)
{
  int c;

  while (!(c = (int)bdos (6, 0xFF)))
    {
      ;
    }

  /* drain typeahead CR/LF only */
  while (bdos (11, 0))
    {
      int d = (int)bdos (6, 0xFF) & 0xff;

      if (d != '\r' && d != '\n' && d != 0)
        {
          break;
        }
    }

  return c & 0xff;
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: LS [-h] [-a] [-p] [-s|-z] [-r] [-i] [-l|-b]");
  puts (" [filespec ...]\r\n");
  puts ("  -a  all files (including system)\r\n");
  puts ("  -b  bare names only\r\n");
  puts ("  -h  show this help text\r\n");
  puts ("  -i  ignore Last Record Byte Count\r\n");
  puts ("  -l  long listing\r\n");
  puts ("  -p  pause each screen\r\n");
  puts ("  -r  reverse sort order\r\n");
  puts ("  -s  sort by name\r\n");
  puts ("  -z  sort by size\r\n");
}

/*****************************************************************************/

static int
toupper_ch (int c)
{
  if (c >= 'a' && c <= 'z')
    {
      return c - 32;
    }

  return c;
}

/*****************************************************************************/

/* Build search FCB from pattern text (e.g. *.386 or HELLO.*) */
static void
pattern_to_fcb (UBYTE *fcb, const char *pat)
{
  int i, ni = 0, ti = 0, in_typ = 0;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  fcb[0] = 0; /* default drive */

  for (i = 1; i <= 11; i++)
    {
      fcb[i] = ' ';
    }

  while (*pat == ' ' || *pat == '\t')
    {
      pat++;
    }

  /* optional n/ prefix for user */
  /* optional d: drive */
  if (pat[0] && pat[1] == ':')
    {
      int d = toupper_ch ((unsigned char)pat[0]);

      if (d >= 'A' && d <= 'P')
        {
          fcb[0] = (UBYTE)(d - 'A' + 1);
        }

      pat += 2;
    }

  while (*pat && *pat != ' ' && *pat != '\t' && *pat != '\r')
    {
      char c = (char)toupper_ch ((unsigned char)*pat++);

      if (c == '.')
        {
          in_typ = 1;
          ti = 0;

          continue;
        }

      if (c == '*')
        {
          if (!in_typ)
            {
              while (ni < 8)
                {
                  fcb[1 + ni++] = '?';
                }
            }
          else
            {
              while (ti < 3)
                {
                  fcb[9 + ti++] = '?';
                }
            }

          continue;
        }

      if (c == '?')
        {
          if (!in_typ && ni < 8)
            {
              fcb[1 + ni++] = '?';
            }
          else if (in_typ && ti < 3)
            {
              fcb[9 + ti++] = '?';
            }

          continue;
        }

      if (!in_typ && ni < 8)
        {
          fcb[1 + ni++] = c;
        }
      else if (in_typ && ti < 3)
        {
          fcb[9 + ti++] = c;
        }
    }

  /* bare name with no type => * type? original uses *.* default */
  if (!in_typ && ni == 0)
    {
      for (i = 0; i < 11; i++)
        {
          fcb[1 + i] = '?';
        }
    }
  else if (!in_typ)
    {
      /* name only: any type */
      for (i = 0; i < 3; i++)
        {
          fcb[9 + i] = '?';
        }
    }
}

/*****************************************************************************/

static void
search_pattern (UBYTE *fcb, UBYTE *dma, int user, int word_mode)
{
  UWORD r;

  bdos (26, (LONG)(unsigned long)dma);

  if (fcb[0])
    {
      bdos (14, (LONG)(fcb[0] - 1));
    }

  fcb[12] = '?'; /* all extents */
  fcb[14] = '?'; /* all modules */

  r = bdos (17, (LONG)(unsigned long)fcb);

  while (r != 255)
    {
      const UBYTE *de = dma + (r * 32);

      /* current user only (search may return other users with ?) */
      if (de[0] == (UBYTE)user)
        {
          add_dirent (de, word_mode);
        }

      r = bdos (18, 0);
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb[36];
  UBYTE dma[128];
  UBYTE dpb[32];
  char tail[128];
  unsigned tlen, i;
  int flag_all = 0, flag_mode = 0, flag_pause = 0;
  int sort_by = 0, reverse_sort = 0, ignore_lrbc = 0;
  int user, drive;
  unsigned block_size = 2048;
  int word_mode = 0;
  unsigned long all_exact = 0, all_alloc_k = 0;
  int count = 0, ctr = 0, process = 1;
  char *pats[MAX_PATS];
  int npats = 0;

  /* Parse command tail */
  tlen = CMD_TAIL[0];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      tail[i] = (char)CMD_TAIL[1 + i];
    }

  tail[tlen] = 0;

  i = 0;

  while (tail[i] == ' ' || tail[i] == '\t')
    {
      i++;
    }
  while (tail[i])
    {
      if (tail[i] == '-' || tail[i] == '/')
        {
          i++;

          while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
            {
              char c = (char)toupper_ch ((unsigned char)tail[i++]);

              switch (c)
                {
                case 'H':
                  help ();

                  bdos (0, 0);

                  break;

                case 'A':
                  flag_all = 1;

                  break;

                case 'S':
                  sort_by = 1;

                  break;

                case 'Z':
                  sort_by = 2;

                  break;

                case 'R':
                  reverse_sort = 1;

                  break;

                case 'I':
                  ignore_lrbc = 1;

                  break;

                case 'B':
                  flag_mode = 2;

                  break;

                case 'L':
                  flag_mode = 1;

                  break;

                case 'P':
                  flag_pause = 1;

                  break;

                default:
                  help ();
                  puts ("ERR: Wrong parameters\r\n");

                  bdos (0, 0);
                }
            }
          while (tail[i] == ' ' || tail[i] == '\t')
            {
              i++;
            }

          continue;
        }

      if (npats < MAX_PATS)
        {
          pats[npats++] = &tail[i];
        }

      while (tail[i] && tail[i] != ' ' && tail[i] != '\t')
        {
          i++;
        }

      if (tail[i])
        {
          tail[i++] = 0;
        }

      while (tail[i] == ' ' || tail[i] == '\t')
        {
          i++;
        }
    }

  user = (int)bdos (32, 0xFF);
  drive = (int)bdos (25, 0);

  /* DPB for block size / word map mode */
  bdos (31, (LONG)(unsigned long)dpb);
  /* dpb: spt@0, bsh@2, blm@3, exm@4, dum@5, dsm@6 */
  {
    UBYTE bsh = dpb[2];
    UWORD dsm = (UWORD)dpb[6] | ((UWORD)dpb[7] << 8);
    block_size = 128u << bsh;
    word_mode = (dsm > 255) ? 1 : 0;
  }

  /*
   * Collect directory entries - extent '?' so all extents of multi-extent
   * files are returned (needed for correct record totals + last LRBC).
   */

  nfiles = 0;

  if (npats == 0)
    {
      if (DEF_FCB[1] != ' ' && DEF_FCB[1] != '?' && DEF_FCB[1] != 0)
        {
          for (i = 0; i < 36; i++)
            {
              fcb[i] = DEF_FCB[i];
            }

          if (fcb[9] == ' ' && fcb[10] == ' ' && fcb[11] == ' ')
            {
              fcb[9] = fcb[10] = fcb[11] = '?';
            }
        }
      else
        {
          pattern_to_fcb (fcb, "*.*");
        }

      if (fcb[0])
        {
          drive = fcb[0] - 1;
        }

      search_pattern (fcb, dma, user, word_mode);
    }
  else
    {
      int p;

      for (p = 0; p < npats; p++)
        {
          pattern_to_fcb (fcb, pats[p]);

          if (fcb[0])
            {
              drive = fcb[0] - 1;
            }

          search_pattern (fcb, dma, user, word_mode);
        }
    }

  sort_files (sort_by, reverse_sort, ignore_lrbc);

  if (flag_mode != 2)
    {
      puts ("Drive ");
      putu ((unsigned)user);
      putch ((char)('A' + drive));
      puts (":\r\n");
      ctr++;
    }

  for (i = 0; i < (unsigned)nfiles && process; i++)
    {
      struct fileent *f = &files[i];
      unsigned long exact, exact_k, alloc_k;

      if (f->sys && !flag_all)
        {
          continue;
        }

      /* Ctrl-C poll when not pausing */
      if (!flag_pause)
        {
          while (bdos (11, 0))
            {
              int d = (int)bdos (1, 0);

              if (d == 3)
                {
                  process = 0;

                  break;
                }
            }

          if (!process)
            {
              break;
            }
        }

      exact = exact_size (f->records, ignore_lrbc ? 0 : f->lrbc);
      exact_k = (exact + 1023UL) / 1024UL;
      alloc_k = (f->blocks * (unsigned long)block_size) / 1024UL;
      all_exact += exact;
      all_alloc_k += alloc_k;
      count++;

      if (flag_mode == 1)
        {
          /* long: attrs, name, exact bytes, exact K, FCBs, blocks */
          putch (f->sys ? 'S' : '-');
          putch (f->ro ? 'R' : '-');
          putch (' ');
          print_name_fixed (f);
          putch (' ');
          putu_w (exact, 7);
          puts ("b ");
          putu_w (exact_k, 4);
          puts ("K ");
          putu_w (f->fcbs, 3);
          puts (" FCB ");
          putu_w (f->blocks, 4);
          puts (" Blk\r\n");
          ctr++;
        }
      else if (flag_mode == 2)
        {
          print_name (f, 1);
          puts ("\r\n");
          ctr++;
        }
      else
        {
          /* default: 4 columns, exact size in K */
          putch (f->sys ? '*' : ' ');
          print_name_fixed (f);
          putu_w (exact_k, 5);
          putch ('K');

          if (count % 4 != 0)
            {
              putch (',');
            }

          if (count % 4 == 0)
            {
              puts ("\r\n");
              ctr++;
            }
        }

      if (!flag_mode && count % 4 != 0 && i + 1 >= (unsigned)nfiles)
        {
          puts ("\r\n");
          ctr++;
        }

      if (ctr > 22 || (ctr > 21 && i + 1 >= (unsigned)nfiles))
        {
          ctr = 0;

          if (flag_pause)
            {
              int d, wt = 1;

              puts ("[More]");

              while (wt)
                {
                  d = getch_wait ();

                  switch (d)
                    {
                    case 'q':
                    case 'Q':
                    case 3:
                      puts ("\r\n");
                      wt = 0;
                      process = 0;

                      break;

                    case 13: /* CR: one line */
                      puts ("\r      \r");
                      wt = 0;
                      ctr = 22;

                      break;

                    case 32: /* space: next page */
                      puts ("\r      \r");
                      wt = 0;
                      ctr = 0;

                      break;

                    default:
                      break;
                    }
                }
            }
        }
    }

  if (flag_mode < 2)
    {
      putu ((unsigned)count);
      puts (" File(s) ");
      putu ((all_exact + 1023UL) / 1024UL);
      puts ("K exact, ");
      putu (all_alloc_k);
      puts ("K alloc\r\n");
    }

  bdos (0, 0);
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
