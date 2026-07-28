/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 0ebda36a-82b5-11f1-9cb9-80ee73e9b8e7
 */

/*****************************************************************************/

/* mltiboot.c - multiboot support for CP/M-386 */

/*****************************************************************************/

#include "absaddr.h"
#include "bdosinc.h"
#include "biosdef.h"
#include "mltiboot.h"

/*****************************************************************************/

/*
 * We write to the same scratch locations the original real-mode
 * loader in bios_getmrt() and existing TPA logic works unchanged.
 */

/*****************************************************************************/

#define LEGACY_MEMTOP (ABS_U32 (0x600))
#define LEGACY_TPA_BASE (ABS_U32 (0x604))

/*****************************************************************************/

void
mb_init_from_multiboot (void *mbi_ptr) /*cppcheck-suppress unusedFunction*/
{
  const struct multiboot_info *mbi = (struct multiboot_info *)mbi_ptr;
  uint64_t top = 0;

  /* Basic mem info */
  if (mbi && (mbi->flags & (1U << 0)))
    {
      if (mbi->mem_upper > 0)
        {
          /* mem_lower + mem_upper, starts at 1MB */
          top = 0x100000ULL + ((uint64_t)mbi->mem_upper * 1024ULL);
        }
      else
        {
          /* Only low memory exists */
          top = (uint64_t)mbi->mem_lower * 1024ULL;
        }
    }

  /* Full memory map if present (bit 6) - required for 4G */

  if (mbi && (mbi->flags & (1U << 6)))
    {
      uint32_t p = mbi->mmap_addr;
      uint32_t end = p + mbi->mmap_length;
      uint64_t run_top = 0;
      int have_run = 0;

      while (p < end)
        {
          const struct multiboot_mmap_entry *e
              = (struct multiboot_mmap_entry *)(uintptr_t)p;
          if (e->size == 0)
            {
              break;
            }

          uint64_t eend = e->addr + e->len;

          if (!have_run)
            {
              if (e->type == 1 && eend > 0x100000ULL)
                {
                  run_top = eend;
                  have_run = 1;
                }
            }
          else
            {
              uint64_t ebase = e->addr;

              if (ebase != run_top || e->type != 1)
                {
                  break;
                }
              run_top = eend;
            }

          p += e->size + sizeof (uint32_t);
        }

      if (have_run)
        {
          top = run_top;
        }
    }

  /* Clamp for 32-bit flat mode (match old loader behaviour) */

  if (top > 0xFFFFFFFFULL || top == 0)
    {
      top = 0xFFFFFFFFULL;
    }

  /*
   * Write legacy locations so the rest of the OS
   * (bios_getmrt, bgetseg, set_tpa etc.) works
   */

  *LEGACY_MEMTOP = (uint32_t)top;

  /*
   * Choose TPA base like the bootload logic:
   * after kernel + small stack reserve
   */

  extern char __kernel_end [];
  uint32_t k_end = (uint32_t)(uintptr_t)&__kernel_end;
  uint32_t stack_res = 0x4000;
  uint32_t tpa_base = k_end + stack_res + 0x1000;

  if (top != 0xFFFFFFFFULL && (uint64_t)tpa_base + 0x1000ULL > top)
    {
      tpa_base = (top > (uint64_t)k_end + 0x1000ULL)
                      ? (uint32_t)top - 0x1000
                      : k_end;
    }

  *LEGACY_TPA_BASE = tpa_base;
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
