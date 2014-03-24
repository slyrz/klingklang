#ifndef KK_PLAYER_EVENTS_H
#define KK_PLAYER_EVENTS_H

#include <klingklang/event.h>
#include <klingklang/library.h>

enum {
  KK_PLAYER_PAUSE,
  KK_PLAYER_PROGRESS,
  KK_PLAYER_SEEK,
  KK_PLAYER_START,
  KK_PLAYER_STOP,
};

typedef struct kk_player_event_pause kk_player_event_pause_t;
typedef struct kk_player_event_progress kk_player_event_progress_t;
typedef struct kk_player_event_seek kk_player_event_seek_t;
typedef struct kk_player_event_start kk_player_event_start_t;
typedef struct kk_player_event_stop kk_player_event_stop_t;

struct kk_player_event_pause {
  kk_event_fields;
};

struct kk_player_event_progress {
  kk_event_fields;
  float progress;
};

struct kk_player_event_seek {
  kk_event_fields;
  float perc;
};

struct kk_player_event_start {
  kk_event_fields;
  kk_library_file_t *file;
};

struct kk_player_event_stop {
  kk_event_fields;
};

void kk_player_event_seek (kk_event_queue_t *queue, float perc);
void kk_player_event_start (kk_event_queue_t *queue, kk_library_file_t *file);
void kk_player_event_progress (kk_event_queue_t *queue, float progress);
void kk_player_event_stop (kk_event_queue_t *queue);
void kk_player_event_pause (kk_event_queue_t *queue);

#endif
