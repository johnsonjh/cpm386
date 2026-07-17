/* vgauser.h - ring-3 interface to VGA text memory */

#ifndef VGA_USER_H
# define VGA_USER_H

# define BDOS_CON_VIDEO 224

/* Filled by BDOS 224 */
struct cpm_vga_text
{
  unsigned short sel;        /* GDT selector|RPL3 or 0 */
  unsigned short cols;       /* 80                     */
  unsigned short rows;       /* 25                     */
  unsigned short cell_bytes; /* 2                      */
  unsigned long map_size;    /* bytes mapped (32K)     */
  unsigned long phys_base;   /* physical base          */
};

/* ES:offset store of one text cell (char|attr<<8) */
static inline void
vga_es_store16 (unsigned short sel, unsigned off, unsigned short cell)
{
  __asm__ volatile ("movw %0, %%es\n\t"
                    "movw %2, %%es:(%1)\n\t"
                    :
                    : "r"(sel), "r"(off), "r"(cell)
                    : "memory");
}

/* ES:offset load of one text cell. */
static inline unsigned short
vga_es_load16 (unsigned short sel, unsigned off)
{
  unsigned short cell;

  __asm__ volatile ("movw %1, %%es\n\t"
                    "movw %%es:(%2), %0\n\t"
                    : "=r"(cell)
                    : "r"(sel), "r"(off)
                    : "memory");
  return cell;
}

#endif
