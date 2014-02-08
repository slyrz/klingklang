#-----------------------------------------------------------------------------
# Autoconf Requirements
#-----------------------------------------------------------------------------
AC_PREREQ(2.61)
AC_INIT([klingklang],[0.1])
AC_LANG([C])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([include/klingklang/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign subdir-objects silent-rules])
AM_MAINTAINER_MODE([enable])

m4_include([m4/ax_compiler_vendor.m4])
m4_include([m4/ax_check_compile_flag.m4])

#-----------------------------------------------------------------------------
# Define Package Options
#-----------------------------------------------------------------------------
AC_ARG_WITH([backend], AS_HELP_STRING([--with-backend=alsa|ao|oss|portaudio|pulseaudio|sndio], [use audio backend]), [
  AS_IF([test "x$with_backend" = "xalsa"], [backend="alsa"])
  AS_IF([test "x$with_backend" = "xao"], [backend="ao"])
  AS_IF([test "x$with_backend" = "xoss"], [backend="oss"])
  AS_IF([test "x$with_backend" = "xportaudio"], [backend="portaudio"])
  AS_IF([test "x$with_backend" = "xpulseaudio"], [backend="pulseaudio"])
  AS_IF([test "x$with_backend" = "xsndio"], [backend="sndio"])
])

AC_ARG_ENABLE([logging], AS_HELP_STRING([--enable-logging], [compile with logging functionality]), [
  AS_IF([test "x$enable_logging" = "xyes"], [logging="yes"], [logging="no"])
], [logging="no"])

AC_ARG_ENABLE([debugging], AS_HELP_STRING([--enable-debugging], [compile with debugging functionality]), [
  AS_IF([test "x$enable_debugging" = "xyes"], [debugging="yes"], [debugging="no"])
], [debugging="no"])

#-----------------------------------------------------------------------------
# Handle Package Options
#-----------------------------------------------------------------------------
AS_IF([test "x$logging" = "x"], AC_DEFINE(LOGGING, [1], [Compile with logging support]))
AS_IF([test "x$debugging" = "xyes"], AC_DEFINE(DEBUGGING, [1], [Compile with debug support]))
AS_IF([test "x$debugging" = "xyes"], [
  CFLAGS=`echo "-g $CFLAGS" | sed -e "s/-O[0-9]//"`
])

AM_CONDITIONAL([DEBUGGING], [test "x$debugging" = "xyes"])

#-----------------------------------------------------------------------------
# Checks For Compiler And Friends
#-----------------------------------------------------------------------------
AC_PROG_CC([gcc clang])
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_USE_SYSTEM_EXTENSIONS

PKG_PROG_PKG_CONFIG

#-----------------------------------------------------------------------------
# Checks For Libraries
#-----------------------------------------------------------------------------
AC_SEARCH_LIBS([timer_create],[rt posix4])
AC_SEARCH_LIBS([fabs],[m])

PKG_CHECK_MODULES([libavcodec], [libavcodec])
PKG_CHECK_MODULES([libavformat], [libavformat])
PKG_CHECK_MODULES([libavutil], [libavutil])
PKG_CHECK_MODULES([libswscale], [libswscale])

PKG_CHECK_MODULES([xcb], [xcb])
PKG_CHECK_MODULES([xcb_event], [xcb-event])
PKG_CHECK_MODULES([xcb_icccm], [xcb-icccm])

PKG_CHECK_MODULES([cairo], [cairo])
PKG_CHECK_MODULES([xkbcommon], [xkbcommon])
#-----------------------------------------------------------------------------
# Checks For Audio Backend
#-----------------------------------------------------------------------------

# Check which audio backends are present
AC_CHECK_HEADERS([sndio.h], [have_sndio="yes"], [have_sndio="no"])
AC_CHECK_HEADERS([sys/soundcard.h], [have_oss="yes"], [have_oss="no"])
PKG_CHECK_MODULES([libasound], [alsa], [have_alsa="yes"], [have_alsa="no"])
PKG_CHECK_MODULES([libao], [ao], [have_ao="yes"], [have_ao="no"])
PKG_CHECK_MODULES([portaudio], [portaudio-2.0], [have_portaudio="yes"], [have_portaudio="no"])
PKG_CHECK_MODULES([pulseaudio], [libpulse-simple], [have_pulseaudio="yes"], [have_pulseaudio="no"])

