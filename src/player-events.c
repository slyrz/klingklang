#include <klingklang/player-events.h>

/**
 * The way the following functions initialize the event structs is the only way
 * gcc/clang don't complain about type punning and valgrind doesn't report some
 * mysterious uninitialized bytes.
 */
void
kk_player_event_seek (kk_event_queue_t *queue, float perc)
{
  kk_player_event_seek_t event;

  memset (&event, 0, sizeof (kk_player_event_seek_t));
  event.type = KK_PLAYER_SEEK;
  event.perc = perc;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_player_event_seek_t));
}

void
kk_player_event_start (kk_event_queue_t *queue, kk_library_file_t *file)
{
  kk_player_event_start_t event;

  memset (&event, 0, sizeof (kk_player_event_start_t));
  event.type = KK_PLAYER_START;
  event.file = file;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_player_event_start_t));
}

void
kk_player_event_progress (kk_event_queue_t *queue, float progress)
{
  kk_player_event_progress_t event;

  memset (&event, 0, sizeof (kk_player_event_progress_t));
  event.type = KK_PLAYER_PROGRESS;
  event.progress = progress;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_player_event_progress_t));
}

void
kk_player_event_stop (kk_event_queue_t *queue)
{
  kk_player_event_stop_t event;

  memset (&event, 0, sizeof (kk_player_event_stop_t));
  event.type = KK_PLAYER_STOP;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_player_event_stop_t));
}

void
kk_player_event_pause (kk_event_queue_t *queue)
{
  kk_player_event_pause_t event;

  memset (&event, 0, sizeof (kk_player_event_pause_t));
  event.type = KK_PLAYER_PAUSE;
  kk_event_queue_write (queue, (void *) &event, sizeof (kk_player_event_pause_t));
}
