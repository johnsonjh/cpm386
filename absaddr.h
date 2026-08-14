/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 0778656e-82b4-11f1-b3d6-80ee73e9b8e7
 */

/*****************************************************************************/

#ifndef ABSADDR_H
# define ABSADDR_H

/*****************************************************************************/

static inline void *
abs_ptr (unsigned long addr) /*cppcheck-suppress unusedFunction*/
{
  void *p = (void *)addr;

  /*LINTED E_ASM_UNUSED_PARAM*/
  __asm__ ("" : "+r"(p));

  return p;
}

/*****************************************************************************/

# define ABS_U8(a) ((volatile unsigned char *)abs_ptr ((unsigned long)(a)))
# define ABS_U16(a) ((volatile unsigned short *)abs_ptr ((unsigned long)(a)))
# define ABS_U32(a) ((volatile unsigned int *)abs_ptr ((unsigned long)(a)))

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
