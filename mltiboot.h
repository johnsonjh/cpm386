/*
 * CP/M-386 - mltiboot.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 1833f84a-82b5-11f1-85c2-80ee73e9b8e7
 */

/*****************************************************************************/

/* mltiboot.h - multiboot 1 definitions for CP/M-386 */

/*****************************************************************************/

#ifndef MULTIBOOT_H
# define MULTIBOOT_H

/*****************************************************************************/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;

/*****************************************************************************/

# define MULTIBOOT_HEADER_MAGIC 0x1BADB002U
# define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002U

/*****************************************************************************/

/* flags in multiboot header */
# define MULTIBOOT_HEADER_FLAG_ALIGN (1U << 0)
# define MULTIBOOT_HEADER_FLAG_MEMINFO (1U << 1)
/* 0x00000003 = align + meminfo */

/*****************************************************************************/

struct multiboot_header
{
  uint32_t magic;
  uint32_t flags;
  uint32_t checksum;
  uint32_t header_addr;
  uint32_t load_addr;
  uint32_t load_end_addr;
  uint32_t bss_end_addr;
  uint32_t entry_addr;
  uint32_t mode_type;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
} __attribute__ ((packed));

/*****************************************************************************/

/* Multiboot information structure passed in EBX, magic in EAX */
struct multiboot_info
{
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms [4]; /* union of a.out/elf symbols */
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
} __attribute__ ((packed));

/*****************************************************************************/

/* memory map entry (when flags & (1<<6)) */
struct multiboot_mmap_entry
{
  uint32_t size; /* size of the rest of the entry (not including this field) */
  uint64_t addr;
  uint64_t len;
  uint32_t type; /* 1 = available, 2 = reserved, etc. */
} __attribute__ ((packed));

/*****************************************************************************/

/* init function */
void mb_init_from_multiboot (void *mbi_ptr);

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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
