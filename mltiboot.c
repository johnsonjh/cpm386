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
#include "memmap.h"
#include "mltiboot.h"

/*****************************************************************************/

static uint64_t
mb_merge_mmap (const struct multiboot_info *mbi)
{
  uint64_t top = (uint64_t)TPA_MIN_BASE;
  uint32_t start = mbi->mmap_addr;
  uint32_t end = mbi->mmap_addr + mbi->mmap_length;
  int grew = 1;
  int passes = 0;

  while (grew && passes < 64)
    {
      uint32_t p = start;

      grew = 0;
      passes++;

      while (p < end)
        {
          const struct multiboot_mmap_entry *e
              = (const struct multiboot_mmap_entry *)(uintptr_t)p;
          uint64_t eend;

          if (e->size == 0)
            {
              break;
            }

          p += e->size + sizeof (uint32_t);

          if (e->type != 1 || e->addr > top)
            {
              continue;
            }

          eend = e->addr + e->len;

          if (eend > top)
            {
              top = eend;
              grew = 1;
            }
        }
    }

  return top;
}

/*****************************************************************************/

void
mb_init_from_multiboot (void *mbi_ptr) /*cppcheck-suppress unusedFunction*/
{
  const struct multiboot_info *mbi = (struct multiboot_info *)mbi_ptr;
  uint64_t top = (uint64_t)TPA_MIN_BASE;
  uint32_t flags = 0;

  if (mbi && (mbi->flags & (1U << 0)) && mbi->mem_upper > 0)
    {
      top = (uint64_t)TPA_MIN_BASE + ((uint64_t)mbi->mem_upper * 1024ULL);
      flags = MEMF_MBBASIC;
    }

  if (mbi && (mbi->flags & (1U << 6)) && mbi->mmap_length >= sizeof (uint32_t))
    {
      uint64_t mtop = mb_merge_mmap (mbi);

      if (mtop > (uint64_t)TPA_MIN_BASE)
        {
          top = mtop;
          flags = MEMF_MBMMAP;
        }
    }

  if (top > (uint64_t)TPA_MAX_TOP)
    {
      top = (uint64_t)TPA_MAX_TOP;
    }

  if (top < (uint64_t)TPA_MIN_BASE)
    {
      top = (uint64_t)TPA_MIN_BASE;
    }

  *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_BASE) = (uint32_t)TPA_MIN_BASE;
  *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_TOP) = (uint32_t)top;
  *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_FLAGS) = flags | MEMF_A20;
  *ABS_U32 (MEMMAP_ADDR + MEMMAP_OFF_MAGIC) = (uint32_t)MEMMAP_MAGIC;
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
