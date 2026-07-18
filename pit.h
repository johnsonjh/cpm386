/* pit.h - 8253/8254 PIT */

/*****************************************************************************/

#ifndef PIT_H
# define PIT_H

/*****************************************************************************/

# define PIT_HZ 1193182UL

/*****************************************************************************/

void pit_init (void);
void pit_poll (void);
void pit_read (unsigned long *lo, unsigned long *hi);
void pit_sleep_until (unsigned long lo, unsigned long hi);

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
