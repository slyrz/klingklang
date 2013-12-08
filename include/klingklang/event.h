#ifndef KK_EVENT_H
#define KK_EVENT_H

#include <klingklang/base.h>

#define kk_event_set(type,event,field,value) \
  do { \
    (((type*)&(event))->field) = (value); \
  } while (0)

#define kk_event_fields \
  unsigned int type;

typedef struct kk_event_queue_s kk_event_queue_t;
typedef struct kk_event_handler_s kk_event_handler_t;
typedef struct kk_event_loop_s kk_event_loop_t;
typedef struct kk_event_s kk_event_t;

typedef void (*kk_event_func_f) (kk_event_loop_t *, int, void *);

struct kk_event_s {
  kk_event_fields;
  unsigned int padding[15];
};

struct kk_event_queue_s {
  int fd[2];
};

struct kk_event_handler_s {
  int fd;
  void *arg;
  kk_event_func_f func;
};

struct kk_event_loop_s {
  size_t cap;
  size_t len;
  int mfd;
  unsigned running:1;
  unsigned exit:1;
  kk_event_handler_t handler[];
};

int kk_event_queue_init (kk_event_queue_t ** queue);
int kk_event_queue_free (kk_event_queue_t * queue);
int kk_event_queue_write (kk_event_queue_t * queue, void *ptr, size_t n);
int kk_event_queue_get_read_fd (kk_event_queue_t * queue);
int kk_event_queue_get_write_fd (kk_event_queue_t * queue);

int kk_event_loop_init (kk_event_loop_t ** loop, size_t cap);
int kk_event_loop_free (kk_event_loop_t * loop);
int kk_event_loop_exit (kk_event_loop_t * loop);
int kk_event_loop_add (kk_event_loop_t * loop, int fd, kk_event_func_f func, void *arg);
int kk_event_loop_run (kk_event_loop_t * loop);

#endif
