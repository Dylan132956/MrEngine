

#include <stdlib.h>

#ifdef _GNU_SOURCE
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

#include "strtod.h"



/**
 * Wrapper around strtod which uses the "C" locale so the decimal
 * point is always '.'
 */
double
glsl_strtod(const char *s, char **end)
{
#if defined(_GNU_SOURCE) && !defined(__CYGWIN__) && !defined(__FreeBSD__) && \
   !defined(__HAIKU__)
   static locale_t loc = NULL;
   if (!loc) {
      loc = newlocale(LC_CTYPE_MASK, "C", NULL);
   }
   return strtod_l(s, end, loc);
#else
   return strtod(s, end);
#endif
}
