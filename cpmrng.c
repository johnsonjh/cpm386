/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 8b632a7a-8e95-11f1-9335-80ee73e9b8e7
 */

/*****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/*****************************************************************************/

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/*****************************************************************************/

static void
salsa20_quarterround (uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
  *b ^= ROTL32 (*a + *d, 7);
  *c ^= ROTL32 (*b + *a, 9);
  *d ^= ROTL32 (*c + *b, 13);
  *a ^= ROTL32 (*d + *c, 18);
}

/*****************************************************************************/

static void
salsa20_block (const uint32_t in[16], uint32_t out[16])
{
  uint32_t x[16];
  int i;

  for (i = 0; i < 16; i++)
    {
      x[i] = in[i];
    }

  for (i = 0; i < 10; i++)
    {
      salsa20_quarterround ( &x  [0], &x  [4], &x  [8], &x [12] );
      salsa20_quarterround ( &x  [5], &x  [9], &x [13], &x  [1] );
      salsa20_quarterround ( &x [10], &x [14], &x  [2], &x  [6] );
      salsa20_quarterround ( &x [15], &x  [3], &x  [7], &x [11] );
      salsa20_quarterround ( &x  [0], &x  [1], &x  [2], &x  [3] );
      salsa20_quarterround ( &x  [5], &x  [6], &x  [7], &x  [4] );
      salsa20_quarterround ( &x [10], &x [11], &x  [8], &x  [9] );
      salsa20_quarterround ( &x [15], &x [12], &x [13], &x [14] );
    }

  for (i = 0; i < 16; i++)
    {
      out[i] = x[i] + in[i];
    }
}

/*****************************************************************************/

static uint8_t  rng_key[32];
static uint8_t  rng_nonce[8];
static uint64_t rng_counter;
static unsigned long rng_seed_total;   /* cumulative seed bytes absorbed */
static unsigned long rng_output_count; /* calls since last rekey         */

/*****************************************************************************/

#define RNG_SEED_THRESHOLD 256 /* minimum seed bytes before output   */
#define RNG_REKEY_INTERVAL 256 /* output calls between rekeys (~16K) */

/*****************************************************************************/

static const uint32_t sigma[4] = {
  0x61707865UL, /* "expa" */
  0x3320646EUL, /* "nd 3" */
  0x79622D32UL, /* "2-by" */
  0x6B206574UL  /* "te k" */
};

/*****************************************************************************/

static uint32_t
load32_le (const uint8_t *p)
{
  return (uint32_t)p[0]
      | ((uint32_t)p[1] << 8)
      | ((uint32_t)p[2] << 16)
      | ((uint32_t)p[3] << 24);
}

/*****************************************************************************/

static void
store32_le (uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v);
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/*****************************************************************************/

static void
build_matrix (uint32_t m[16])
{
  m[0]  = sigma[0];
  m[1]  = load32_le (&rng_key[0]);
  m[2]  = load32_le (&rng_key[4]);
  m[3]  = load32_le (&rng_key[8]);
  m[4]  = load32_le (&rng_key[12]);
  m[5]  = sigma[1];
  m[6]  = load32_le (&rng_nonce[0]);
  m[7]  = load32_le (&rng_nonce[4]);
  m[8]  = (uint32_t)(rng_counter & 0xFFFFFFFFUL);
  m[9]  = (uint32_t)(rng_counter >> 32);
  m[10] = sigma[2];
  m[11] = load32_le (&rng_key[16]);
  m[12] = load32_le (&rng_key[20]);
  m[13] = load32_le (&rng_key[24]);
  m[14] = load32_le (&rng_key[28]);
  m[15] = sigma[3];
}

/*****************************************************************************/

static void
mix_pool (void)
{
  uint32_t m[16], out[16];
  int i;

  build_matrix (m);
  salsa20_block (m, out);

  for (i = 0; i < 8; i++)
    {
      store32_le (&rng_key[i * 4], out[i]);
    }

  store32_le (&rng_nonce[0], out[8]);
  store32_le (&rng_nonce[4], out[9]);

  rng_counter++;
}

/*****************************************************************************/

unsigned short
salsa20rng_seed (const unsigned char *buf, unsigned long len)
{
  unsigned long i;

  if (!buf || len == 0 || len > 64)
    {
      return 0xFFFF;
    }

  for (i = 0; i < len; i++)
    {
      rng_key[i % 32] ^= buf[i];
    }

  for (i = 0; i < len && i < 8; i++)
    {
      rng_nonce[i] ^= buf[i];
    }

  rng_seed_total += len;

  mix_pool ();

  return 0;
}

/*****************************************************************************/

static void
rekey (void)
{
  uint32_t m[16], out[16];
  int i;

  build_matrix (m);
  salsa20_block (m, out);

  for (i = 0; i < 8; i++)
    {
      store32_le (&rng_key[i * 4], out[i]);
    }

  store32_le (&rng_nonce[0], out[8]);
  store32_le (&rng_nonce[4], out[9]);

  rng_counter = 0;
  rng_output_count = 0;
}

/*****************************************************************************/

unsigned short
salsa20rng_get16 (void)
{
  uint32_t m[16], out[16];
  uint16_t result;

  if (rng_seed_total < RNG_SEED_THRESHOLD)
    {
      return 0xFFFF;
    }

  if (rng_output_count >= RNG_REKEY_INTERVAL)
    {
      rekey ();
    }

  build_matrix (m);
  salsa20_block (m, out);

  result = (uint16_t)(out[10] & 0xFFFF);

  if (result == 0xFFFF)
    {
      result = 0xFFFE;
    }

  rng_counter++;
  rng_output_count++;

  return result;
}

/*****************************************************************************/

int
salsa20rng_seeded (void)
{
  return (rng_seed_total >= RNG_SEED_THRESHOLD) ? 1 : 0;
}

/*****************************************************************************/

#define PIT_CH0    0x40
#define PIT_CMD    0x43
#define PIT_LATCH  0x00

/*****************************************************************************/

static inline void
rng_outb (uint16_t port, uint8_t val)
{
  __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*****************************************************************************/

static inline uint8_t
rng_inb (uint16_t port)
{
  uint8_t r;

  __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));

  return r;
}

/*****************************************************************************/

static uint16_t
rng_pit_raw (void)
{
  uint8_t lo, hi;

  rng_outb (PIT_CMD, PIT_LATCH);
  lo = rng_inb (PIT_CH0);
  hi = rng_inb (PIT_CH0);

  return (uint16_t)(lo | ((uint16_t)hi << 8));
}

/*****************************************************************************/

static void
rng_busy_delay (unsigned long iters)
{
  volatile unsigned long i;

  for (i = 0; i < iters; i++)
    {
      __asm__ volatile ("nop");
    }
}

/*****************************************************************************/

#define PIT_SEED_ROUNDS  16  /* number of 32-byte seed blocks */
#define PIT_SAMPLES      32  /* samples per round             */

/*****************************************************************************/

void
salsa20rng_auto_seed_pit (void)
{
  int round;

  for (round = 0; round < PIT_SEED_ROUNDS; round++)
    {
      uint8_t buf[PIT_SAMPLES];
      int j;

      for (j = 0; j < PIT_SAMPLES; j++)
        {
          unsigned long delay = 50 + (j * 7) + (round * 13);

          if (j > 0)
            {
              delay += (unsigned long)(buf[j - 1] & 0x3F);
            }

          rng_busy_delay (delay);

          buf[j] = (uint8_t)(rng_pit_raw () & 0xFF);
        }

      (void)salsa20rng_seed (buf, PIT_SAMPLES);
    }
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
