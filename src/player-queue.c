#include <klingklang/player-queue.h>

static void
_kk_player_queue_free_items (kk_player_queue_t *queue)
{
  kk_player_item_t *item;
  kk_player_item_t *next;

  item = queue->fst;
  while (item) {
    next = item->next;
    free (item);
    item = next;
  }
}

int
kk_player_queue_init (kk_player_queue_t **queue)
{
  kk_player_queue_t *result;

  result = calloc (1, sizeof (kk_player_queue_t));
  if (result == NULL)
    goto error;

  if (pthread_mutex_init (&result->mutex, NULL) != 0)
    goto error;

  *queue = result;
  return 0;
error:
  kk_player_queue_free (result);
  *queue = NULL;
  return -1;
}

int
kk_player_queue_free (kk_player_queue_t *queue)
{
  if (queue == NULL)
    return 0;

  if (queue->fst)
    _kk_player_queue_free_items (queue);

  pthread_mutex_destroy (&queue->mutex);
  free (queue);
  return 0;
}

int
kk_player_queue_is_empty (kk_player_queue_t *queue)
{
  int result;

  pthread_mutex_lock (&queue->mutex);
  result = (queue->cur == NULL);
  pthread_mutex_unlock (&queue->mutex);
  return result;
}

int
kk_player_queue_is_filled (kk_player_queue_t *queue)
{
  return !kk_player_queue_is_empty (queue);
}

int
kk_player_queue_clear (kk_player_queue_t *queue)
{
  pthread_mutex_lock (&queue->mutex);
  _kk_player_queue_free_items (queue);
  queue->fst = NULL;
  queue->lst = NULL;
  queue->cur = NULL;
  pthread_mutex_unlock (&queue->mutex);
  return 0;
}

int
kk_player_queue_add (kk_player_queue_t *queue, kk_list_t *sel)
{
  kk_player_item_t *start = NULL;
  kk_player_item_t *next;
  kk_player_item_t *lst;

  size_t i;

  if (sel->len == 0)
    return -1;

  pthread_mutex_lock (&queue->mutex);

  lst = queue->lst;
  for (i = 0; i < sel->len; i++) {
    next = calloc (1, sizeof (kk_player_item_t));
    if (next == NULL)
      goto error;

    /* Remember this in case we have to set fst/cur pointers */
    if (i == 0)
      start = next;

    next->file = (kk_library_file_t *) sel->items[i];
    next->prev = lst;
    if (lst)
      lst->next = next;

    lst = queue->lst = next;
  }

  /**
   * Special cases:
   * 1) Queue is empty: We have to set fst and cur pointer, too.
   * 2) Queue is filled but cur pointer was NULL: we set cur pointer to the
   *  first item of the newly added selection.
   */
  if (queue->fst == NULL)
    queue->cur = queue->fst = start;

  if (queue->cur == NULL)
    queue->cur = start;

  pthread_mutex_unlock (&queue->mutex);
  return 0;
error:
  pthread_mutex_unlock (&queue->mutex);
  return -1;
}

int
kk_player_queue_pop (kk_player_queue_t *queue, kk_player_item_t *dst)
{
  pthread_mutex_lock (&queue->mutex);
  if (queue->cur == NULL)
    goto error;
  memcpy (dst, queue->cur, sizeof (kk_player_item_t));
  queue->cur = queue->cur->next;
  pthread_mutex_unlock (&queue->mutex);
  return 0;
error:
  pthread_mutex_unlock (&queue->mutex);
  return -1;
}
