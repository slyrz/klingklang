/**
 * This file incorporates modified versions of work covered by the 
 * following copyright and permission notice:
 * 
 * digit_count, digit_compare, kk_str_natcmp
 * -----------------------------------------
 * Copyright (c) 2011 Jan Varho <jan at varho dot org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * strlcpy, strlcat
 * ----------------
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <klingklang/str.h>

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#else
#  define isdigit(c) (((c) >= '0') & ((c) <= '9'))
#endif

/**
 * What is a Buzhash table?
 * Basically just random numbers satisfiying the following property: If you 
 * think of the array as 32 columns of bits, each column contains exactly 128
 * zeros and 128 ones in a random order.
 * 
 * A Python code to generate those constants might look like
 * 
 * >>> columns = [ shuffle([0] *128 + [1] * 128) for i in range(32) ]
 * ... for i in range(256):
 * ...   value = 0
 * ...   for j, column in enumerate(columns):
 * ...     value |= column.pop () << j
 * ...   print("0x{:08x}ul, ".format(value))
 * 
 * where shuffle is a function that shuffles list items. To perform 
 * case-insensitve search, we use a slighty modified Buzhash table:
 * The lowercase and uppercase values of an alphabetic character share the
 * same constants
 * 
 * buzhash_table[lower(C)] == buzhash_table[upper(C)] for C in 'a' ... 'z'
 * 
 * without breaking the 128 zeros / 128 ones per column property.
 */