# We pick one of the available backends if the user didn't specify which one.
# Please note: these tests are ordered according to importance:
#
#   Cross Platform < Native < Sound Server
#
# This means native libaries overwrite cross platform libraries and
# sound server libraries overwrite native libraries.
AS_IF([test "x$backend" = "x"], [
  AS_IF([test "x$have_ao" = "xyes"], [backend="ao"])
  AS_IF([test "x$have_portaudio" = "xyes"], [backend="portaudio"])
  AS_IF([test "x$have_oss" = "xyes"], [backend="oss"])
  AS_IF([test "x$have_alsa" = "xyes"], [backend="alsa"])
  AS_IF([test "x$have_sndio" = "xyes"], [backend="sndio"])
  AS_IF([test "x$have_pulseaudio" = "xyes"], [backend="pulseaudio"])
])

AS_IF([test "x$backend" = "x"], [
  AC_MSG_ERROR([no audio backend detected.])
])

AM_CONDITIONAL([BACKEND_ALSA], [test "x$backend" = "xalsa"])
AM_CONDITIONAL([BACKEND_AO], [test "x$backend" = "xao"])
AM_CONDITIONAL([BACKEND_OSS], [test "x$backend" = "xoss"])
AM_CONDITIONAL([BACKEND_PORTAUDIO], [test "x$backend" = "xportaudio"])
AM_CONDITIONAL([BACKEND_PULSEAUDIO], [test "x$backend" = "xpulseaudio"])
AM_CONDITIONAL([BACKEND_SNDIO], [test "x$backend" = "xsndio"])

#-----------------------------------------------------------------------------
# Checks For Header Files
#-----------------------------------------------------------------------------
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([time.h])

AC_CHECK_HEADERS([libavcodec/avcodec.h])
AC_CHECK_HEADERS([libavformat/avformat.h])
AC_CHECK_HEADERS([libavutil/avutil.h])
AC_CHECK_HEADERS([libavutil/dict.h])
AC_CHECK_HEADERS([xcb/xcb.h])
AC_CHECK_HEADERS([xcb/xcb_event.h])
AC_CHECK_HEADERS([xcb/xcb_icccm.h])
AC_CHECK_HEADERS([xcb/xcb_keysyms.h])

#-----------------------------------------------------------------------------
# Checks For Types
#-----------------------------------------------------------------------------
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

#-----------------------------------------------------------------------------
# Checks For Compiler Characteristics
#-----------------------------------------------------------------------------
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT

AX_COMPILER_VENDOR
AS_IF([test "x$debugging" = "xyes"], [
  AS_CASE([x$ax_cv_c_compiler_vendor],
    [xgnu], [
      AX_CHECK_COMPILE_FLAG([-Wdouble-promotion], [vendor_flags="$vendor_flags -Wdouble-promotion" ])
      AX_CHECK_COMPILE_FLAG([-Wjump-misses-init], [vendor_flags="$vendor_flags -Wjump-misses-init" ])
      AX_CHECK_COMPILE_FLAG([-Wlogical-op], [vendor_flags="$vendor_flags -Wlogical-op" ])
      AX_CHECK_COMPILE_FLAG([-Wmaybe-uninitialized], [vendor_flags="$vendor_flags -Wmaybe-uninitialized" ])
    ],
    [xclang], [
      AX_CHECK_COMPILE_FLAG([-Wcovered-switch-default], [vendor_flags="$vendor_flags -Wcovered-switch-default" ])
    ],
    [])
])
AC_SUBST([VENDOR_FLAGS], [$vendor_flags])

#-----------------------------------------------------------------------------
# Checks For Library Functions
#-----------------------------------------------------------------------------
AC_FUNC_FORK
AC_FUNC_REALLOC

