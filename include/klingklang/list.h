#ifndef KK_LIST_H
#define KK_LIST_H

#include <klingklang/base.h>

/**
 * Pass to kk_list_sort to compare list items. Return a value
 *    = 0 if items are equal
 *    < 0 if first item < second item
 *    > 0 if first item > second item
 */
typedef int (*kk_list_cmp_f) (const void *, const void *);

typedef struct kk_list_s kk_list_t;

struct kk_list_s {
  size_t len;
  size_t cap;
  void **items;
};

int kk_list_init (kk_list_t ** list);
int kk_list_free (kk_list_t * list);

int kk_list_is_empty (kk_list_t * list);
int kk_list_is_filled (kk_list_t * list);

int kk_list_append (kk_list_t * list, void *item);
int kk_list_sort (kk_list_t * list, kk_list_cmp_f cmp);

#endif
