/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 853dc088-8e95-11f1-9727-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * prng.c - PRNG test program
 *
 * Usage:
 *   PRNG          print 2 random bytes as hex
 *   PRNG N        print N random bytes as hex
 *   PRNG -h       help
 *   PRNG -s       re-seed the RNG from PIT jitter
 *   PRNG -s HEX   re-seed with a 64-char hex string (32 bytes / 256 bits)
 */

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
#define BDOS_RNG_GET 253
#define BDOS_RNG_SEED 254
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

/*****************************************************************************/

/* Seed block for BDOS 254 - must match struct cpm_rng_seed in cpmrng.h. */

struct rng_seed
{
  ULONG data; /* TPA-relative pointer to seed bytes */
  ULONG len;  /* byte count, 1..64                  */
};

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

static const char hex_digits[] = "0123456789ABCDEF";

/*****************************************************************************/

static void
puthex8 (UBYTE v)
{
  putch (hex_digits[(v >> 4) & 0x0F]);
  putch (hex_digits[v & 0x0F]);
}

/*****************************************************************************/

static void
putu (ULONG n)
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

static int
hexval (char c)
{
  if (c >= '0' && c <= '9')
    {
      return c - '0';
    }

  if (c >= 'A' && c <= 'F')
    {
      return c - 'A' + 10;
    }

  if (c >= 'a' && c <= 'f')
    {
      return c - 'a' + 10;
    }

  return -1;
}

/*****************************************************************************/

static void
help (void)
{
  puts ("PRNG: Salsa20-based PRNG utility (BDOS 253/254)\r\n");
  puts ("Usage:\r\n");
  puts ("  PRNG          print 2 random bytes (as hex)\r\n");
  puts ("  PRNG N        print N random bytes (N=1..32767)\r\n");
  puts ("  PRNG -h       print this help text\r\n");
  puts ("  PRNG -s       re-seed RNG (using timer jitter)\r\n");
  puts ("  PRNG -s HEX   re-seed RNG (using 64-char hex string)\r\n");
}

/*****************************************************************************/

static int
get_tail (char *dst, int max)
{
  int tlen, i;
  const UBYTE *src;

  tlen = CMD_TAIL[0];

  if (tlen > 126)
    {
      tlen = 126;
    }

  if (tlen > max - 1)
    {
      tlen = max - 1;
    }

  src = &CMD_TAIL[1];

  for (i = 0; i < tlen; i++)
    {
      dst[i] = (char)src[i];
    }

  dst[tlen] = 0;

  return tlen;
}

/*****************************************************************************/

static const char *
skip_ws (const char *p)
{
  while (*p == ' ' || *p == '\t')
    {
      p++;
    }

  return p;
}

/*****************************************************************************/

static int
parse_uint (const char *p, unsigned long *out)
{
  unsigned long n = 0;
  int count = 0;

  while (*p >= '0' && *p <= '9')
    {
      n = n * 10 + (unsigned long)(*p - '0');
      p++;
      count++;
    }

  if (count)
    {
      *out = n;
    }

  return count;
}

/*****************************************************************************/

static void
seed_from_pit (void)
{
  UBYTE buf[32];
  struct rng_seed sb;
  UWORD r;

  puts ("Seeding RNG ...\r\n");

  {
    int i;

    struct
    {
      ULONG lo;
      ULONG hi; /*cppcheck-suppress unusedStructMember*/
      ULONG hz; /*cppcheck-suppress unusedStructMember*/
    } ticks;

    for (i = 0; i < 32; i++)
      {
        volatile int j;

        for (j = 0; j < (i * 17 + 31); j++)
          {
            __asm__ volatile ("nop");
          }

        (void)bdos (225, (LONG)(ULONG)&ticks);

        buf[i]  = (UBYTE)(ticks.lo & 0xFF);
        buf[i] ^= (UBYTE)((ticks.lo >> 8) & 0xFF);
        buf[i] ^= (UBYTE)(i);
      }
  }

  sb.data = (ULONG)&buf[0];
  sb.len = 32;
  r = bdos (BDOS_RNG_SEED, (LONG)(ULONG)&sb);

  if (r == 0xFFFF)
    {
      puts ("Seeding failed!\r\n");
    }
  else
    {
      puts ("Seeded (256 bits).\r\n");
    }
}

