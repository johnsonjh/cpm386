/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: a060f59c-82b5-11f1-b974-80ee73e9b8e7
 */

/*****************************************************************************/

/* rm.c: delete files */

/*****************************************************************************/

/*
 * Usage: RM [-h] | [-a][-i][-f] filespec [filespec ...]
 *
 *   -a  include system files
 *   -i  confirm each delete (Y/N)
 *   -f  force: clear R/O then delete
 *
 * BDOS only (int 0x30).  Multi-extent names appear once (ex/s2 search = '?').
 */

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
#define MAX_NAMES 64
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
                    : "a"((unsigned)func),
                      "i"(BDOS_INT),
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

static int
getch_wait (void)
{
  int c;

  while (!(c = (int)bdos (6, 0xFF)))
    {
      ;
    }

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
  puts ("Usage: RM [-h] [-a] [-i] [-f] filespec [filespec ...]\r\n");
  puts ("  -a  include system files\r\n");
  puts ("  -f  force delete of R/O files\r\n");
  puts ("  -h  show this help text\r\n");
  puts ("  -i  confirm each file\r\n");
}

/*****************************************************************************/

/* Collect unique 11-char names (+ sys/ro flags) matching pattern */
struct ent
{
  char name[11];
  UBYTE sys;
  UBYTE ro;
  UBYTE used;
};

/*****************************************************************************/

static struct ent ents[MAX_NAMES];
static int nents;

/*****************************************************************************/

static int
name_eq (const struct ent *e, const UBYTE *de)
{
  int i;

  for (i = 0; i < 11; i++)
    {
      if ((e->name[i] & 0x7f) != (de[1 + i] & 0x7f))
        {
          return 0;
        }
    }

  return 1;
}

/*****************************************************************************/

static void
add_ent (const UBYTE *de)
{
  int i, idx = -1;

  if (de[0] >= 16)
    {
      return;
    }

  for (i = 0; i < nents; i++)
    {
      if (ents[i].used && name_eq (&ents[i], de))
        {
          idx = i;

          break;
        }
    }

  if (idx < 0)
    {
      if (nents >= MAX_NAMES)
        {
          return;
        }

      idx = nents++;

      for (i = 0; i < 11; i++)
        {
          ents[idx].name[i] = (char)(de[1 + i] & 0x7f);
        }

      ents[idx].sys = 0;
      ents[idx].ro = 0;
      ents[idx].used = 1;
    }

  if (de[9] & 0x80)
    {
      ents[idx].ro = 1;
    }

  if (de[10] & 0x80)
    {
      ents[idx].sys = 1;
    }
}

/*****************************************************************************/

static void
pattern_to_fcb (UBYTE *fcb, const char *pat)
{
  int i, ni = 0, ti = 0, in_typ = 0;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  for (i = 1; i <= 11; i++)
    {
      fcb[i] = ' ';
    }

  while (*pat == ' ' || *pat == '\t')
    {
      pat++;
    }

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

  if (!in_typ && ni == 0)
    {
      for (i = 0; i < 11; i++)
        {
          fcb[1 + i] = '?';
        }
    }
  else if (!in_typ)
    {
      for (i = 0; i < 3; i++)
        {
          fcb[9 + i] = '?';
        }
    }
}

/*****************************************************************************/

static void
print_name (const struct ent *e)
{
  int i, any = 0;

  for (i = 0; i < 8; i++)
    {
      if (e->name[i] == ' ')
        {
          break;
        }

      putch (e->name[i] & 0x7f);
      any = 1;
    }

  if (e->name[8] != ' ' || e->name[9] != ' ' || e->name[10] != ' ')
    {
      putch ('.');

      for (i = 8; i < 11; i++)
        {
          if (e->name[i] == ' ')
            {
              break;
            }

          putch (e->name[i] & 0x7f);
        }
    }
  else if (!any)
    {
      puts ("?");
    }
}

