/* pit.c: 8253/8254 (ring 0) */

/*****************************************************************************/

/* User programs use BDOS 225 (get) / 226 (sleep-until), not these helpers! */

/*****************************************************************************/

#include "pit.h"

/*****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

/*****************************************************************************/

static inline void
outb (uint16_t port, uint8_t val)
{
  __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*****************************************************************************/

static inline uint8_t
inb (uint16_t port)
{
  uint8_t r;

  __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));

  return r;
}

/*****************************************************************************/

#define PIT_CH0 0x40
#define PIT_CMD 0x43

/*****************************************************************************/

#define PIT_CMD_CH0_MODE2 0x34
#define PIT_CMD_LATCH0 0x00

/*****************************************************************************/

static uint32_t pit_wraps;
static uint16_t pit_last;
static int pit_ready;

/*****************************************************************************/

static uint16_t
pit_raw (void)
{
  uint8_t lo, hi;

  outb (PIT_CMD, PIT_CMD_LATCH0);
  lo = inb (PIT_CH0);
  hi = inb (PIT_CH0);

  return (uint16_t)(lo | ((uint16_t)hi << 8));
}

/*****************************************************************************/

void
pit_init (void)
{
  outb (PIT_CMD, PIT_CMD_CH0_MODE2);
  outb (PIT_CH0, 0x00); /* divisor low  */
  outb (PIT_CH0, 0x00); /* divisor high */

  pit_wraps = 0;
  pit_last = pit_raw ();
  pit_ready = 1;
}

/*****************************************************************************/

void
pit_poll (void)
{
  uint16_t now;

  if (!pit_ready)
    {
      return;
    }

  now = pit_raw ();

  if (now > pit_last)
    {
      pit_wraps++;
    }

  pit_last = now;
}

/*****************************************************************************/

static uint64_t
pit_now64 (void)
{
  uint16_t now;

  if (!pit_ready)
    {
      pit_init ();
    }

  now = pit_raw ();

  if (now > pit_last)
    {
      pit_wraps++;
    }

  pit_last = now;

  return ((uint64_t)pit_wraps << 16) + (uint16_t)(0u - now);
}

/*****************************************************************************/

void
pit_read (unsigned long *lo, unsigned long *hi)
{
  uint64_t t = pit_now64 ();

  if (lo)
    {
      *lo = (unsigned long)(t & 0xffffffffUL);
    }

  if (hi)
    {
      *hi = (unsigned long)(t >> 32);
    }
}

/*****************************************************************************/

void
pit_sleep_until (unsigned long lo, unsigned long hi)
{
  uint64_t target = ((uint64_t)hi << 32) | (uint64_t)lo;

  while (pit_now64 () < target)
    {
      /* Busy-wait PIC masked / no sleep-idle yet */
      ;
    }
}

/*****************************************************************************/
