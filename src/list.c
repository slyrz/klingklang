#include <klingklang/list.h>
#include <klingklang/util.h>

#ifdef HAVE_QSORT_R
#  ifdef HAVE_QSORT_R_GNU
static int _kk_list_compare (const void *a, const void *b, void *arg);
#  else
static int _kk_list_compare (void *arg, const void *a, const void *b);
#  endif
#else
static int _kk_list_compare (const void *a, const void *b);
#endif

static int _kk_list_enlarge (kk_list_t *list);

#ifndef HAVE_QSORT_R
#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static void *arg = NULL;
#endif

static int
#ifdef HAVE_QSORT_R
#  ifdef HAVE_QSORT_R_GNU
_kk_list_compare (const void *a, const void *b, void *arg)
#  else
_kk_list_compare (void *arg, const void *a, const void *b)
#  endif
#else
_kk_list_compare (const void *a, const void *b)
#endif
{
  /**
   * kk_list_t stores void* pointers. The qsort function passes pointers to
   * these pointers to this function. So the underlying parameter type
   * is actually void** and not void*.
   * But since the qsort compare function prototype expects void* parameters,
   * we declared them as void* here. Then we do the necessary type casting to
   * cast them to actual void* pointers, before we pass them to the
   * user-defined compare function.
   */
  return ((kk_list_cmp_f) arg) (*((const void *const *) a), *((const void *const *) b));
}

static int
_kk_list_enlarge (kk_list_t *list)
{
  const size_t cap = kk_get_next_pow2 (list->cap);

  /* Sanity check ... who needs 32k list items? */
  if (cap > 0x8000)
    return -1;

  list->items = realloc (list->items, cap *sizeof (void *));
  if (list->items == NULL)
    return -1;

  list->cap = cap;
  return 0;
}

int
kk_list_init (kk_list_t **list)
{
  kk_list_t *result;

  result = calloc (1, sizeof (kk_list_t));
  if (result == NULL)
    goto error;

  /**
   * Start with a default capacity of 32 items. The list capacity grows by
   * powers of 2 whenever the current capacity is not large enough to hold
   * the required number of list items.
   */
  result->cap = 32;
  result->len = 0;

  result->items = calloc (result->cap, sizeof (void *));
  if (result->items == NULL)
    goto error;

  *list = result;
  return 0;
error:
  kk_list_free (result);
  *list = NULL;
  return -1;
}

int
kk_list_free (kk_list_t *list)
{
  if (list == NULL)
    return 0;
  free (list->items);
  free (list);
  return 0;
}

int
kk_list_is_empty (kk_list_t *list)
{
  return (list->len == 0);
}

int
kk_list_is_filled (kk_list_t *list)
{
  return (list->len > 0);
}

int
kk_list_append (kk_list_t *list, void *item)
{
  if (list->len >= list->cap) {
    if (_kk_list_enlarge (list) != 0)
      return -1;
  }
  list->items[list->len++] = item;
  return 0;
}

int
kk_list_sort (kk_list_t *list, kk_list_cmp_f cmp)
{
  if (list->len <= 1)
    return 0;

#ifdef HAVE_QSORT_R
#  ifdef HAVE_QSORT_R_GNU
  qsort_r (list->items, list->len, sizeof (void *), _kk_list_compare, cmp);
#  else
  qsort_r (list->items, list->len, sizeof (void *), cmp, _kk_list_compare);
#  endif
#else
  pthread_mutex_lock (&mutex);
  arg = cmp;
  qsort (list->items, list->len, sizeof (void *), _kk_list_compare);
  arg = NULL;
  pthread_mutex_unlock (&mutex);
#endif

  return 0;
}