static uint32_t buzhash_table[256] = {
  0xae56688cul, 0x99531140ul, 0x030f64acul, 0x519951ccul,
  0x835da0ccul, 0xb18f08bdul, 0xd0710294ul, 0xe66cc76cul,
  0x6c006d6bul, 0xdc70ffcbul, 0xfbf65331ul, 0x5656f7e3ul,
  0x8bdf5f61ul, 0x8ec48fa6ul, 0xf5b56a5ful, 0x86cade8cul,
  0xf3ea7915ul, 0x79d93d75ul, 0xf08a2706ul, 0x6170fd3eul,
  0x4aeedf1aul, 0x74627950ul, 0x681cb2a6ul, 0x5c6281b8ul,
  0x40ba779bul, 0x1ac5aedful, 0xd567f59eul, 0x10d58569ul,
  0x6768ac12ul, 0x8b6aef21ul, 0xa8536ac7ul, 0xae59f950ul,
  0x9dda2d56ul, 0x5a656ee2ul, 0x33ddeaa4ul, 0xa0426548ul,
  0x792fb8b6ul, 0xb094040dul, 0x1fe3ecbaul, 0x680f8dceul,
  0xae4b120bul, 0x90f8d7a9ul, 0xc7bb9c84ul, 0xe1cbde50ul,
  0x0a0cccfful, 0x1e9d44b4ul, 0x79e08311ul, 0x1463330cul,
  0x93a2de3aul, 0xab731a92ul, 0x02b2f865ul, 0xa48efbbcul,
  0x529fd95cul, 0x7e2cf1c3ul, 0x5ace3467ul, 0x96afd1e5ul,
  0x6d165c73ul, 0xa3e40723ul, 0xdf27fd14ul, 0x73a956d1ul,
  0x68988af8ul, 0x4c2b1282ul, 0x9f22fa43ul, 0x05f93272ul,
  0x300c0d01ul, 0xf129db59ul, 0xf8daefd3ul, 0xe437d1e1ul,
  0x137f89a0ul, 0x6e3b2267ul, 0x285b5627ul, 0x8294c6a9ul,
  0x86d7855aul, 0x9df7d66eul, 0x33c3c757ul, 0x7be0f49cul,
  0xae75e426ul, 0x9e3f25bful, 0x3d412518ul, 0x6f28ac44ul,
  0xb0f78454ul, 0x1b029a30ul, 0x79dbb1b7ul, 0xe57500c1ul,
  0x0bd79669ul, 0x2451a917ul, 0x8e98e5aaul, 0x056d56c2ul,
  0x7158d15bul, 0x2b4161eful, 0x0a04362dul, 0x213c69e7ul,
  0x64322e22ul, 0x942f1055ul, 0x2fe50298ul, 0x72c55745ul,
  0x1f996ee9ul, 0xf129db59ul, 0xf8daefd3ul, 0xe437d1e1ul,
  0x137f89a0ul, 0x6e3b2267ul, 0x285b5627ul, 0x8294c6a9ul,
  0x86d7855aul, 0x9df7d66eul, 0x33c3c757ul, 0x7be0f49cul,
  0xae75e426ul, 0x9e3f25bful, 0x3d412518ul, 0x6f28ac44ul,
  0xb0f78454ul, 0x1b029a30ul, 0x79dbb1b7ul, 0xe57500c1ul,
  0x0bd79669ul, 0x2451a917ul, 0x8e98e5aaul, 0x056d56c2ul,
  0x7158d15bul, 0x2b4161eful, 0x0a04362dul, 0x641eff80ul,
  0xca3904f4ul, 0xc3ad0c69ul, 0xee04978aul, 0xaf707848ul,
  0x91c97519ul, 0xc162ba9ful, 0xf5c85dc7ul, 0xcae2c69ful,
  0x5fddba8eul, 0x073d6ccbul, 0x07c0bd4bul, 0x04bbb6d0ul,
  0x26020f54ul, 0xeb741a75ul, 0x1b23e92cul, 0x791ea3e9ul,
  0x43febe50ul, 0xebf287a8ul, 0xf1fcab3aul, 0x2f6a50b6ul,
  0x21de8cb8ul, 0xcac20d4bul, 0x350f2be3ul, 0x378bf0d9ul,
  0x0ed5d3e3ul, 0x619c7c8ful, 0x296477cdul, 0x78a8367eul,
  0x90f7092ful, 0x3b55e467ul, 0x56e949b6ul, 0x6bb206b4ul,
  0xf9b67be4ul, 0x5d64cdbful, 0x900926c4ul, 0xdb3b6beful,
  0xc903b3d9ul, 0x4565d716ul, 0xa0cd3854ul, 0x565408f3ul,
  0x8a5bd169ul, 0x4a3554c6ul, 0x94b983d5ul, 0x0df113b0ul,
  0xe7fc4a09ul, 0xdfb8c8b7ul, 0xc46f2e61ul, 0x3fe84289ul,
  0x7980d20cul, 0xf880d11aul, 0x5bba09acul, 0xe367bbbcul,
  0xee290c9bul, 0xca43bd0aul, 0x9a958f74ul, 0xdbf56cd3ul,
  0xacad7ae0ul, 0xf3888fe4ul, 0xd687800aul, 0x160e5d73ul,
  0x6c7eb51cul, 0x18434798ul, 0x4202315cul, 0xa28676f4ul,
  0xa557dd74ul, 0xf72662c3ul, 0x3b99b817ul, 0x07ca43bbul,
  0x469c814ful, 0x7e3516eeul, 0x90de11baul, 0x912e998bul,
  0xb910b01bul, 0x24d7aa1dul, 0x5d522541ul, 0x08b4a6cdul,
  0xb0a3b81ful, 0x3b33a012ul, 0xc5c10231ul, 0x355185d6ul,
  0xa8f7d90bul, 0xa565fcbaul, 0x8dcfac96ul, 0x9f125e7bul,
  0x968ebcabul, 0x408fc9fbul, 0x20e27844ul, 0x1f117a1dul,
  0xf65623deul, 0xb78c03b6ul, 0x25a0f9e1ul, 0x0b2f211dul,
  0x5e9c7393ul, 0xfc004b79ul, 0x5faac6e2ul, 0xa0fd541ful,
  0xe982df45ul, 0x18adc29dul, 0x63ce10a6ul, 0xaf4a7c4cul,
  0x5024b426ul, 0x89b1c582ul, 0xa6bad136ul, 0x4dd5b07aul,
  0x552c02c8ul, 0xbdc32cfbul, 0x632ee33eul, 0x4d8b0776ul,
  0x527362b3ul, 0xef12f92eul, 0xd4ef3ea5ul, 0x4bc9cf80ul,
  0xf083ef4aul, 0x6f680942ul, 0x7d9434c2ul, 0xb53a1cfbul,
  0x9a89dbe4ul, 0x563d58bdul, 0xc62e0b55ul, 0xe238784ful,
  0x84803ad3ul, 0xd4b45ad1ul, 0xe4240b3ful, 0xcc302b20ul,
  0x84ac5fb4ul, 0xfc086a7dul, 0x941c0aa1ul, 0xdc942a28ul,
  0xd4963b39ul, 0xd43e3a2cul, 0xd4ac6b28ul, 0xd42c3b38ul,
};

#define rol32(v,s) (((v) << (s)) | ((v) >> (32 - (s))))

