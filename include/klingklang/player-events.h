#ifndef KK_PLAYER_EVENTS_H
#define KK_PLAYER_EVENTS_H

#include <klingklang/event.h>

enum {
  KK_PLAYER_PAUSE,
  KK_PLAYER_PROGRESS,
  KK_PLAYER_START,
  KK_PLAYER_STOP,
};

typedef struct kk_player_event_pause_s kk_player_event_pause_t;
typedef struct kk_player_event_progress_s kk_player_event_progress_t;
typedef struct kk_player_event_start_s kk_player_event_start_t;
typedef struct kk_player_event_stop_s kk_player_event_stop_t;

struct kk_player_event_pause_s {
  kk_event_fields;
};

struct kk_player_event_progress_s {
  kk_event_fields;
  float progress;
};

struct kk_player_event_start_s {
  kk_event_fields;
  kk_library_file_t *file;
};

struct kk_player_event_stop_s {
  kk_event_fields;
};

#endif
