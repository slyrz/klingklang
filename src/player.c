#include <klingklang/player.h>
#include <klingklang/util.h>

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

    /*Remember this in case we have to set fst/cur pointers */
    if (i == 0)
      start = next;

    next->file = (kk_library_file_t *) sel->items[i];
    next->prev = lst;
    if (lst)
      lst->next = next;

    lst = queue->lst = next;
  }

  /**
   *Special cases:
   *1) Queue is empty: We have to set fst and cur pointer, too.
   *2) Queue is filled but cur pointer was NULL: we set cur pointer to the
   * first item of the newly added selection.
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

/**
 *The way the following functions initialize the event structs is the only way
 *gcc/clang don't complain about type punning and valgrind doesn't report some
 *mysterious uninitialized bytes.
 */
static void
kk_player_event_start (kk_player_t *player, kk_library_file_t *file)
{
  kk_player_event_start_t event;

  memset (&event, 0, sizeof (kk_player_event_start_t));
  event.type = KK_PLAYER_START;
  event.file = file;
  kk_event_queue_write (player->events, (void *) &event, sizeof (kk_player_event_start_t));
}

static void
kk_player_event_progress (kk_player_t *player, float progress)
{
  kk_player_event_progress_t event;

  memset (&event, 0, sizeof (kk_player_event_progress_t));
  event.type = KK_PLAYER_PROGRESS;
  event.progress = progress;
  kk_event_queue_write (player->events, (void *) &event, sizeof (kk_player_event_progress_t));
}

static void
kk_player_event_stop (kk_player_t *player)
{
  kk_player_event_stop_t event;

  memset (&event, 0, sizeof (kk_player_event_stop_t));
  event.type = KK_PLAYER_STOP;
  kk_event_queue_write (player->events, (void *) &event, sizeof (kk_player_event_stop_t));
}

static void
kk_player_event_pause (kk_player_t *player)
{
  kk_player_event_pause_t event;

  memset (&event, 0, sizeof (kk_player_event_pause_t));
  event.type = KK_PLAYER_PAUSE;
  kk_event_queue_write (player->events, (void *) &event, sizeof (kk_player_event_pause_t));
}

static void
kk_player_worker_cleanup (kk_player_t *player)
{
  pthread_mutex_unlock (&player->mutex);
}

static void *
kk_player_worker (kk_player_t *player)
{
  const int max_retries = 3;

  pthread_cleanup_push ((void (*)(void *)) kk_player_worker_cleanup, player);

  for (;;) {
    int d = 0;

    for (;;) {
      int s = 0;
      int e = 0;

      kk_frame_t frame;

      /**
       *We lock our mutex and check if the input field is NULL. If it is, 
       *we call pthread_cond_wait (with mutex still locked). Every call of 
       *pthread_cond_wait releases the mutex and locks our thread on the 
       *condition variable. Thus other threads are able to aquire the mutex. 
       *These other threads hopefully assign something to input and wake us 
       *with a pthread_cond_signal call. This call causes pthread_cond_wait
       *to return with mutex locked. Then we try to read a frame from input
       *and write it to the output device. Releasing the mutex before writing 
       *our frame to the input device allows other threads to change input
       *in the meantime.
       */
      pthread_mutex_lock (&player->mutex);
      while ((player->input == NULL) || (player->pause)) {
        pthread_cond_wait (&player->cond, &player->mutex);
      }

      /**
       *Done decoding a frame. This should work on the first try,
       *but sometimes it doesn't. In this case the loop handles these
       *errors pretty well and the user won't notice them at all. Unless
       *something is very wrong and we give up.
       */
      pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

      for (e = 0; e < max_retries; e++) {
        if ((s = kk_input_get_frame (player->input, &frame)) >= 0)
          break;
        kk_log (KK_LOG_WARNING, "Error while reading and decoding frame (%d). Trying to recover.", s);
      }
      pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
      pthread_mutex_unlock (&player->mutex);

      /*No data read? Stop */
      if (s == 0)
        break;

      /**
       *Now if this happened we really failed reading and decoding another 
       *frame.
       */
      if (e == max_retries) {
        kk_log (KK_LOG_WARNING, "Reading frame failed %d times. I'm giving up now.", max_retries);
        break;
      }

      /*Don't send this event too often */
      if ((++d & 0x7f) == 0)
        kk_player_event_progress (player, frame.prog);

      kk_device_write (player->device, &frame);
    }

    kk_player_next (player);
  }

  /*Basically unreachable, yes */
  pthread_cleanup_pop (0);
  return NULL;
}

