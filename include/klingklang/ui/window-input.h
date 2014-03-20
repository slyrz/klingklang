#ifndef KK_UI_WINDOW_INPUT_H
#define KK_UI_WINDOW_INPUT_H

#include <klingklang/event.h>

typedef struct kk_window_input_state_s kk_window_input_state_t;

struct kk_window_input_state_s {
	kk_event_queue_t *queue;
	int fd;
};

int kk_window_input_request (kk_event_queue_t *queue);

#endif