static inline void
buzhash_init (uint32_t *hash, const unsigned char *data, size_t len)
{
  size_t i;

  /**
   * Clang doesn't compute the right hash value if the last step get's pulled
   * into the loop (rol32(..., 0) is the problem).
   */
  *hash = 0ul;
  for (i = 1; i < len; i++, data++)
    *hash ^= rol32 (buzhash_table[(size_t) (*data)], len - i);
  *hash ^= buzhash_table[(size_t) (*data)];
}

static inline void
buzhash_update (uint32_t *hash, const unsigned char *data, size_t len)
{
  *hash = rol32 (*hash, 1) ^ rol32 (buzhash_table[(size_t) (*data)], len) ^ buzhash_table[(size_t) (*(data + len))];
}

static inline uint64_t
bloom_mask (uint32_t val)
{
  return (1ull << (val % 23)) | (1ull << (val % 47)) | (1ull << (val % 61));
}

static int
_kk_str_search_add (kk_str_search_t *search, const char *pattern)
{
  kk_str_pattern_t *pat;

  if (search->len >= search->cap)
    return -1;

  pat = search->pattern + search->len;
  pat->h = 0ul;
  pat->l = strlen (pattern);
  pat->s = (unsigned char *) strdup (pattern);

  if (search->len == 0)
    search->m = pat->l;
  else if (pat->l < search->m)
    search->m = pat->l;

  search->len++;
  return 0;
}

int
kk_str_search_init (kk_str_search_t **search, const char *pattern, const char *delim)
{
  kk_str_search_t *result = NULL;

  char *dup = NULL;
  char *tok = NULL;
  char *ptr = NULL;

  size_t cap = 1;
  size_t i;

  dup = strdup (pattern);
  if (dup == NULL)
    goto error;

  /* Count the number of patterns */
  if (delim == NULL) {
    cap = 1;
  }
  else {
    if (strlen (delim) > 1)
      goto error;

    tok = dup;
    while (ptr = strchr (tok, delim[0]), ptr >= tok) {
      cap = cap + (ptr > tok);
      tok = ptr + 1;
    }
  }

  result = calloc (1, sizeof (kk_str_search_t) + cap *sizeof (kk_str_pattern_t));
  if (result == NULL)
    goto error;
  result->cap = cap;
  result->len = 0;

  if (delim == NULL) {
    if (_kk_str_search_add (result, pattern) != 0)
      goto error;
  }
  else {
    tok = strtok_r (dup, delim, &ptr);
    while (tok != NULL) {
      if (*tok) {
        if (_kk_str_search_add (result, tok) != 0)
          goto error;
      }
      tok = strtok_r (NULL, delim, &ptr);
    }
  }

  if (result->len == 0)
    goto error;

  if (result->m > 4)
    result->m = 4;

  for (i = 0; i < result->len; i++) {
    buzhash_init (&result->pattern[i].h, result->pattern[i].s, result->m);
    result->bloom |= bloom_mask (result->pattern[i].h);
  }

  free (dup);
  *search = result;
  return 0;
error:
  kk_str_search_free (result);
  free (dup);
  *search = NULL;
  return -1;
}

int
kk_str_search_free (kk_str_search_t *search)
{
  size_t i;

  if (search == NULL)
    return 0;

  for (i = 0; i < search->len; i++)
    free (search->pattern[i].s);
  free (search);
  return 0;
}

static inline int
_kk_str_pattern_is_match (kk_str_pattern_t *pattern, const char *haystack, uint32_t h)
{
  return (pattern->h == h) && (strncasecmp (haystack, (const char *) pattern->s, pattern->l) == 0);
}

static inline int
_kk_str_search_find (kk_str_search_t *search, const char *haystack, kk_str_match_t *match, int return_on_first_match)
{
  uint64_t m;
  uint32_t h;
  size_t i;

  /* Clear results of previous calls */
  *match = 0ull;

  /* Make sure haystack isn't smaller than our minimum required length */
  for (i = 0; i < search->m; i++)
    if (haystack[i] == '\0')
      return 0;

  buzhash_init (&h, (const unsigned char *) haystack, search->m);
  for (;;) {
    m = bloom_mask (h);

    /**
     * Current position in haystack might math one of our patterns (needles).
     * We go through each pattern and check if it really matches.
     */
    if ((search->bloom & m) == m) {
      for (i = 0; i < search->len; i++)
        if (_kk_str_pattern_is_match (search->pattern + i, haystack, h)) {
          *match |= m;
          if ((return_on_first_match) || ((search->bloom & *match) == search->bloom))
            return 1;
        }
    }

    if (haystack[search->m] == '\0')
      break;

    buzhash_update (&h, (const unsigned char *) haystack, search->m);
    haystack++;
  }
  return 0;
}

