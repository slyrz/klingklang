#ifndef KK_DEVICE_H
#define KK_DEVICE_H

#include <klingklang/base.h>
#include <klingklang/format.h>
#include <klingklang/frame.h>

#include <pthread.h>

typedef struct kk_device_s kk_device_t;
typedef struct kk_device_backend_s kk_device_backend_t;

struct kk_device_s {
  kk_format_t *format;
  pthread_mutex_t mutex;
};

struct kk_device_backend_s {
  size_t size;
  int (*init) (kk_device_t *dev);
  int (*free) (kk_device_t *dev);
  int (*drop) (kk_device_t *dev);
  int (*setup) (kk_device_t *dev, kk_format_t *format);
  int (*write) (kk_device_t *dev, kk_frame_t *frame);
};

int kk_device_init (kk_device_t **dev);
int kk_device_free (kk_device_t *dev);
int kk_device_drop (kk_device_t *dev);
int kk_device_setup (kk_device_t *dev, kk_format_t *format);
int kk_device_write (kk_device_t *dev, kk_frame_t *frame);

#endif
