/* platform.h - machine-dependent constants */

#ifndef PLATFORM_H
# define PLATFORM_H

# ifndef CPM386_HAS_VGA_TEXT
#  define CPM386_HAS_VGA_TEXT 1
# endif

# ifndef CPM386_VGA_TEXT_BASE
#  define CPM386_VGA_TEXT_BASE 0x000B8000UL
# endif

/* 32 KiB full VGA text plane */
# ifndef CPM386_VGA_TEXT_SIZE
#  define CPM386_VGA_TEXT_SIZE 0x8000UL
# endif

/* Logical geometry of the default page */
# ifndef CPM386_VGA_TEXT_COLS
#  define CPM386_VGA_TEXT_COLS 80
# endif

# ifndef CPM386_VGA_TEXT_ROWS
#  define CPM386_VGA_TEXT_ROWS 25
# endif

# ifndef CPM386_VGA_TEXT_CELL
#  define CPM386_VGA_TEXT_CELL 2 /* char + attribute */
# endif

#endif
