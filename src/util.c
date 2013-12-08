/**
 * This file incorporates modified versions of work covered by the 
 * following copyright and permission notice:
 *
 * kk_get_next_pow2
 * ----------------
 * Released into public domain 2001 Sean Eron Anderson <seander@cs.stanford.edu>
 * http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 */

#include <klingklang/util.h>

#include <stdarg.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h> /* getpid */
#endif
/**
 * Definition of some terminal color codes
 */
#define KK_COL_NORM     "\x1b[0m"
#define KK_COL_RED      "\x1b[31;1m"
#define KK_COL_GREEN    "\x1b[32;1m"
#define KK_COL_BLUE     "\x1b[34;1m"
#define KK_COL_YELLOW   "\x1b[33;1m"

#define KK_COL(s)       KK_COL_ ## s
#define KK_COL_STR(s,c) KK_COL(c) s KK_COL(NORM)

static const int log_level = KK_LOG_DEBUG;

static const char *log_level_str[] = {
  [KK_LOG_DEBUG]   = KK_COL_STR ("dbg", BLUE),
  [KK_LOG_INFO]    = KK_COL_STR ("inf", GREEN),
  [KK_LOG_WARNING] = KK_COL_STR ("wrn", YELLOW),
  [KK_LOG_ERROR]   = KK_COL_STR ("err", RED),
};

static inline void
_kk_vlogf (int level, const char *fmt, va_list args)
{
  static pid_t pid = 0;
  static char info[64] = { '\0' };
  static int ignore = 0;

  if (((level != KK_LOG_ATTACH) && (level < log_level)) || ((level == KK_LOG_ATTACH) && (ignore))) {
    ignore = 1;
    return;
  }

  if (level != KK_LOG_ATTACH) {
    if (pid == 0)
      pid = getpid ();
    snprintf (info, 64, "[ " PACKAGE ":%d ~ %s ]", pid, log_level_str[level]);
  }
  else {
    info[0] = info[1] = info[2] = '.';
    info[3] = '\0';
  }

  fprintf (stderr, "%s ", info);
  vfprintf (stderr, fmt, args);
  fputc ('\n', stderr);
  ignore = 0;
}

void
kk_log (int level, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  _kk_vlogf (level, fmt, args);
  va_end (args);
}

void
kk_err (int status, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  _kk_vlogf (KK_LOG_ERROR, fmt, args);
  va_end (args);
  exit (status);
}

size_t 
kk_get_next_pow2 (size_t val)
{
  val |= val >> 1;
  val |= val >> 2;
  val |= val >> 4;
  val |= val >> 8;
  val |= val >> 16;
  return ++val;
}
