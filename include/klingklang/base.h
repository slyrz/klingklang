#ifndef KK_BASE_H
#define KK_BASE_H

#ifdef HAVE_CONFIG_H
#  include <klingklang/config.h>
#endif

#ifdef STDC_HEADERS
#  include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif

#ifdef HAVE_STDBOOL_H
#  include <stdbool.h>
#else
   typedef enum { false, true } bool;
#endif

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#ifdef HAVE_STRING_H
#  include <string.h>
#endif

/* Honestly, this might be totally useless... */
#ifndef INT32_MAX
#  if (defined __WORDSIZE) && (__WORDSIZE == 64)
#    define INT32_MAX INT_MAX
     typedef signed int int32_t;
#  else
#    define INT32_MAX LONG_MAX
     typedef signed long int int32_t;
#  endif
#endif

#ifndef INT64_MAX
#  if (defined __WORDSIZE) && (__WORDSIZE == 64)
#    define INT64_MAX LONG_MAX
     typedef signed long int int32_t;
#  else
#    define INT64_MAX LLONG_MAX
     typedef signed long long int int64_t;
#  endif
#endif

#ifndef UINT8_MAX
#  define UINT8_MAX UCHAR_MAX
     typedef unsigned char uint8_t;
#endif

#ifndef UINT16_MAX
#  define UINT16_MAX USHRT_MAX
     typedef unsigned short uint16_t;
#endif

#ifndef UINT32_MAX
#  if (defined __WORDSIZE) && (__WORDSIZE == 64)
#    define UINT32_MAX UINT_MAX
     typedef unsigned int uint32_t;
#  else
#    define UINT32_MAX ULONG_MAX
     typedef unsigned long int uint32_t;
#  endif
#endif

#ifndef UINT64_MAX
#  if (defined __WORDSIZE) && (__WORDSIZE == 64)
#    define UINT64_MAX ULONG_MAX
     typedef unsigned long int uint32_t;
#  else
#    define UINT64_MAX ULLONG_MAX
     typedef unsigned long long int uint64_t;
#  endif
#endif

#endif
