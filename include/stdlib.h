#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

#define abs(ival)		(ival < 0? -ival : ival)

void *malloc(size_t size);
void free(void *addr);
struct task;
void __free(void *addr, struct task *task);

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *src, int c, size_t len);

#endif /* __STDLIB_H__ */
