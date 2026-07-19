/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 29a97c58-82b5-11f1-b95a-80ee73e9b8e7
 */

/*****************************************************************************/

/* shole.c - punch sparse hole in a CP/M dirent map */

/*****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

/*
 * Copy src into dst[width], uppercased
 * and space-padded (CP/M 8.3 field).
 */

static void
pad_field (char *dst, int width, const char *src, int srclen)
{
  int i;

  if (srclen > width)
    {
      srclen = width;
    }

  for (i = 0; i < srclen; i++)
    {
      dst[i] = (char)toupper ((unsigned char)src[i]);
    }

  for (; i < width; i++)
    {
      dst[i] = ' ';
    }
}

/*****************************************************************************/

int
main (int argc, char **argv)
{
  const char *path, *want, *dot;
  char name[8], typ[3];
  unsigned char *data;
  long sz;
  FILE *f;
  int i, j, patched = 0;

  if (argc < 3)
    {
      fprintf (stderr, "usage: %s image name.typ\n", argv[0]);

      return 1;
    }

  path = argv[1];
  want = argv[2];

  dot = strchr (want, '.');

  if (dot)
    {
      pad_field (name, 8, want, (int)(dot - want));
      pad_field (typ, 3, dot + 1, (int)strlen (dot + 1));
    }
  else
    {
      pad_field (name, 8, want, (int)strlen (want));
      pad_field (typ, 3, "", 0);
    }

  f = fopen (path, "rb");

  if (!f)
    {
      perror (path);

      return 1;
    }

  if (fseek (f, 0, SEEK_END) != 0)
    {
      perror ("fseek");
      fclose (f);

      return 1;
    }

  sz = ftell (f);

  if (sz < 32)
    {
      fclose (f);
      fprintf (stderr, "shole: image too small\n");

      return 1;
    }

  rewind (f);
  data = malloc ((size_t)sz);

  if (!data || fread (data, 1, (size_t)sz, f) != (size_t)sz)
    {
      fprintf (stderr, "shole: read failed\n");
      free (data);
      fclose (f);

      return 1;
    }

  fclose (f);

  /* Scan first 8 KiB of directory (256 entries for 4mb-hd). */
  for (i = 0; i + 32 <= sz && i < 8192; i += 32)
    {
      int nonzero[16], ncnt = 0, target, old;

      if (data[i] >= 16)
        {
          continue;
        }

      if (memcmp (data + i + 1, name, 8) != 0
          || memcmp (data + i + 9, typ, 3) != 0)
        {
          continue;
        }

      /* Map is 16 bytes at +16. Zero a middle non-zero byte. */
      for (j = 0; j < 16; j++)
        {
          if (data[i + 16 + j] != 0)
            {
              nonzero[ncnt++] = j;
            }
        }

      if (ncnt < 3)
        {
          printf ("shole: %s only %d map bytes, skip\n", want, ncnt);

          continue;
        }

      target = nonzero[ncnt / 2];
      old = data[i + 16 + target];
      data[i + 16 + target] = 0;
      patched = 1;
      printf ("shole: %s extent@dir %d cleared map[%d] was %d\n", want, i,
              target, old);

      break; /* first matching extent only */
    }

  if (!patched)
    {
      fprintf (stderr, "shole: %s not found\n", want);
      free (data);

      return 1;
    }

  f = fopen (path, "wb");

  if (!f)
    {
      perror (path);
      free (data);

      return 1;
    }

  if (fwrite (data, 1, (size_t)sz, f) != (size_t)sz)
    {
      fprintf (stderr, "shole: write failed\n");
      fclose (f);
      free (data);

      return 1;
    }

  fclose (f);
  free (data);

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
