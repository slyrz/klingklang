#ifndef KK_PLAYER_H
#define KK_PLAYER_H

#include <klingklang/base.h>
#include <klingklang/input.h>
#include <klingklang/device.h>
#include <klingklang/library.h>
#include <klingklang/player-events.h>
#include <klingklang/player-queue.h>

#include <pthread.h>

typedef struct kk_player kk_player_t;

struct kk_player {
  kk_player_queue_t *queue;
  kk_event_queue_t *events;
  kk_input_t *input;
  kk_device_t *device;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  pthread_t thread;
  unsigned pause:1;
  unsigned shuffle:1;
};

int kk_player_init (kk_player_t **player);
int kk_player_free (kk_player_t *player);
int kk_player_start (kk_player_t *player);
int kk_player_pause (kk_player_t *player);
int kk_player_stop (kk_player_t *player);
int kk_player_seek (kk_player_t *player, float perc);
int kk_player_next (kk_player_t *player);

int kk_player_get_event_fd (kk_player_t *player);

#endif
