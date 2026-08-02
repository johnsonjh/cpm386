/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: eafcd888-82b4-11f1-99f5-80ee73e9b8e7
 */

/*****************************************************************************/

/* mk386.c: wrap a raw binary as a 386 absolute command file. */

/*****************************************************************************/

/*
 * Usage: mk386 <input.bin> <output.386> [load_off [min_kb]]
 */

/*****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

/*
 * The .386 header, described in bdosdef.h.  This is a host tool and cannot
 * include the kernel headers, so the three values it needs are repeated
 * here, and we need always to keep them in sync.
 */

#define CPM386_MAGIC 0x3638334DUL /* "M386" in little-endian */
#define CPM386_VERSION 1
#define CPM386_HDR_SIZE 32

/*****************************************************************************/

static void
put32 (unsigned char *p, uint32_t v)
{
  p [0] = (unsigned char)(v & 0xFF);
  p [1] = (unsigned char)((v >> 8) & 0xFF);
  p [2] = (unsigned char)((v >> 16) & 0xFF);
  p [3] = (unsigned char)((v >> 24) & 0xFF);
}

/*****************************************************************************/

int
main (int argc, char **argv)
{
  const char *in, *out;
  uint32_t load = 0x100;
  uint32_t min_kb = 0;
  FILE *f;
  long sz;
  int i;
  unsigned char *buf;
  unsigned char hdr [CPM386_HDR_SIZE];

  if (argc < 3)
    {
      (void)fprintf (stderr, "usage: %s in.bin out.386 [load_off [min_kb]]\n",
                     argv [0]);
      (void)fprintf (stderr,
                     "  min_kb: total TPA the program needs, in KB.\n");
      (void)fprintf (stderr,
                     "          0 (the default) means do not check.\n");

      return 1;
    }

  in = argv [1];
  out = argv [2];

  if (argc >= 4)
    {
      load = (uint32_t)strtoul (argv [3], NULL, 0);
    }

  if (argc >= 5)
    {
      min_kb = (uint32_t)strtoul (argv [4], NULL, 0);
    }

  f = fopen (in, "rb");

  if (!f)
    {
      perror (in);

      return 1;
    }

  if (fseek (f, 0, SEEK_END) != 0)
    {
      perror ("fseek");
      (void)fclose (f);

      return 1;
    }

  sz = ftell (f);

  if (sz < 0)
    {
      (void)fclose (f);

      return 1;
    }

  rewind (f);
  buf = malloc ((size_t)sz);

  if (!buf || (sz && fread (buf, 1, (size_t)sz, f) != (size_t)sz))
    {
      (void)fprintf (stderr, "read failed\n");
      free (buf);
      (void)fclose (f);

      return 1;
    }

  (void)fclose (f);

  /*
   * Image may contain leading zeros if linked at 0x100 (file starts at 0).
   * objcopy -O binary from a VMA of 0x100 still starts the file at 0x100
   * content without zero-padding from 0 - ld/objcopy emits from first section.
   * Our user.ld sets . = 0x100 so the binary begins at the first byte of .text
   * and load_off in the header places it at TPA+0x100.
   */

  for (i = 0; i < CPM386_HDR_SIZE; i++)
    {
      hdr [i] = 0;
    }

  put32 (hdr + 0, CPM386_MAGIC);
  hdr [4] = CPM386_VERSION;
  hdr [5] = CPM386_HDR_SIZE;
  /* hdr[6..7] flags, reserved, left zero */
  put32 (hdr + 8, load);
  put32 (hdr + 12, (uint32_t)sz);
  put32 (hdr + 16, load); /* entry = load */
  put32 (hdr + 20, min_kb);
  /* hdr[24..31] reserved, left zero */

  f = fopen (out, "wb");

  if (!f)
    {
      perror (out);
      free (buf);

      return 1;
    }

  if (fwrite (hdr, 1, CPM386_HDR_SIZE, f) != CPM386_HDR_SIZE
      || (sz && fwrite (buf, 1, (size_t)sz, f) != (size_t)sz))
    {
      (void)fprintf (stderr, "write failed\n");
      (void)fclose (f);
      free (buf);

      return 1;
    }

  (void)fclose (f);
  free (buf);

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
