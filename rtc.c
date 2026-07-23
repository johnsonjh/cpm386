/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: aa5882fe-82b5-11f1-bcdb-80ee73e9b8e7
 */

/*****************************************************************************/

/* rtc.c: CMOS RTC */

/*****************************************************************************/

/*
 * Ring-0 only: programs use BDOS 104 (set) / 105 (get) via int 0x30.
 */

/*****************************************************************************/

#include "rtc.h"

/*****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

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

/* CMOS index/data ports; NMI stays enabled (bit7 clear on index). */
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

/*****************************************************************************/

static uint8_t
cmos_read (uint8_t reg)
{
  outb (CMOS_ADDR, (uint8_t)(reg & 0x7f));
  return inb (CMOS_DATA);
}

/*****************************************************************************/

static void
cmos_write (uint8_t reg, uint8_t val)
{
  outb (CMOS_ADDR, (uint8_t)(reg & 0x7f));
  outb (CMOS_DATA, val);
}

/*****************************************************************************/

static int
cmos_update_in_progress (void)
{
  return (cmos_read (0x0a) & 0x80) != 0;
}

/*****************************************************************************/

static uint8_t
bcd_to_bin (uint8_t v)
{
  return (uint8_t)((v & 0x0f) + ((v >> 4) * 10));
}

/*****************************************************************************/

static uint8_t
bin_to_bcd (uint8_t v)
{
  return (uint8_t)(((v / 10) << 4) | (v % 10));
}

/*****************************************************************************/

static int
is_leap (unsigned y)
{
  return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

/*****************************************************************************/

static const unsigned char mdays_n[]
    = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*****************************************************************************/

unsigned short
rtc_ymd_to_days (unsigned y, unsigned m, unsigned d)
{
  unsigned day = 0;
  unsigned yy, mm;

  /* Epoch: 1978-01-01 = day 0 */
  for (yy = 1978; yy < y; yy++)
    {
      day += is_leap (yy) ? 366u : 365u;
    }

  for (mm = 1; mm < m; mm++)
    {
      day += mdays_n[mm - 1];

      if (mm == 2 && is_leap (y))
        {
          day++;
        }
    }

  day += (d - 1);

  return (unsigned short)day;
}

/*****************************************************************************/

void
rtc_days_to_ymd (unsigned short days, unsigned *y, unsigned *m, unsigned *d)
{
  unsigned yy = 1978;
  unsigned rem = days;

  for (;;)
    {
      unsigned ylen = is_leap (yy) ? 366u : 365u;

      if (rem < ylen)
        {
          break;
        }

      rem -= ylen;
      yy++;
    }

  *y = yy;
  *m = 1;

  for (;;)
    {
      unsigned dim;
      dim = mdays_n[*m - 1];

      if (*m == 2 && is_leap (yy))
        {
          dim = 29;
        }

      if (rem < dim)
        {
          break;
        }

      rem -= dim;
      (*m)++;
    }

  *d = rem + 1;
}

/*****************************************************************************/

int
rtc_get (struct cpm_datetime *dt)
{
  uint8_t s, m, h, day, mon, yr, stb;
  int spins = 0;
  unsigned y, mo, d;

  if (!dt)
    {
      return 1;
    }

  /* Wait for update to finish (UIP clear) */
  while (cmos_update_in_progress () && ++spins < 100000)
    {
      ;
    }

  /* Read twice for consistency */
  do
    {
      s = cmos_read (0x00);
      m = cmos_read (0x02);
      h = cmos_read (0x04);
      day = cmos_read (0x07);
      mon = cmos_read (0x08);
      yr = cmos_read (0x09);
      stb = cmos_read (0x0b);
    }
  while ((s != cmos_read (0x00) || m != cmos_read (0x02)) && ++spins < 100000);

  /* Convert BCD if needed (DM bit clear => BCD) */
  if ((stb & 0x04) == 0)
    {
      s = bcd_to_bin (s);
      m = bcd_to_bin (m);
      h = bcd_to_bin (h & 0x7f);
      day = bcd_to_bin (day);
      mon = bcd_to_bin (mon);
      yr = bcd_to_bin (yr);
    }
  else
    {
      h &= 0x7f;
    }

  /* 12-hour mode: bit 7 of hours is PM when not 24h (bit1 of status B) */
  if ((stb & 0x02) == 0)
    {
      uint8_t pm = h & 0x80;
      h &= 0x7f;

      if (pm && h < 12)
        {
          h = (uint8_t)(h + 12);
        }

      if (!pm && h == 12)
        {
          h = 0;
        }
    }

  /*
   * CMOS year is 00-99; map 00-77 -> 2000-2077, 78-99 -> 1978-1999
   * (CP/M-era convention / original TOD: YY in 00-99 for 2000-2099 in docs,
   * but CMOS is free-running - use 1978-2077 window covering epoch).
   */

  if (yr < 78)
    {
      y = 2000u + yr;
    }
  else
    {
      y = 1900u + yr;
    }

  if (mon < 1 || mon > 12 || day < 1 || day > 31 || h > 23 || m > 59 || s > 59)
    {
      return 2;
    }

  mo = mon;
  d = day;
  dt->days = rtc_ymd_to_days (y, mo, d);
  dt->hour = h;
  dt->min = m;
  dt->sec = s;

  return 0;
}

/*****************************************************************************/

/*
 * MP/M BDOS 155 (T_SECONDS): fill stamp with BCD h/m/s (and seconds).
 * Day count matches rtc_get / BDOS 105 (1978-01-01 = day 0).
 */

int
rtc_get_bcd (struct cpm_datetime *dt)
{
  int r = rtc_get (dt);

  if (r)
    {
      return r;
    }

  dt->hour = bin_to_bcd (dt->hour);
  dt->min = bin_to_bcd (dt->min);
  dt->sec = bin_to_bcd (dt->sec);

  return 0;
}

/*****************************************************************************/

int
rtc_set (const struct cpm_datetime *dt)
{
  unsigned y, mo, d;
  uint8_t stb, yr, bin;
  uint8_t sec, min, hour, day, mon;

  if (!dt || dt->hour > 23 || dt->min > 59 || dt->sec > 59)
    {
      return 1;
    }

  rtc_days_to_ymd (dt->days, &y, &mo, &d);

  if (y < 1978 || y > 2099 || mo < 1 || mo > 12 || d < 1 || d > 31)
    {
      return 2;
    }

  yr = (uint8_t)(y % 100);
  sec = dt->sec;
  min = dt->min;
  hour = dt->hour;
  day = (uint8_t)d;
  mon = (uint8_t)mo;

  stb = cmos_read (0x0b);
  /* Stop updates while programming (SET bit) */
  cmos_write (0x0b, (uint8_t)(stb | 0x80));

  if ((stb & 0x04) == 0)
    {
      /* BCD mode */
      sec = bin_to_bcd (sec);
      min = bin_to_bcd (min);
      hour = bin_to_bcd (hour);
      day = bin_to_bcd (day);
      mon = bin_to_bcd (mon);
      yr = bin_to_bcd (yr);
    }

  /* Prefer 24-hour format */
  bin = (uint8_t)((stb | 0x02) & ~0x80); /* 24h, clear SET for final */
  cmos_write (0x00, sec);
  cmos_write (0x02, min);
  cmos_write (0x04, hour);
  cmos_write (0x07, day);
  cmos_write (0x08, mon);
  cmos_write (0x09, yr);
  cmos_write (0x0b, bin); /* release SET, keep 24h / data mode */

  return 0;
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
