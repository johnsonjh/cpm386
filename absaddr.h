#ifndef ABSADDR_H
# define ABSADDR_H

static inline void *
abs_ptr (unsigned long addr)
{
  void *p = (void *)addr;

  __asm__ ("" : "+r"(p));
  return p;
}

# define ABS_U8(a) ((volatile unsigned char *)abs_ptr ((unsigned long)(a)))
# define ABS_U16(a) ((volatile unsigned short *)abs_ptr ((unsigned long)(a)))
# define ABS_U32(a) ((volatile unsigned int *)abs_ptr ((unsigned long)(a)))

#endif
