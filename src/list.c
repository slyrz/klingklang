#include <klingklang/list.h>
#include <klingklang/util.h>

#ifdef HAVE_QSORT_R
static int _kk_list_compare (const void *a, const void *b, void *arg);
#else
static int _kk_list_compare (const void *a, const void *b);
#endif

static int _kk_list_enlarge (kk_list_t *list);

#ifndef HAVE_QSORT_R
#  include <pthread.h>
#endif

#ifndef HAVE_QSORT_R
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static void *arg = NULL;
#endif

static int
#ifdef HAVE_QSORT_R
_kk_list_compare (const void *a, const void *b, void *arg)
#else
_kk_list_compare (const void *a, const void *b)
#endif
{
  /**
   * kk_list_t stores void* pointers. Calling qsort,
   * we receive pointers to those pointers in this function. Function parameters
   * are declared void*to not break the qsort compare function prototype. 
   * So we cast them to void**, dereference them and cast them to
   * actual void*pointers and call the user-defined compare function.
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
  if ((list->len >= list->cap) && (_kk_list_enlarge (list) != 0))
    return -1;
  list->items[list->len++] = item;
  return 0;
}

int
kk_list_sort (kk_list_t *list, kk_list_cmp_f cmp)
{
  if (list->len <= 1)
    return 0;

#ifdef HAVE_QSORT_R
  qsort_r (list->items, list->len, sizeof (void *), _kk_list_compare, cmp);
#else
  pthread_mutex_lock (&mutex);
  arg = cmp;
  qsort (list->items, list->len, sizeof (void *), _kk_list_compare);
  arg = NULL;
  pthread_mutex_unlock (&mutex);
#endif

  return 0;
}
