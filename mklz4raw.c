/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 427e21c0-9417-11f1-a0b8-80ee73e9b8e7
 */

/*****************************************************************************/

/* mklz4raw.c: strip lz4 frame header, emit raw block stream */

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif

/*****************************************************************************/

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

/*****************************************************************************/

static const int never = 0;

#define FREE(p) \
  do {          \
    free((p));  \
    (p) = NULL; \
  } while(never)

/*****************************************************************************/

static unsigned int
get32le (const unsigned char *p)
{
  return (unsigned int)p [0]
      | ((unsigned int)p [1] << 8)
      | ((unsigned int)p [2] << 16)
      | ((unsigned int)p [3] << 24);
}

/*****************************************************************************/

int
main (int argc, char **argv)
{
  FILE *fin, *fout;
  long fsize, out_size;
  unsigned char *buf;
  unsigned char flg;
  int pos, p;

  if (argc < 3)
    {
      (void)fprintf (stderr, "Usage: %s <input.lz4> <output.lz4raw>\n",
                     argv [0]);

      return EXIT_FAILURE;
    }

  fin = fopen (argv [1], "rb");

  if (! fin)
    {
      perror (argv [1]);

      return EXIT_FAILURE;
    }

  (void)fseek (fin, 0, SEEK_END);
  fsize = ftell (fin);
  (void)fseek (fin, 0, SEEK_SET);

  buf = (unsigned char *)malloc (fsize);

  if (! buf)
    {
      (void)fprintf (stderr, "%s: malloc failed\n", argv [0]);
      (void)fclose (fin);

      return EXIT_FAILURE;
    }

  if ((long)fread (buf, 1, fsize, fin) != fsize)
    {
      perror ("read");
      FREE (buf);
      (void)fclose (fin);

      return EXIT_FAILURE;
    }

  (void)fclose (fin);

  if (fsize < 7 || buf [0] != 0x04 || buf [1] != 0x22
                || buf [2] != 0x4D || buf [3] != 0x18)
    {
      (void)fprintf (stderr, "%s: not an LZ4 frame file\n", argv [0]);
      FREE (buf);

      return EXIT_FAILURE;
    }

  flg = buf[4];
  pos = 6;

  if (flg & 0x08)
    pos += 8;

  if (flg & 0x01)
    pos += 4;

  pos += 1;

  if (pos > fsize)
    {
      (void)fprintf (stderr, "%s: frame header truncated\n", argv [0]);
      FREE (buf);

      return EXIT_FAILURE;
    }

  p = pos;

  for (;;)
    {
      unsigned int bsz;

      if (p + 4 > fsize)
        {
          (void)fprintf (stderr,
                         "%s: unexpected end of file in block scan\n",
                         argv [0]);
          FREE (buf);

          return EXIT_FAILURE;
        }

      bsz = get32le (buf + p);

      if (bsz == 0)
        {
          p += 4;

          break;
        }

      bsz &= 0x7FFFFFFFU;
      p += 4 + (int)bsz;

      if (flg & 0x10)
        p += 4;
    }

  out_size = p - pos;

  fout = fopen (argv[2], "wb");

  if (! fout)
    {
      perror (argv[2]);
      FREE (buf);

      return EXIT_FAILURE;
    }

  if ((long)fwrite (buf + pos, 1, out_size, fout) != out_size)
    {
      perror ("write");
      FREE (buf);
      (void)fclose (fout);

      return EXIT_FAILURE;
    }

  (void)fclose (fout);

  (void)fprintf (stderr,
                 "%s: %ld frame bytes -> %ld raw block bytes"
                 " (header %d bytes stripped)\n",
                 argv [0], fsize, out_size, pos);

  FREE (buf);

  return EXIT_SUCCESS;
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
