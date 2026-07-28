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
 * Header (12 bytes LE) of:
 *   load_off   - TPA-relative load offset (usually 0x100)
 *   img_size   - image byte length
 *   entry_off  - TPA-relative entry (usually same as load_off)
 */

/*****************************************************************************/

/*
 * Usage: mk386 input.bin output.386 [load_off_hex]
 */

/*****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

int
main (int argc, char **argv)
{
  const char *in, *out;
  uint32_t load = 0x100;
  FILE *f;
  long sz;
  unsigned char *buf;
  uint32_t hdr[3];

  if (argc < 3)
    {
      (void)fprintf (stderr, "usage: %s in.bin out.386 [load_off]\n", argv[0]);

      return 1;
    }

  in = argv[1];
  out = argv[2];

  if (argc >= 4)
    {
      load = (uint32_t)strtoul (argv[3], NULL, 0);
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

  hdr[0] = load;
  hdr[1] = (uint32_t)sz;
  hdr[2] = load; /* entry = load */

  f = fopen (out, "wb");

  if (!f)
    {
      perror (out);
      free (buf);

      return 1;
    }

  if (fwrite (hdr, 4, 3, f) != 3
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
