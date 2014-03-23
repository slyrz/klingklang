#ifndef KK_PLAYER_QUEUE_H
#define KK_PLAYER_QUEUE_H

#include <klingklang/base.h>
#include <klingklang/library.h>

#include <pthread.h>

typedef struct kk_player_queue_s kk_player_queue_t;
typedef struct kk_player_item_s kk_player_item_t;

struct kk_player_queue_s {
  kk_player_item_t *fst;
  kk_player_item_t *lst;
  kk_player_item_t *cur;
  pthread_mutex_t mutex;
};

struct kk_player_item_s {
  kk_player_item_t *prev;
  kk_player_item_t *next;
  kk_library_file_t *file;
};

int kk_player_queue_init (kk_player_queue_t **queue);
int kk_player_queue_free (kk_player_queue_t *queue);
int kk_player_queue_clear (kk_player_queue_t *queue);
int kk_player_queue_add (kk_player_queue_t *queue, kk_list_t *sel);
int kk_player_queue_pop (kk_player_queue_t *queue, kk_player_item_t *dst);
int kk_player_queue_is_empty (kk_player_queue_t *queue);
int kk_player_queue_is_filled (kk_player_queue_t *queue);

#endif
