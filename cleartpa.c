/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: cca7b4c6-856a-11f1-a3fd-80ee73e9b8e7
 */

/*****************************************************************************/

/* XXX: CLEARTPA fails when the TPA is >3.4GB - need to fix this bug! */
/* TODO: Show progress as the TPA is cleared and verified */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_TPA 63

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)0x80)

/*****************************************************************************/

extern char _end;

/*****************************************************************************/

struct tpa_req
{
  UWORD parms;
  UWORD _pad;
  unsigned long low;
  unsigned long high;
};

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

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct tpa_req t;
  unsigned long esp;
  unsigned long i;
  /* unsigned long start_addr, end_addr; */
  int verify = 0;
  const char *tail = (char *)CMD_TAIL;
  int tail_len = tail[0];

  /* parse args */
  for (i = 1; i <= tail_len; i++)
    {
      if (tail[i] == '-')
        {
          if (tail[i + 1] == 'h' || tail[i + 1] == 'H')
            {
              puts ("Usage: CLEARTPA [-h] [-v]\r\n");
              puts ("  -h   Show this help\r\n");
              puts ("  -v   Verify the TPA after clearing\r\n");

              bdos (0, 0);
            }

          if (tail[i + 1] == 'v' || tail[i + 1] == 'V')
            {
              verify = 1;
            }
        }
    }

  __asm__ volatile ("mov %%esp, %0" : "=r"(esp));

  t.parms = 0;
  t._pad = 0;
  t.low = 0;
  t.high = 0;
  bdos (BDOS_TPA, (LONG)(unsigned long)&t);

  unsigned long start_addr1 = (unsigned long)&_end;
  start_addr1 = (start_addr1 + 3) & ~3;
  unsigned long end_addr1 = esp - 16384;
  end_addr1 &= ~3;

  if (end_addr1 > t.high)
    {
      end_addr1 = t.high;
    }

  if (start_addr1 < t.low)
    {
      start_addr1 = t.low;
    }

  if (start_addr1 > end_addr1)
    {
      start_addr1 = end_addr1;
    }

  unsigned long start_addr2 = esp + 16384;
  start_addr2 = (start_addr2 + 3) & ~3;
  unsigned long end_addr2 = t.high;
  end_addr2 &= ~3;

  if (start_addr2 < t.low)
    {
      start_addr2 = t.low;
    }

  if (start_addr2 > end_addr2)
    {
      start_addr2 = end_addr2;
    }

  unsigned long total_k
      = ((end_addr1 - start_addr1) + (end_addr2 - start_addr2)) / 1024;

  puts ("Clearing: ");
  putu (total_k);
  puts ("K\r\n");

  volatile unsigned long *p1 = (volatile unsigned long *)start_addr1;
  unsigned long count1 = (end_addr1 - start_addr1) / 4;
  for (i = 0; i < count1; i++)
    {
      p1[i] = 0;
    }

  volatile unsigned long *p2 = (volatile unsigned long *)start_addr2;
  unsigned long count2 = (end_addr2 - start_addr2) / 4;
  for (i = 0; i < count2; i++)
    {
      p2[i] = 0;
    }

  if (verify)
    {
      puts ("Verify: ");
      putu (total_k);
      puts ("K\r\n");
      int failed = 0;
      for (i = 0; i < count1; i++)
        {
          if (p1[i] != 0)
            {
              failed = 1;
              break;
            }
        }

      for (i = 0; i < count2; i++)
        {
          if (p2[i] != 0)
            {
              failed = 1;
              break;
            }
        }

      if (failed)
        {
          puts ("Failure!\r\n");
        }
      else
        {
          puts ("Success!\r\n");
        }
    }
  else
    {
      puts ("Success!\r\n");
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
