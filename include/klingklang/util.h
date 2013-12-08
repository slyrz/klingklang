#ifndef KK_UTIL_H
#define KK_UTIL_H

#include <klingklang/base.h>

enum {
  KK_LOG_ATTACH,
  KK_LOG_DEBUG,
  KK_LOG_INFO,
  KK_LOG_WARNING,
  KK_LOG_ERROR,
};

void kk_log (int level, const char *fmt, ...);
void kk_err (int status, const char *fmt, ...);

size_t kk_get_next_pow2 (size_t val);

#endif
