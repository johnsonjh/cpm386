/*
 * CP/M-386 - fparse.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: b11f5db6-82b4-11f1-8dd8-80ee73e9b8e7
 */

/*****************************************************************************/

/* fparse.c - exercise BDOS 152 F_PARSE and 163 S_OSVER */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_FPARSE 152
#define BDOS_OSVER 163

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

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

static void
putch (char c)
{
  (void)bdos (2, (LONG)(unsigned char)c);
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
puthex2 (unsigned v)
{
  static const char h [] = "0123456789ABCDEF";

  putch (h [(v >> 4) & 0xf]);
  putch (h [v & 0xf]);
}

/*****************************************************************************/

static void
puthex4 (UWORD v)
{
  puthex2 (v >> 8);
  puthex2 (v & 0xff);
}

/*****************************************************************************/

static void
store_le32 (UBYTE *p, ULONG v)
{
  p [0] = (UBYTE)(v & 0xff);
  p [1] = (UBYTE)((v >> 8) & 0xff);
  p [2] = (UBYTE)((v >> 16) & 0xff);
  p [3] = (UBYTE)((v >> 24) & 0xff);
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char line [96];
  UBYTE fcb [36] = { 0 };
  UBYTE pfcb [8];
  UWORD r, osv;
  unsigned int tlen, i;
  char *src;

  tlen = CMD_TAIL [0];

  if (tlen > 80)
    {
      tlen = 80;
    }

  if (tlen > 0)
    {
      unsigned int j;

      for (i = 0; i < tlen; i++)
        {
          line [i] = (char)CMD_TAIL [1 + i];
        }

      line [tlen] = 0;
      j = 0;

      while (line [j] == ' ' || line [j] == '\t')
        {
          j++;
        }
      src = line + j;
    }
  else
    {
      const char *d = "B:HELLO.386;SECRET  rest";

      for (i = 0; d [i]; i++)
        {
          line [i] = d [i];
        }

      line [i] = 0;
      src = line;
    }

  osv = bdos (BDOS_OSVER, 0);
  puts ("S_OSVER=");
  puthex4 (osv);
  puts ("\r\n");

  puts ("Parse: \"");
  puts (src);
  puts ("\"\r\n");

  store_le32 (pfcb + 0, (ULONG)(UBYTE *)src);
  store_le32 (pfcb + 4, (ULONG)(UBYTE *)fcb);

  r = bdos (BDOS_FPARSE, (LONG)(ULONG)pfcb);
  puts ("F_PARSE ret=");
  puthex4 (r);
  puts ("\r\n");

  if (r == 0xFFFF)
    {
      puts ("INVALID\r\n");

      (void)bdos (0, 0);
    }

  puts ("drive=");
  puthex2 (fcb [0]);
  puts (" name=");

  for (i = 1; i <= 8; i++)
    {
      putch (fcb [i] ? (char)fcb [i] : ' ');
    }

  puts (" typ=");

  for (i = 9; i <= 11; i++)
    {
      putch (fcb [i] ? (char)fcb [i] : ' ');
    }

  if (fcb [0x1A])
    {
      puts (" pw=");

      for (i = 0; i < fcb [0x1A] && i < 8; i++)
        {
          putch ((char)fcb [0x10 + i]);
        }

      puts (" (len=");
      puthex2 (fcb [0x1A]);
      puts (")");
    }

  puts ("\r\n");

  if (r == 0)
    {
      puts ("terminated (NUL/CR)\r\n");
    }
  else
    {
      /* ret is byte offset into the source string */
      puts ("next +");
      puthex4 (r);
      puts ("='");
      putch (src [r]);
      puts ("'\r\n");
    }

  (void)bdos (0, 0);
}

/*****************************************************************************/

#if defined(__clang__) || defined(__clang_version__)
void *
memset(void *s, int c, unsigned long n)
{
  unsigned char *p = s;

  while (n--)
    *p++ = (unsigned char)c;

  return s;
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
