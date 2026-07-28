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

static void
usage (const char *prog)
{
  (void)fprintf (stderr,
                 "usage: %s [-b blocksize] [-w | -B] image name.typ\n"
                 "  -b blocksize   image block size in bytes (default 2048);\n"
                 "                 used to auto-detect map entry width when\n"
                 "                 neither -w nor -B is given\n"
                 "  -w             force 16-bit (word) allocation-map entries\n"
                 "                 (8 slots in the 16-byte map)\n"
                 "  -B             force 8-bit (byte) allocation-map entries\n"
                 "                 (16 slots in the 16-byte map)\n",
                 prog);
}

/*****************************************************************************/

int
main (int argc, char **argv)
{
  const char *path, *want, *dot;
  char name[8], typ[3];
  unsigned char *data;
  long sz, blocksize;
  FILE *f;
  int i, j, patched, ai;
  int word_mode, force_word, force_byte;

  blocksize = 2048;
  word_mode = 0;
  force_word = 0;
  force_byte = 0;
  patched = 0;

  ai = 1;

  while (ai < argc && argv[ai][0] == '-' && argv[ai][1] != '\0')
    {
      if (strcmp (argv[ai], "-b") == 0)
        {
          if (ai + 1 >= argc)
            {
              (void)fprintf (stderr, "shole: -b requires an argument\n");
              usage (argv[0]);

              return 1;
            }

          blocksize = atol (argv[ai + 1]);

          if (blocksize <= 0)
            {
              (void)fprintf (stderr, "shole: invalid blocksize '%s'\n",
                             argv[ai + 1]);

              return 1;
            }

          ai += 2;
        }
      else if (strcmp (argv[ai], "-w") == 0)
        {
          force_word = 1;
          ai += 1;
        }
      else if (strcmp (argv[ai], "-B") == 0)
        {
          force_byte = 1;
          ai += 1;
        }
      else if (strcmp (argv[ai], "--") == 0)
        {
          ai += 1;

          break;
        }
      else
        {
          (void)fprintf (stderr, "shole: unknown option '%s'\n", argv[ai]);
          usage (argv[0]);

          return 1;
        }
    }

  if (force_word && force_byte)
    {
      (void)fprintf (stderr, "shole: -w and -B are mutually exclusive\n");

      return 1;
    }

  if (argc - ai < 2)
    {
      usage (argv[0]);

      return 1;
    }

  path = argv[ai];
  want = argv[ai + 1];

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
      (void)fclose (f);

      return 1;
    }

  sz = ftell (f);

  if (sz < 32)
    {
      (void)fclose (f);
      (void)fprintf (stderr, "shole: image too small\n");

      return 1;
    }

  rewind (f);
  data = malloc ((size_t)sz);

  if (!data || fread (data, 1, (size_t)sz, f) != (size_t)sz)
    {
      (void)fprintf (stderr, "shole: read failed\n");
      free (data);
      (void)fclose (f);

      return 1;
    }

  (void)fclose (f);

  if (force_word)
    {
      word_mode = 1;
    }
  else if (force_byte)
    {
      word_mode = 0;
    }
  else
    {
      long nblocks = sz / blocksize;

      word_mode = (nblocks > 256);
    }

  /* Scan first 8 KiB of directory (256 entries for 4mb-hd). */
  for (i = 0; i + 32 <= sz && i < 8192; i += 32)
    {
      int nonzero[8], ncnt, target, old, nslots;

      if (data[i] >= 16)
        {
          continue;
        }

      if (memcmp (data + i + 1, name, 8) != 0
          || memcmp (data + i + 9, typ, 3) != 0)
        {
          continue;
        }

      ncnt = 0;
      nslots = word_mode ? 8 : 16;

      for (j = 0; j < nslots; j++)
        {
          unsigned int val;

          if (word_mode)
            {
              val = (unsigned int)data[i + 16 + j * 2]
                    | ((unsigned int)data[i + 16 + j * 2 + 1] << 8);
            }
          else
            {
              val = data[i + 16 + j];
            }

          if (val != 0)
            {
              nonzero[ncnt++] = j;
            }
        }

      if (ncnt < 3)
        {
          (void)printf ("shole: %s only %d map entries, skip\n", want, ncnt);

          continue;
        }

      target = nonzero[ncnt / 2];

      if (word_mode)
        {
          old = data[i + 16 + target * 2]
                | (data[i + 16 + target * 2 + 1] << 8);
          data[i + 16 + target * 2] = 0;
          data[i + 16 + target * 2 + 1] = 0;
        }
      else
        {
          old = data[i + 16 + target];
          data[i + 16 + target] = 0;
        }

      patched = 1;
      (void)printf ("shole: %s extent@dir %d cleared map[%d] (%s) was %d\n",
                    want, i, target, word_mode ? "word" : "byte", old);

      break; /* first matching extent only */
    }

  if (!patched)
    {
      (void)fprintf (stderr, "shole: %s not found\n", want);
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
      (void)fprintf (stderr, "shole: write failed\n");
      (void)fclose (f);
      free (data);

      return 1;
    }

  (void)fclose (f);
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
