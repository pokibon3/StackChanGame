#include <stdlib.h>

char *itoa(int value, char *str, int base)
{
    char *out = str;
    char *start = str;
    unsigned int magnitude;

    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    if (value < 0 && base == 10) {
        *out++ = '-';
        start = out;
        magnitude = (unsigned int)(-(value + 1)) + 1U;
    } else {
        magnitude = (unsigned int)value;
    }

    do {
        unsigned int digit = magnitude % (unsigned int)base;
        *out++ = (char)(digit < 10 ? ('0' + digit) : ('a' + digit - 10));
        magnitude /= (unsigned int)base;
    } while (magnitude != 0U);

    *out = '\0';

    for (char *left = start, *right = out - 1; left < right; ++left, --right) {
        char tmp = *left;
        *left = *right;
        *right = tmp;
    }

    return str;
}
