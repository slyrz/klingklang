#-----------------------------------------------------------------------------
# Autoconf Requirements
#-----------------------------------------------------------------------------
AC_PREREQ(2.61)

#-----------------------------------------------------------------------------
# Autoconf Requirements
#-----------------------------------------------------------------------------
AC_INIT([klingklang],[0.1])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([include/klingklang/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG([C])

m4_include([m4/ax_compiler_vendor.m4])
m4_include([m4/ax_check_compile_flag.m4])

AM_INIT_AUTOMAKE([foreign subdir-objects silent-rules])
AM_MAINTAINER_MODE([enable])

#-----------------------------------------------------------------------------
# Define Package Options
#-----------------------------------------------------------------------------
AC_ARG_WITH([backend], AS_HELP_STRING([--with-backend=ao|alsa], [use audio backend]), [
  AS_IF([test "x$with_backend" = "xalsa"], [backend="alsa"])
  AS_IF([test "x$with_backend" = "xao"], [backend="ao"])
  AS_IF([test "x$with_backend" = "xportaudio"], [backend="portaudio"])
  AS_IF([test "x$with_backend" = "xpulseaudio"], [backend="pulseaudio"])
])

# We really need this option
AS_IF([test "x$backend" = "x"], AC_MSG_ERROR([No audio backend specified. See --help]))

AC_ARG_ENABLE([logging], AS_HELP_STRING([--enable-logging], [compile with logging functionality]), [ 
  AS_IF([test "x$enable_logging" = "xyes"], [logging="yes"], [logging="no"])
], [logging="no"])

AC_ARG_ENABLE([debugging], AS_HELP_STRING([--enable-debugging], [compile with debugging functionality]), [ 
  AS_IF([test "x$enable_debugging" = "xyes"], [debugging="yes"], [debugging="no"])
], [debugging="no"])

#-----------------------------------------------------------------------------
# Handle Package Options
#-----------------------------------------------------------------------------
AS_IF([test "x$backend" = "x"], AC_MSG_ERROR([No audio backend specified. See --help]))
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
PKG_CHECK_MODULES([libavcodec], [libavcodec], [], AC_MSG_ERROR([libavcodec not found.]))
PKG_CHECK_MODULES([libavformat], [libavformat], [], AC_MSG_ERROR([libavformat not found.]))
PKG_CHECK_MODULES([libavutil], [libavutil], [], AC_MSG_ERROR([libavutil not found.]))
PKG_CHECK_MODULES([libswscale], [libswscale], [], AC_MSG_ERROR([libswscale not found.]))

PKG_CHECK_MODULES([xcb], [xcb], [], AC_MSG_ERROR([xcb not found.]))
PKG_CHECK_MODULES([xcb_keysyms], [xcb-keysyms], [], AC_MSG_ERROR([xcb-keysyms not found.]))
PKG_CHECK_MODULES([xcb_event], [xcb-event], [], AC_MSG_ERROR([xcb-event not found.]))
PKG_CHECK_MODULES([xcb_icccm], [xcb-icccm], [], AC_MSG_ERROR([xcb-icccm not found.]))

PKG_CHECK_MODULES([cairo], [cairo], [], AC_MSG_ERROR([cairo not found.]))

#-----------------------------------------------------------------------------
# Checks For Audio Backend
#-----------------------------------------------------------------------------
if test "x$backend" = "xalsa"; then
	PKG_CHECK_MODULES([libasound], [alsa], [], AC_MSG_ERROR([libasound not found.]))
fi

if test "x$backend" = "xao"; then
	PKG_CHECK_MODULES([libao], [ao], [], AC_MSG_ERROR([libao not found.]))
fi

if test "x$backend" = "xportaudio"; then
  PKG_CHECK_MODULES([portaudio], [portaudio-2.0], [], AC_MSG_ERROR([portaudio-2.0 not found.]))
fi

if test "x$backend" = "xpulseaudio"; then
  PKG_CHECK_MODULES([pulseaudio], [libpulse-simple], [], AC_MSG_ERROR([libpulse-simple not found.]))
fi

AM_CONDITIONAL([BACKEND_ALSA], [test "x$backend" = "xalsa"])
AM_CONDITIONAL([BACKEND_AO], [test "x$backend" = "xao"])
AM_CONDITIONAL([BACKEND_PORTAUDIO], [test "x$backend" = "xportaudio"])
AM_CONDITIONAL([BACKEND_PULSEAUDIO], [test "x$backend" = "xpulseaudio"])

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

OLFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -lrt"
AC_MSG_CHECKING([for usable per-process timers])
AC_RUN_IFELSE([
  AC_LANG_SOURCE([[
    #include <errno.h>
    #include <signal.h>
    #include <time.h>
    int
    main (void)
    {
      struct sigevent s;
      timer_t t;
      errno = 0;
      timer_create (CLOCK_REALTIME, &s, &t);
      return ((errno == ENOTSUP) | (errno == ENOSYS));
    }
  ]])
], [
  AC_MSG_RESULT([yes])
  AC_SUBST([timer_LIBS], [-lrt])
  AC_DEFINE([HAVE_POSIX_TIMER_API], [1], [Define to 1 if you have the strstr function])
], [
  AC_MSG_RESULT([no])
  AC_SUBST([timer_LIBS], [])
])
LDFLAGS="$OLFLAGS"

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
