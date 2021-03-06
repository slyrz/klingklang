#ifndef KK_FRAME_H
#define KK_FRAME_H

#include <klingklang/base.h>
#include <klingklang/format.h>

#define KK_FRAME_MAX_PLANES		2

typedef struct kk_frame kk_frame_t;

struct kk_frame {
  float prog;
  size_t size;
  size_t samples;
  size_t planes;
  uint8_t *data[KK_FRAME_MAX_PLANES];
};

int kk_frame_init (kk_frame_t **frame);
int kk_frame_free (kk_frame_t *frame);

int kk_frame_interleave (kk_frame_t *restrict dst, kk_frame_t *restrict src, kk_format_t *fmt);

#endif
