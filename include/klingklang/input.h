#ifndef KK_INPUT_H
#define KK_INPUT_H

#include <klingklang/format.h>
#include <klingklang/frame.h>

typedef struct kk_input_s kk_input_t;

int kk_input_init (kk_input_t **inp, const char *filename);
int kk_input_free (kk_input_t *inp);

int kk_input_get_frame (kk_input_t *inp, kk_frame_t *frame);
int kk_input_get_format (kk_input_t *inp, kk_format_t *format);

#endif