int
kk_player_init (kk_player_t **player)
{
  kk_player_t *result;

  result = calloc (1, sizeof (kk_player_t));
  if (result == NULL)
    goto error;

  if (kk_device_init (&result->device) != 0)
    goto error;

  if (kk_player_queue_init (&result->queue) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (pthread_cond_init (&result->cond, NULL) != 0)
    goto error;

  if (pthread_mutex_init (&result->mutex, NULL) != 0)
    goto error;

  if (pthread_create (&result->thread, 0, (void *(*)(void *)) kk_player_worker, result) != 0)
    goto error;

  *player = result;
  return 0;
error:
  kk_player_free (result);
  *player = NULL;
  return -1;
}

int
kk_player_free (kk_player_t *player)
{
  if (player == NULL)
    return 0;

  if ((player->thread) && (pthread_cancel (player->thread) == 0))
    pthread_join (player->thread, NULL);

  pthread_cond_destroy (&player->cond);
  pthread_mutex_destroy (&player->mutex);

  if (player->queue)
    kk_player_queue_free (player->queue);

  if (player->events)
    kk_event_queue_free (player->events);

  if (player->device)
    kk_device_free (player->device);

  free (player);
  return 0;
}

static int
_kk_player_start (kk_player_t *player)
{
  static kk_format_t format;

  char *path = NULL;
  size_t len;
  size_t out;

  kk_player_item_t item;

  memset (&item, 0, sizeof (kk_player_item_t));
  if (kk_player_queue_pop (player->queue, &item) != 0)
    goto error;

  len = 512;
  path = calloc (len, sizeof (char));
  if (path == NULL)
    goto error;

  out = kk_library_file_get_path (item.file, path, len);
  if (out >= len) {
    if (out >= 8192)
      goto error;

    /*Contains truncated stuff - useless */
    free (path);

    /*Try one last time */
    len = out + 1;
    path = calloc (len, sizeof (char));
    if (path == NULL)
      goto error;

    out = kk_library_file_get_path (item.file, path, len);
    if (out >= len)
      goto error;
  }

  if (kk_input_init (&player->input, path) < 0) {
    kk_log (KK_LOG_WARNING, "Could not open file '%s'...", path);
    goto error;
  }

  memset (&format, 0, sizeof (kk_format_t));
  if (kk_input_get_format (player->input, &format)) {
    kk_log (KK_LOG_WARNING, "Could not determine format of file '%s'...", path);
    goto error;
  }

  kk_log (KK_LOG_DEBUG, "Detected audio format of '%s':", item.file->name);
  kk_log (KK_LOG_ATTACH, "Byte Order: %s", kk_format_get_byte_order_str (&format));
  kk_log (KK_LOG_ATTACH, "Channels: %d", kk_format_get_channels (&format));
  kk_log (KK_LOG_ATTACH, "Datatype: %d bits %s", kk_format_get_bits (&format), kk_format_get_type_str (&format));
  kk_log (KK_LOG_ATTACH, "Layout: %s", kk_format_get_layout_str (&format));

  if (kk_device_setup (player->device, &format)) {
    kk_log (KK_LOG_WARNING, "Setting up device failed.");
    goto error;
  }

  kk_player_event_start (player, item.file);
  player->pause = 0;
  free (path);
  return 0;
error:
  if (player->input)
    kk_input_free (player->input);
  player->input = NULL;
  free (path);
  return -1;
}

int
kk_player_start (kk_player_t *player)
{
  int ret = -1;

  if ((player->input != NULL) | kk_player_queue_is_empty (player->queue))
    return -1;

  pthread_mutex_lock (&player->mutex);
  while (kk_player_queue_is_filled (player->queue)) {
    ret = _kk_player_start (player);
    if (ret == 0)
      break;
  }
  if (ret == 0)
    pthread_cond_signal (&player->cond);
  pthread_mutex_unlock (&player->mutex);
  return ret;
}

int
kk_player_pause (kk_player_t *player)
{
  int was_paused = player->pause;

  kk_player_event_pause (player);
  pthread_mutex_lock (&player->mutex);
  /*Toggle lowest bit */
  player->pause = (player->pause ^ 1) & 1;
  if (was_paused)
    pthread_cond_signal (&player->cond);
  pthread_mutex_unlock (&player->mutex);
  return 0;
}

int
kk_player_stop (kk_player_t *player)
{
  if (player->input == NULL)
    return 0;

  kk_player_event_stop (player);
  pthread_mutex_lock (&player->mutex);
  kk_device_drop (player->device);
  kk_input_free (player->input);
  player->input = NULL;
  pthread_mutex_unlock (&player->mutex);
  return 0;
}

int
kk_player_next (kk_player_t *player)
{
  if ((player->input) && (kk_player_stop (player) != 0))
    return -1;
  return kk_player_start (player);
}

int
kk_player_get_event_fd (kk_player_t *player)
{
  return kk_event_queue_get_read_fd (player->events);
}
