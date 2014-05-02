#ifndef KK_UI_KEYS_H
#define KK_UI_KEYS_H

#include <klingklang/base.h>

#ifdef HAVE_XKBCOMMON
#  include "keys/xkbcommon.h"
#endif

#ifdef HAVE_XCB_KEYSYMS
#  include "keys/xcb.h"
#endif

typedef struct kk_keys kk_keys_t;

int kk_keys_init (kk_keys_t **keys);
int kk_keys_free (kk_keys_t *keys);

int kk_keys_get_symbol (kk_keys_t *keys, uint32_t code);
int kk_keys_get_modifiers (kk_keys_t *keys);

int kk_keys_set_modifiers (kk_keys_t *keys, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

#endif
