#ifndef KK_STR_H
#define KK_STR_H

#include <klingklang/base.h>

typedef uint64_t kk_str_match_t;
typedef struct kk_str_pattern_s kk_str_pattern_t;
typedef struct kk_str_search_s kk_str_search_t;

struct kk_str_pattern_s {
  uint32_t h;
  size_t l;
  unsigned char *s;
};

struct kk_str_search_s {
  uint64_t bloom;
  size_t cap;
  size_t len;
  size_t m;
  kk_str_pattern_t pattern[];
};

int kk_str_search_init (kk_str_search_t **search, const char *pattern, const char *delim);
int kk_str_search_free (kk_str_search_t *search);
int kk_str_search_find_any (kk_str_search_t *search, const char *haystack, kk_str_match_t *match);
int kk_str_search_find_all (kk_str_search_t *search, const char *haystack, kk_str_match_t *match);
int kk_str_search_matches_any (kk_str_search_t *search, kk_str_match_t match);
int kk_str_search_matches_all (kk_str_search_t *search, kk_str_match_t match);

size_t kk_str_cat (char *dst, const char *src, size_t len);
size_t kk_str_cpy (char *dst, const char *src, size_t len);
size_t kk_str_len (const char *src, size_t len);

int kk_str_natcmp (const char *str1, const char *str2);

#endif