int
kk_str_search_find_any (kk_str_search_t *search, const char *haystack, kk_str_match_t *match)
{
  return _kk_str_search_find (search, haystack, match, 1);
}

int
kk_str_search_find_all (kk_str_search_t *search, const char *haystack, kk_str_match_t *match)
{
  return _kk_str_search_find (search, haystack, match, 0);
}

int
kk_str_search_matches_any (kk_str_search_t *search, kk_str_match_t match)
{
  return ((match & search->bloom) > 0);
}

int
kk_str_search_matches_all (kk_str_search_t *search, kk_str_match_t match)
{
  return (match == search->bloom);
}

/**
 * Appends src to string dst of size len (unlike strncat, len is the
 * full size of dst, not space left).  At most len-1 characters
 * will be copied.  Always NUL terminates (unless len <= strlen(dst)).
 * Returns strlen(src) + MIN(len, strlen(initial dst)).
 * If retval >= len, truncation occurred.
 */
#ifndef HAVE_STRLCAT
static inline size_t
strlcat (char *dst, const char *src, size_t len)
{
  const char *s = src;
  char *d = dst;
  size_t n = len;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end */
  while (n-- != 0 && *d != '\0')
    d++;
  dlen = (size_t) (d - dst);
  n = len - dlen;

  if (n == 0)
    return dlen + strlen (s);

  while (*s != '\0') {
    if (n != 1) {
      *d++ = *s;
      n--;
    }
    s++;
  }
  *d = '\0';

  return dlen + (size_t) (s - src);     /* count does not include NUL */
}
#endif

size_t
kk_str_cat (char *dst, const char *src, size_t len)
{
  return strlcat (dst, src, len);
}

/**
 * Copy src to string dst of size len.  At most len-1 characters
 * will be copied.  Always NUL terminates (unless len == 0).
 * Returns strlen(src); if retval >= len, truncation occurred.
 */
#ifndef HAVE_STRLCPY
static inline size_t
strlcpy (char *dst, const char *src, size_t len)
{
  const char *s = src;
  char *d = dst;
  size_t n = len;

  /* Copy as many bytes as will fit */
  if (n != 0) {
    while (--n != 0) {
      if ((*d++ = *s++) == '\0')
        break;
    }
  }
  /* Not enough room in dst, add NUL and traverse rest of src */
  if (n == 0) {
    if (len != 0)
      *d = '\0';                /* NUL-terminate dst */
    while (*s++);
  }
  return (size_t) (s - src) - 1;        /* count does not include NUL */
}
#endif

size_t
kk_str_cpy (char *dst, const char *src, size_t len)
{
  return strlcpy (dst, src, len);
}

/**
 * Tries to determine strlen by looping over the first len bytes of str.
 * If no null byte is found, len is returned.
 */
#ifndef HAVE_STRNLEN
static inline size_t
strnlen (const char *str, size_t len) 
{
  size_t ret;

  ret = 0;
  while ((*str) & (ret < len)) {
    ret++;
    str++;
  }
  return ret;
}
#endif

size_t 
kk_str_len (const char *str, size_t len)
{
  return strnlen (str, len);
}

static size_t
digit_count (const char **p)
{
  const char *s;

  while (**p == '0')
    (*p)++;

  s = *p;
  while (isdigit (*s))
    s++;

  return (size_t) (s - *p);
}

static int
digit_compare (const char **p1, const char **p2)
{
  const size_t c1 = digit_count (p1);
  const size_t c2 = digit_count (p2);
  const size_t n = (c1 < c2) ? c1 : c2;

  int c;

  if (c1 != c2)
    return (int) (c1 - c2);

  if ((c = strncmp (*p1, *p2, n)) != 0)
    return c;

  *p1 += c1;
  *p2 += c2;
  return 0;
}

int
kk_str_natcmp (const char *s1, const char *s2)
{
  int c;

  while ((*s1) & (*s2)) {
    if (isdigit (*s1) && isdigit (*s2)) {
      while (isdigit (*s1) && isdigit (*s2)) {
        if ((c = digit_compare (&s1, &s2)))
          return c;
      }
    }
    else {
      if (*s1 != *s2)
        return (unsigned char) *s1 - (unsigned char) *s2;
      s1++;
      s2++;
    }
  }
  return 0;
}
