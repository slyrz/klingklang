#ifndef KK_UI_PROGRESSBAR_H
#define KK_UI_PROGRESSBAR_H

#include <klingklang/base.h>
#include <klingklang/ui/widget.h>

typedef struct kk_progressbar kk_progressbar_t;

struct kk_progressbar {
  kk_widget_t widget;
  double progress;
};

int kk_progressbar_init (kk_progressbar_t **progressbar);
int kk_progressbar_free (kk_progressbar_t *progressbar);
int kk_progressbar_set_value (kk_progressbar_t *progressbar, double value);

#endif
