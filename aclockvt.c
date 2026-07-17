/* aclockvt.c - VT100 aclock for CP/M-386 */

/*
 * Based on aclock-vt100.c
 * Copyright (c) 2002 Antoni Sawicki <tenox@tenox.tc>
 * Version 1.8 (knr-nofloat-vt100); Dublin, June 2002
 */

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

#include "aclock.h"

#define BDOS_INT     0x30
#define BDOS_CONOUT  2
#define BDOS_CONIN   1
#define BDOS_CONST   11
#define BDOS_GET_TOD 105

struct cpm_datetime {
  UWORD days;
  UBYTE hour;
  UBYTE min;
  UBYTE sec;
};

void _start(void) __attribute__((section(".text._start")));

static UWORD bdos(WORD func, LONG info)
{
  UWORD ret;
  __asm__ volatile (
    "int %2"
    : "=a"(ret)
    : "a"((unsigned)func), "i"(BDOS_INT), "d"((unsigned long)info)
    : "memory", "cc"
  );

  return ret;
}

static void putch(char c)
{
  bdos(BDOS_CONOUT, (LONG)(unsigned char)c);
}

static void puts(const char *s)
{
  while (*s)
    putch(*s++);
}

static int key_ready(void)
{
  return bdos(BDOS_CONST, 0) != 0;
}

static void flush_key(void)
{
  while (key_ready())
    (void)bdos(BDOS_CONIN, 0);
}

static int wait_next_second(UBYTE last_sec)
{
  struct cpm_datetime dt;

  for (;;) {
    if (key_ready()) {
      (void)bdos(BDOS_CONIN, 0);

      return 1;
    }

    if (bdos(BDOS_GET_TOD, (LONG)(unsigned long)&dt) == 0 && dt.sec != last_sec)
      return 0;
  }
}

static void cls(void)
{
  puts("\033[0;0H\033[J");
}

static void draw_point(int x, int y, char c)
{
  puts("\033[");

  {
    char buf[8];
    int i = 0, n = y;

    if (n <= 0) {
      putch('0');
    } else {
      while (n && i < 8) {
        buf[i++] = (char)('0' + n % 10);
        n /= 10;
      }

      while (i)
        putch(buf[--i]);
    }

    putch(';');

    n = x;
    i = 0;

    if (n <= 0) {
      putch('0');
    } else {
      while (n && i < 8) {
        buf[i++] = (char)('0' + n % 10);
        n /= 10;
      }

      while (i)
        putch(buf[--i]);
    }

    putch('H');
  }

  putch(c);
}

static void draw_text(int x, int y, const char *string)
{
  puts("\033[");
  {
    char buf[8];
    int i = 0, n = y;

    if (n <= 0)
      putch('0');
    else {
      while (n && i < 8) {
        buf[i++] = (char)('0' + n % 10);
        n /= 10;
      }

      while (i)
        putch(buf[--i]);
    }

    putch(';');

    n = x;
    i = 0;

    if (n <= 0)
      putch('0');
    else {
      while (n && i < 8) {
        buf[i++] = (char)('0' + n % 10);
        n /= 10;
      }

      while (i)
        putch(buf[--i]);
    }

    putch('H');
  }

  puts(string);
}

static void draw_circle(void)
{
  int n;

  for (n = 0; n < 60; n++)
    draw_point(circle[n][0], circle[n][1], (char)circle[n][2]);
}

static void draw_hour(int n)
{
  int m;

  for (m = 0; m < 6; m++)
    draw_point(hour[n][m][0], hour[n][m][1], 'h');
}

static void draw_minute(int n)
{
  int m;

  for (m = 0; m < 8; m++)
    draw_point(minute[n][m][0], minute[n][m][1], 'm');
}

static void draw_seconds(int n)
{
  int m;

  for (m = 0; m < 8; m++)
    draw_point(minute[n][m][0], minute[n][m][1], '.');
}

void _start(void)
{
  struct cpm_datetime dt;
  int hidx;
  char dig[16];

  flush_key();

  for (;;) {
    if (bdos(BDOS_GET_TOD, (LONG)(unsigned long)&dt) != 0) {
      dt.hour = dt.min = dt.sec = 0;
    }

    cls();

    draw_circle();

    hidx = ((dt.hour >= 12 ? dt.hour - 12 : dt.hour) * 5) + (dt.min / 10);

    if (hidx < 0)
      hidx = 0;

    if (hidx > 59)
      hidx = 59;

    draw_hour(hidx);

    draw_minute(dt.min % 60);

    draw_seconds(dt.sec % 60);

    draw_text(35, 6, ".:ACLOCK:.");

    dig[0] = '[';
    dig[1] = (char)('0' + (dt.hour / 10) % 10);
    dig[2] = (char)('0' + dt.hour % 10);
    dig[3] = ':';
    dig[4] = (char)('0' + (dt.min / 10) % 10);
    dig[5] = (char)('0' + dt.min % 10);
    dig[6] = ':';
    dig[7] = (char)('0' + (dt.sec / 10) % 10);
    dig[8] = (char)('0' + dt.sec % 10);
    dig[9] = ']';
    dig[10] = 0;

    draw_text(35, 19, dig);

    if (wait_next_second(dt.sec))
      break;
  }

  puts("\033[24;1H\r\n");

  bdos(0, 0);
}
