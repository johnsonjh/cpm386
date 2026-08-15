/*
 * CP/M-386 - cleartpa.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: cca7b4c6-856a-11f1-a3fd-80ee73e9b8e7
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned long ULONG;

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
  ULONG low;
  ULONG high;
};

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
putu (ULONG n)
{
  char b [12];
  int i = 0;

  if (!n)
    {
      putch ('0');

      return;
    }

  while (n && i < 12)
    {
      b [i++] = (char)('0' + n % 10);
      n /= 10;
    }
  while (i)
    {
      putch (b [--i]);
    }
}

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct tpa_req t;
  ULONG esp;
  ULONG i;
  int verify = 0;
  const char *tail = (char *)CMD_TAIL;
  int tail_len = tail [0];

  /* parse args */
  for (i = 1; i <= tail_len; i++)
    {
      if (tail [i] == '-')
        {
          if (tail [i + 1] == 'h' || tail [i + 1] == 'H')
            {
              puts ("Usage: CLEARTPA [-h] [-v]\r\n");
              puts ("  -h   Show this help\r\n");
              puts ("  -v   Verify the TPA after clearing\r\n");

              (void)bdos (0, 0);
            }

          if (tail [i + 1] == 'v' || tail [i + 1] == 'V')
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
  (void)bdos (BDOS_TPA, (LONG)(ULONG)&t);

  ULONG start_addr1 = (ULONG)&_end;
  start_addr1 = (start_addr1 + 3) & ~3;
  ULONG end_addr1 = esp - 16384;
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

  ULONG start_addr2 = esp + 16384;
  start_addr2 = (start_addr2 + 3) & ~3;
  ULONG end_addr2 = t.high;
  end_addr2 &= ~3;

  if (start_addr2 < t.low)
    {
      start_addr2 = t.low;
    }

  if (start_addr2 > end_addr2)
    {
      start_addr2 = end_addr2;
    }

  ULONG total_k
      = ((end_addr1 - start_addr1) + (end_addr2 - start_addr2)) / 1024;

  ULONG cleared_bytes = 0;
  ULONG chunk;

  volatile ULONG *p1 = (volatile ULONG *)start_addr1;
  ULONG count1 = (end_addr1 - start_addr1) / 4;
  ULONG rem1 = count1;

  puts ("Clearing: 0K");

  while (rem1 > 0)
    {
      chunk = rem1 > 25600 ? 25600 : rem1;

      for (i = 0; i < chunk; i++)
        {
          *p1++ = 0;
        }

      rem1 -= chunk;
      cleared_bytes += chunk * 4;

      puts ("\rClearing: ");
      putu (cleared_bytes / 1024);
      puts ("K");
    }

  volatile ULONG *p2 = (volatile ULONG *)start_addr2;
  ULONG count2 = (end_addr2 - start_addr2) / 4;
  ULONG rem2 = count2;

  while (rem2 > 0)
    {
      chunk = rem2 > 25600 ? 25600 : rem2;

      for (i = 0; i < chunk; i++)
        {
          *p2++ = 0;
        }

      rem2 -= chunk;
      cleared_bytes += chunk * 4;

      puts ("\rClearing: ");
      putu (cleared_bytes / 1024);
      puts ("K");
    }

  puts ("\rClearing: ");
  putu (total_k);
  puts ("K\r\n");

  if (verify)
    {
      ULONG verified_bytes = 0;
      puts ("Verify: 0K");
      int failed = 0;

      p1 = (volatile ULONG *)start_addr1;
      rem1 = count1;

      while (rem1 > 0 && !failed)
        {
          chunk = rem1 > 25600 ? 25600 : rem1;

          for (i = 0; i < chunk; i++)
            {
              if (*p1++ != 0)
                {
                  failed = 1;

                  break;
                }
            }

          if (failed)
            break;

          rem1 -= chunk;
          verified_bytes += chunk * 4;

          puts ("\rVerify: ");
          putu (verified_bytes / 1024);
          puts ("K");
        }

      p2 = (volatile ULONG *)start_addr2;
      rem2 = count2;

      while (rem2 > 0 && !failed)
        {
          chunk = rem2 > 25600 ? 25600 : rem2;

          for (i = 0; i < chunk; i++)
            {
              if (*p2++ != 0)
                {
                  failed = 1;

                  break;
                }
            }

          if (failed)
            break;

          rem2 -= chunk;
          verified_bytes += chunk * 4;

          puts ("\rVerify: ");
          putu (verified_bytes / 1024);
          puts ("K");
        }

      if (failed)
        {
          puts ("\r\nFailure!\r\n");
        }
      else
        {
          puts ("\rVerify: ");
          putu (total_k);
          puts ("K\r\nSuccess!\r\n");
        }
    }
  else
    {
      puts ("Success!\r\n");
    }

  (void)bdos (0, 0);
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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
