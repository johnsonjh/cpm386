/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 827d6232-82b4-11f1-a540-80ee73e9b8e7
 */

/*****************************************************************************/

/* bringup.h - shared declarations for bring-up */

/*****************************************************************************/

#ifndef CPM_BRINGUP_H
# define CPM_BRINGUP_H

/*****************************************************************************/

# include "bdosinc.h"
# include "bdosdef.h" /* struct dpb, struct dph */

/*****************************************************************************/

/*
 * 256 KiB image: enough for multi-extent and >64K .386 test programs
 */

/*****************************************************************************/

# ifdef RAMDISK_KB
#  define RAMDISK_SIZE (RAMDISK_KB * 1024)
# else
#  error RAMDISK_KB undefined
# endif

/*****************************************************************************/

extern unsigned char ramdisk [RAMDISK_SIZE];
extern struct dph dph0;
extern struct dpb dpb0;

/*****************************************************************************/

void cpm_bringup (void);

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
