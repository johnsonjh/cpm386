/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: c2c84892-82b5-11f1-987f-80ee73e9b8e7
 */

/*****************************************************************************/

#ifndef RTC_H
# define RTC_H

/*****************************************************************************/

/*
 * CP/M-386 date/time block (pointer arg for BDOS 104/105).
 * days: day count with day 0 = 1978-01-01 (CP/M Plus epoch style, 0-based).
 * hour/min/sec: binary 0-23 / 0-59 / 0-59 (not BCD - clearer for 386).
 */

struct cpm_datetime
{
  unsigned short days;
  unsigned char hour;
  unsigned char min;
  unsigned char sec;
};

/*****************************************************************************/

/*
 * Read CMOS RTC into *dt. Returns 0 on success, non-zero on failure.
 * hour/min/sec are binary (CP/M-386 BDOS 105).
 */

int rtc_get (struct cpm_datetime *dt);

/*****************************************************************************/

/* BDOS 155 (T_SECONDS / MP/M): same as rtc_get but hour/min/sec packed BCD. */
int rtc_get_bcd (struct cpm_datetime *dt);

/*****************************************************************************/

/* Write *dt to CMOS RTC. Returns 0 on success, non-zero on failure. */
int rtc_set (const struct cpm_datetime *dt);

/*****************************************************************************/

/* Convert calendar <-> days since 1978-01-01 */
unsigned short rtc_ymd_to_days (unsigned y, unsigned m, unsigned d);
void rtc_days_to_ymd (unsigned short days, unsigned *y, unsigned *m,
                      unsigned *d);

/*****************************************************************************/

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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
