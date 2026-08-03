/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 61324b14-82b5-11f1-8b10-80ee73e9b8e7
 */

/*****************************************************************************/

/* platform.h - machine-dependent constants */

/*****************************************************************************/

#ifndef PLATFORM_H
# define PLATFORM_H

/*****************************************************************************/

# ifndef CPM386_HAS_VGA_TEXT
#  define CPM386_HAS_VGA_TEXT 1
# endif

/*****************************************************************************/

# ifndef CPM386_VGA_TEXT_BASE
#  define CPM386_VGA_TEXT_BASE 0x000B8000UL
# endif

/*****************************************************************************/

/* 32 KiB full VGA text plane */
# ifndef CPM386_VGA_TEXT_SIZE
#  define CPM386_VGA_TEXT_SIZE 0x8000UL
# endif

/*****************************************************************************/

/* Logical geometry of the default page */
# ifndef CPM386_VGA_TEXT_COLS
#  define CPM386_VGA_TEXT_COLS 80
# endif

/*****************************************************************************/

# ifndef CPM386_VGA_TEXT_ROWS
#  define CPM386_VGA_TEXT_ROWS 25
# endif

/*****************************************************************************/

# ifndef CPM386_VGA_TEXT_CELL
#  define CPM386_VGA_TEXT_CELL 2 /* char + attribute */
# endif

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
