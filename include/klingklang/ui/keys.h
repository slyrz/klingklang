#ifndef KK_UI_KEYS_H
#define KK_UI_KEYS_H

#include <xkbcommon/xkbcommon.h>

#define KK_KEYS_RULES   "evdev"
#define KK_KEYS_MODEL   "evdev"
#define KK_KEYS_LAYOUT  "us"
#define KK_KEYS_VARIANT ""
#define KK_KEYS_OPTIONS ""

enum {
  KK_KEY_UP,
  KK_KEY_DOWN,
};

enum {
  KK_MOD_CONTROL  = 1 << 1,
  KK_MOD_SHIFT    = 1 << 2,
};

enum {
  KK_KEY_NONE = 0,
  KK_KEY_SPACE = XKB_KEY_space,
  KK_KEY_BACKSPACE = XKB_KEY_BackSpace,
  KK_KEY_TAB = XKB_KEY_Tab,
  KK_KEY_CLEAR = XKB_KEY_Clear,
  KK_KEY_RETURN = XKB_KEY_Return,
  KK_KEY_PAUSE = XKB_KEY_Pause,
  KK_KEY_ESCAPE = XKB_KEY_Escape,
  KK_KEY_DELETE = XKB_KEY_Delete,
  KK_KEY_0 = XKB_KEY_0,
  KK_KEY_1 = XKB_KEY_1,
  KK_KEY_2 = XKB_KEY_2,
  KK_KEY_3 = XKB_KEY_3,
  KK_KEY_4 = XKB_KEY_4,
  KK_KEY_5 = XKB_KEY_5,
  KK_KEY_6 = XKB_KEY_6,
  KK_KEY_7 = XKB_KEY_7,
  KK_KEY_8 = XKB_KEY_8,
  KK_KEY_9 = XKB_KEY_9,
  KK_KEY_A = XKB_KEY_a,
  KK_KEY_B = XKB_KEY_b,
  KK_KEY_C = XKB_KEY_c,
  KK_KEY_D = XKB_KEY_d,
  KK_KEY_E = XKB_KEY_e,
  KK_KEY_F = XKB_KEY_f,
  KK_KEY_G = XKB_KEY_g,
  KK_KEY_H = XKB_KEY_h,
  KK_KEY_I = XKB_KEY_i,
  KK_KEY_J = XKB_KEY_j,
  KK_KEY_K = XKB_KEY_k,
  KK_KEY_L = XKB_KEY_l,
  KK_KEY_M = XKB_KEY_m,
  KK_KEY_N = XKB_KEY_n,
  KK_KEY_O = XKB_KEY_o,
  KK_KEY_P = XKB_KEY_p,
  KK_KEY_Q = XKB_KEY_q,
  KK_KEY_R = XKB_KEY_r,
  KK_KEY_S = XKB_KEY_s,
  KK_KEY_T = XKB_KEY_t,
  KK_KEY_U = XKB_KEY_u,
  KK_KEY_V = XKB_KEY_v,
  KK_KEY_W = XKB_KEY_w,
  KK_KEY_X = XKB_KEY_x,
  KK_KEY_Y = XKB_KEY_y,
  KK_KEY_Z = XKB_KEY_z
};

typedef struct kk_keys_s kk_keys_t;

struct kk_keys_s {
  struct xkb_context *context;
  struct xkb_keymap *keymap;
  struct xkb_state *state;
  struct {
    xkb_mod_mask_t control;
    xkb_mod_mask_t shift;
  } mask;
};

int kk_keys_init (kk_keys_t **keys);
int kk_keys_free (kk_keys_t *keys);

int kk_keys_get_symbol (kk_keys_t *keys, uint32_t code);
int kk_keys_get_modifiers (kk_keys_t *keys);

int kk_keys_set_modifiers (kk_keys_t *keys, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

#endif