/*****************************************************************************/

static void
seed_from_hex (const char *hex)
{
  UBYTE buf[32];
  struct rng_seed sb;
  int i;
  UWORD r;

  for (i = 0; i < 32; i++)
    {
      int h = hexval (hex[i * 2]);
      int l = hexval (hex[i * 2 + 1]);

      if (h < 0 || l < 0)
        {
          puts ("Error: invalid hex character at position ");
          putu ((ULONG)(i * 2 + (h < 0 ? 0 : 1)));
          puts ("\r\n");

          return;
        }

      buf[i] = (UBYTE)((h << 4) | l);
    }

  sb.data = (ULONG)&buf[0];
  sb.len = 32;
  r = bdos (BDOS_RNG_SEED, (LONG)(ULONG)&sb);

  if (r == 0xFFFF)
    {
      puts ("Seeding failed!\r\n");
    }
  else
    {
      puts ("Seeded (256 bits) from user hex string.\r\n");
    }
}

/*****************************************************************************/

static void
emit_random (unsigned long count)
{
  unsigned long i;
  int col = 0;

  for (i = 0; i < count; i++)
    {
      UWORD r = bdos (BDOS_RNG_GET, 0);

      if (r == 0xFFFF)
        {
          if (i == 0)
            {
              puts ("ERROR: PRNG initial state is unseeded:\r\n");
              puts ("  Use PRNG -s to seed from PIT jitter,\r\n");
              puts ("  or  PRNG -s <64-hex-chars> to seed manually.\r\n");
            }
          else
            {
              puts ("\r\nERROR: PRNG is unseeded; aborting!\r\n");
            }

          return;
        }

      puthex8 ((UBYTE)(r >> 8));
      col++;

      if (col >= 32)
        {
          puts ("\r\n");
          col = 0;
        }

      if (i + 1 > count)
        {
          break;
        }

      i++;

      if (i < count)
        {
          puthex8 ((UBYTE)(r & 0xFF));
          col++;

          if (col >= 32)
            {
              puts ("\r\n");
              col = 0;
            }
        }
    }

  if (col > 0)
    {
      puts ("\r\n");
    }
}

/*****************************************************************************/

void
_start (void) /* cppcheck-suppress unusedFunction */
{
  char tail[128];
  const char *p;
  unsigned long count;

  (void)get_tail (tail, sizeof (tail));
  p = skip_ws (tail);

  if (*p == 0)
    {
      emit_random (2);

      (void)bdos (0, 0);
    }

  /* help */
  if ((*p == '-' || *p == '/') && (p[1] == 'h' || p[1] == 'H'))
    {
      help ();

      (void)bdos (0, 0);
    }

  /* seed */
  if ((*p == '-' || *p == '/') && (p[1] == 's' || p[1] == 'S'))
    {
      p = skip_ws (p + 2);

      if (*p == 0)
        {
          seed_from_pit ();
        }
      else
        {
          int hlen = 0;
          const char *q = p;

          while (hexval (*q) >= 0)
            {
              hlen++;
              q++;
            }

          if (hlen != 64)
            {
              puts ("ERROR: seed must be exactly 64 hex chars");
              puts (" (32 bytes / 256 bits).\r\n");
              puts ("Got ");
              putu ((ULONG)hlen);
              puts (" hex chars.\r\n");
              help ();
            }
          else
            {
              seed_from_hex (p);
            }
        }

      (void)bdos (0, 0);
    }

  /* emit COUNT random bytes */
  if (parse_uint (p, &count) > 0)
    {
      if (count == 0)
        {
          puts ("ERROR: count must be at least 1.\r\n");
          help ();
        }
      else if (count > 32767)
        {
          puts ("ERROR: count must be at most 32767.\r\n");
          help ();
        }
      else
        {
          emit_random (count);
        }

      (void)bdos (0, 0);
    }

  puts ("ERROR: Bad option.\r\n");
  help ();

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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
