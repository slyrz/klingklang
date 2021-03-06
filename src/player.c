#include <klingklang/player.h>
#include <klingklang/util.h>

static void
player_worker_cleanup (kk_player_t *player)
{
  pthread_mutex_unlock (&player->mutex);
}

static void *
player_worker (kk_player_t *player)
{
  const int max_retries = 3;

  pthread_cleanup_push ((void (*)(void *)) player_worker_cleanup, player);

  for (;;) {
    int d = 0;

    for (;;) {
      int s = 0;
      int e = 0;

      kk_frame_t frame;

      /**
       * We lock our mutex and check if the input field is NULL. If it is,
       * we call pthread_cond_wait (with mutex still locked). Every call of
       * pthread_cond_wait releases the mutex and locks our thread on the
       * condition variable. Thus other threads are able to aquire the mutex.
       * These other threads hopefully assign something to input and wake us
       * with a pthread_cond_signal call. This call causes pthread_cond_wait
       * to return with mutex locked. Then we try to read a frame from input
       * and write it to the output device. Releasing the mutex before writing
       * our frame to the input device allows other threads to change input
       * in the meantime.
       */
      pthread_mutex_lock (&player->mutex);
      while ((player->input == NULL) || (player->pause)) {
        pthread_cond_wait (&player->cond, &player->mutex);
      }

      /**
       * Done decoding a frame. This should work on the first try,
       * but sometimes it doesn't. In this case the loop handles these
       * errors pretty well and the user won't notice them at all. Unless
       * something is very wrong and we give up.
       */
      pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

      for (e = 0; e < max_retries; e++) {
        if ((s = kk_input_get_frame (player->input, &frame)) >= 0)
          break;
        kk_log (KK_LOG_WARNING,
            "Error while reading and decoding frame (%d). " \
            "Trying to recover.", s);
      }
      pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
      pthread_mutex_unlock (&player->mutex);

      /* No data read? Stop */
      if (s == 0)
        break;

      /**
       * Now if this happened we really failed reading and decoding another
       * frame.
       */
      if (e == max_retries) {
        kk_log (KK_LOG_WARNING,
            "Reading frame failed %d times. I'm giving up now.", max_retries);
        break;
      }

      /* Don't send this event too often */
      if ((++d & 0x7f) == 0)
        kk_player_event_progress (player->events, frame.prog);

      kk_device_write (player->device, &frame);
    }

    kk_player_next (player);
  }

  /* Basically unreachable, yes */
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

  if (pthread_create (&result->thread, NULL,
        (void *(*)(void *)) player_worker, result) != 0)
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
player_start (kk_player_t *player)
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

    /* Contains truncated stuff - useless */
    free (path);

    /* Try one last time */
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
    kk_log (KK_LOG_WARNING, "Could not determine format of file '%s'...",
        path);
    goto error;
  }

  kk_log (KK_LOG_DEBUG, "Detected audio format of '%s':", item.file->name);
  kk_log (KK_LOG_ATTACH, "Byte Order: %s",
      kk_format_get_byte_order_str (&format));
  kk_log (KK_LOG_ATTACH, "Channels: %d",
      kk_format_get_channels (&format));
  kk_log (KK_LOG_ATTACH, "Datatype: %d bits %s",
      kk_format_get_bits (&format), kk_format_get_type_str (&format));
  kk_log (KK_LOG_ATTACH, "Layout: %s",
      kk_format_get_layout_str (&format));

  if (kk_device_setup (player->device, &format)) {
    kk_log (KK_LOG_WARNING, "Setting up device failed.");
    goto error;
  }

  kk_player_event_start (player->events, item.file);
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

  /* Already playing? Not an error. */
  if (player->input != NULL)
    return 0;

  /* Queue empty? We consider that an error. */
  if (kk_player_queue_is_empty (player->queue))
    return -1;

  pthread_mutex_lock (&player->mutex);
  while (kk_player_queue_is_filled (player->queue)) {
    ret = player_start (player);
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

  kk_player_event_pause (player->events);
  pthread_mutex_lock (&player->mutex);
  /* Toggle lowest bit */
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

  kk_player_event_stop (player->events);
  pthread_mutex_lock (&player->mutex);
  kk_device_drop (player->device);
  kk_input_free (player->input);
  player->input = NULL;
  pthread_mutex_unlock (&player->mutex);
  return 0;
}

int
kk_player_seek (kk_player_t *player, float perc)
{
  if (player->input == NULL)
    return 0;

  pthread_mutex_lock (&player->mutex);
  kk_input_seek (player->input, perc);
  kk_player_event_seek (player->events, perc);
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