AC_CHECK_FUNCS([memset], [AC_DEFINE([HAVE_MEMSET], [1], [Define to 1 if you have the memset function])])
AC_CHECK_FUNCS([select], [AC_DEFINE([HAVE_SELECT], [1], [Define to 1 if you have the select function])])
AC_CHECK_FUNCS([strchr], [AC_DEFINE([HAVE_STRCHR], [1], [Define to 1 if you have the strchr function])])
AC_CHECK_FUNCS([strdup], [AC_DEFINE([HAVE_STRDUP], [1], [Define to 1 if you have the strdup function])])
AC_CHECK_FUNCS([strlcat], [AC_DEFINE([HAVE_STRLCAT], [1], [Define to 1 if you have the strlcat function])])
AC_CHECK_FUNCS([strlcpy], [AC_DEFINE([HAVE_STRLCPY], [1], [Define to 1 if you have the strlcpy function])])
AC_CHECK_FUNCS([strncasecmp], [AC_DEFINE([HAVE_STRNCASECMP], [1], [Define to 1 if you have the strncasecmp function])])
AC_CHECK_FUNCS([strndup], [AC_DEFINE([HAVE_STRNDUP], [1], [Define to 1 if you have the strndup function])])
AC_CHECK_FUNCS([strnlen], [AC_DEFINE([HAVE_STRNLEN], [1], [Define to 1 if you have the strnlen function])])
AC_CHECK_FUNCS([strrchr], [AC_DEFINE([HAVE_STRRCHR], [1], [Define to 1 if you have the strrchr function])])
AC_CHECK_FUNCS([strstr], [AC_DEFINE([HAVE_STRSTR], [1], [Define to 1 if you have the strstr function])])
AC_CHECK_FUNCS([qsort_r], [
  AC_DEFINE([HAVE_QSORT_R], [1], [Define to 1 if you have the qsort_r function])
  AC_MSG_CHECKING([for qsort_r style])
  AC_RUN_IFELSE([
    AC_LANG_PROGRAM([[
      #define _GNU_SOURCE
      #include <stdlib.h>
      static int values[2] = { 0, 1 };
      int
      cmp (void *a, void *b, void *c)
      {
        return (a == (void *) cmp);
      }
    ]],
    [[
      qsort_r (values, 2, sizeof (int), cmp, cmp);
      return values[0];
    ]])
  ], [
    AC_MSG_RESULT([GNU style])
    AC_DEFINE([HAVE_QSORT_R_GNU], [1], [Define to 1 if you have the GNU style qsort_r function])
  ], [
    AC_MSG_RESULT([BSD style])
    AC_DEFINE([HAVE_QSORT_R_BSD], [1], [Define to 1 if you have the BSD style qsort_r function])
  ])
])

# The xcb-icccm versions < 0.3.8 provide functions without icccm prefix.
# Use PKG_CHECK... instead of AC_CHECK... because the default search path might
# not contain the library.
PKG_CHECK_EXISTS([xcb-icccm >= 0.3.8], [
  AC_DEFINE([HAVE_XCB_ICCCM_PREFIX], [1], [Define to 1 if xcb-icccm function prefix is "xcb_icccm_"])
])

AC_MSG_CHECKING([for usable per-process timers])
AC_RUN_IFELSE([
  AC_LANG_PROGRAM([[
    #include <errno.h>
    #include <signal.h>
    #include <time.h>
  ]],
  [[
    struct sigevent s;
    timer_t t;
    errno = 0;
    timer_create (CLOCK_REALTIME, &s, &t);
    return ((errno == ENOTSUP) | (errno == ENOSYS));
  ]])
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_POSIX_TIMER_API], [1], [Define to 1 if you have usable timer_create, timer_delete and timer_settime functions])
], [
  AC_MSG_RESULT([no])
])

#-----------------------------------------------------------------------------
# End
#-----------------------------------------------------------------------------
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        ==============

        prefix: ............... ${prefix}
        sysconfdir: ........... ${sysconfdir}
        libdir: ............... ${libdir}
        includedir: ........... ${includedir}

        compiler: ............. ${CC}
        cflags: ............... ${CFLAGS}

        backend: .............. ${backend}
        logging: .............. ${logging}
        debugging: ............ ${debugging}
])
