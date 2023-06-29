
#pragma once


//@todo-rco: Remove STL!
#include <ctype.h>
#include <stdarg.h>


/**
 * Get standard integer types
 */
#include <stdint.h>

/**
 * Functions that have different names in Visual Studio
 */
#ifndef _MSC_VER
#define strnicmp strncasecmp
#define stricmp strcasecmp
#define _strdup strdup
#endif

/**
 * Function inlining
 */
#ifndef inline
#  ifdef __cplusplus
	 /* C++ supports inline keyword */
#  elif defined(__GNUC__)
#    define inline __inline__
#  elif defined(_MSC_VER)
#    define inline __inline
#  elif defined(__ICL)
#    define inline __inline
#  elif defined(__INTEL_COMPILER)
	 /* Intel compiler supports inline keyword */
#  elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#    define inline __inline
#  elif defined(__SUNPRO_C) && defined(__C99FEATURES__)
	 /* C99 supports inline keyword */
#  elif (__STDC_VERSION__ >= 199901L)
	 /* C99 supports inline keyword */
#  else
#    define inline
#  endif
#endif
#ifndef INLINE
#  define INLINE inline
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_LOG2E
#define M_LOG2E     (1.4426950408889634074)
#endif

/**
 * USE_IEEE: Determine if we're using IEEE floating point
 */
#if defined(__i386__) || defined(__386__) || defined(__sparc__) || \
	defined(__s390x__) || defined(__powerpc__) || \
	defined(__x86_64__) || \
	defined(ia64) || defined(__ia64__) || \
	defined(__hppa__) || defined(hpux) || \
	defined(__mips) || defined(_MIPS_ARCH) || \
	defined(__arm__) || \
	defined(__sh__) || defined(__m32r__) || \
	(defined(__sun) && defined(_IEEE_754)) || \
	(defined(__alpha__) && (defined(__IEEE_FLOAT) || !defined(VMS)))
#define USE_IEEE
#define IEEE_ONE 0x3f800000
#endif

#ifdef _MSC_VER
#	define  GetNumArrayElements(x)		_countof(x)
#else
#	ifndef GetNumArrayElements
#		define GetNumArrayElements(x)	(sizeof(x)/sizeof(*(x)))
#	endif
#endif
/*
// Wouldn't compile on VS2015, complains about non-constant size on local arrays a[GetNumArrayElements(b)]
template <typename T, std::size_t N>
constexpr std::size_t GetNumArrayElements(T const(&)[N])
{
	return N;
}
*/
