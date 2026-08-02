/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 886941f6-8e95-11f1-8f36-80ee73e9b8e7
 */

/*****************************************************************************/

/*
 * BDOS 253 (RNG_GET):  return 16 bits of randomness.  Returns 0xFFFF if
 *                      the RNG has not been seeded with enough material.
 * BDOS 254 (RNG_SEED): fold seed material into the pool.
 *                      DE -> struct cpm_rng_seed (TPA-relative).
 *                      Returns 0 on success, 0xFFFF on bad pointer.
 */

/*****************************************************************************/

#ifndef SALSA20RNG_H
# define SALSA20RNG_H

/*****************************************************************************/

/*
 * Seed block passed to BDOS 254.  data is a TPA-relative pointer to len
 * bytes of seed material (1..64).  The material is folded into the
 * existing pool state, never replacing it.
 */

struct cpm_rng_seed
{
  unsigned long data; /* TPA-relative pointer to seed bytes */
  unsigned long len;  /* byte count, 1..64                  */
};

/*****************************************************************************/

/*
 * Kernel API:
 *
 * salsa20rng_seed(): absorb len bytes from buf into the pool.
 *   Always mixes, never overwrites.  Accumulates toward the seeding
 *   threshold.  len must be 1..64.  Returns 0 on success.
 *
 * salsa20rng_get16(): extract 16 bits of randomness.
 *   Returns 0..0xFFFE on success, 0xFFFF if not yet seeded.
 *   Fail-closed: refuses output until >= 256 cumulative seed bytes
 *   have been absorbed.
 *
 * salsa20rng_seeded(): return 1 if the pool is ready, 0 otherwise.
 *
 * salsa20rng_auto_seed_pit(): gather PIT jitter and folds in.
 *   Called automatically during boot for initial entropy.
 */

unsigned short salsa20rng_seed (const unsigned char *buf, unsigned long len);
unsigned short salsa20rng_get16 (void);
int salsa20rng_seeded (void);
void salsa20rng_auto_seed_pit (void);

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
