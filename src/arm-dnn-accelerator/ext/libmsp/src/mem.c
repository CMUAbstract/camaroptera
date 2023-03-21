#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    size_t i = 0;
    while (i < n) {
        *((char *)dest + i) = *((char *)src + i);
        ++i;
    }
    return dest;
}

#if 0 // we cannot override memset because it is called by GCC's C runtime
void *memset(void *s, int c, size_t n)
{
    size_t i = 0;
    while (i < n) {
        *((char *)s + i) = (char)c;
        ++i;
    }
    return s;
}
#endif