/*****************************************************************************/

static void
fill_del_fcb (UBYTE *fcb, const struct ent *e)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  for (i = 0; i < 11; i++)
    {
      fcb[1 + i] = (UBYTE)(e->name[i] & 0x7f);
    }
}

/*****************************************************************************/

static void
search_pattern (UBYTE *fcb, UBYTE *dma, int user)
{
  UWORD r;

  bdos (26, (LONG)(unsigned long)dma);

  if (fcb[0])
    {
      bdos (14, (LONG)(fcb[0] - 1));
    }

  fcb[12] = '?';
  fcb[14] = '?';

  r = bdos (17, (LONG)(unsigned long)fcb);

  while (r != 255)
    {
      UBYTE const *de = dma + (r * 32);

      if (de[0] == (UBYTE)user)
        {
          add_ent (de);
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
  char tail[128];
  unsigned tlen, i;
  int flag_all = 0, flag_ask = 0, flag_force = 0;
  char *pats[MAX_PATS];
  int npats = 0;
  UWORD r;
  int user;

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

                case 'I':
                  flag_ask = 1;

                  break;

                case 'F':
                  flag_force = 1;

                  break;

                default:
                  puts ("ERROR: Wrong parameters\r\n");
                  help ();

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
  nents = 0;

  if (npats == 0)
    {
      /* fall back to default FCB from CCP */
      if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
        {
          puts ("ERROR: No filespec\r\n");
          help ();

          bdos (0, 0);
        }

      for (i = 0; i < 36; i++)
        {
          fcb[i] = DEF_FCB[i];
        }

      if (fcb[9] == ' ' && fcb[10] == ' ' && fcb[11] == ' ')
        {
          fcb[9] = fcb[10] = fcb[11] = '?';
        }

      search_pattern (fcb, dma, user);
    }
  else
    {
      int p;

      for (p = 0; p < npats; p++)
        {
          pattern_to_fcb (fcb, pats[p]);
          search_pattern (fcb, dma, user);
        }
    }

  if (nents == 0)
    {
      puts ("No file found\r\n");

      bdos (0, 0);
    }

  for (i = 0; i < (unsigned)nents; i++)
    {
      struct ent const *e = &ents[i];
      UBYTE dfcb[36];
      int do_del = 1;

      if (e->sys && !flag_all)
        {
          continue;
        }

      if (e->ro && !flag_force)
        {
          continue;
        }

      puts ("Deleting ");
      print_name (e);
      putch (' ');

      if (flag_ask)
        {
          puts ("(Y/N)");

          for (;;)
            {
              int c = getch_wait ();

              if (c == 'y' || c == 'Y')
                {
                  puts ("\r      \rDeleting ");
                  print_name (e);
                  putch (' ');
                  do_del = 1;

                  break;
                }

              if (c == 'n' || c == 'N')
                {
                  puts ("\r      \rSkipping ");
                  print_name (e);
                  puts ("      \r\n");
                  do_del = 0;

                  break;
                }

              if (c == 3)
                {
                  puts ("\r      \r^C       \r\n");

                  bdos (0, 0);
                }
            }
        }

      if (!do_del)
        {
          continue;
        }

      fill_del_fcb (dfcb, e);

      if (e->ro && flag_force)
        {
          /* clear R/O attribute then delete */
          dfcb[9] = (UBYTE)(e->name[0] & 0x7f); /* rebuild clean */

          {
            int j;

            for (j = 0; j < 11; j++)
              {
                dfcb[1 + j] = (UBYTE)(e->name[j] & 0x7f);
              }
          }

          bdos (30, (LONG)(unsigned long)dfcb); /* set attrs clear high bits */
        }

      r = bdos (19, (LONG)(unsigned long)dfcb);

      if (r == 255)
        {
          puts ("Failed\r\n");
        }
      else
        {
          puts ("OK   \r\n");
        }
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
