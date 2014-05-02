#ifndef KK_UI_KEYS_XCB_H
#define KK_UI_KEYS_XCB_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

enum {
  KK_MOD_SHIFT = XCB_MOD_MASK_SHIFT,
  KK_MOD_CONTROL = XCB_MOD_MASK_CONTROL
};

enum {
  KK_KEY_SPACE = XK_space,
  KK_KEY_BACKSPACE = XK_BackSpace,
  KK_KEY_TAB = XK_Tab,
  KK_KEY_CLEAR = XK_Clear,
  KK_KEY_RETURN = XK_Return,
  KK_KEY_PAUSE = XK_Pause,
  KK_KEY_ESCAPE = XK_Escape,
  KK_KEY_DELETE = XK_Delete,
  KK_KEY_0 = XK_0,
  KK_KEY_1 = XK_1,
  KK_KEY_2 = XK_2,
  KK_KEY_3 = XK_3,
  KK_KEY_4 = XK_4,
  KK_KEY_5 = XK_5,
  KK_KEY_6 = XK_6,
  KK_KEY_7 = XK_7,
  KK_KEY_8 = XK_8,
  KK_KEY_9 = XK_9,
  KK_KEY_A = XK_a,
  KK_KEY_B = XK_b,
  KK_KEY_C = XK_c,
  KK_KEY_D = XK_d,
  KK_KEY_E = XK_e,
  KK_KEY_F = XK_f,
  KK_KEY_G = XK_g,
  KK_KEY_H = XK_h,
  KK_KEY_I = XK_i,
  KK_KEY_J = XK_j,
  KK_KEY_K = XK_k,
  KK_KEY_L = XK_l,
  KK_KEY_M = XK_m,
  KK_KEY_N = XK_n,
  KK_KEY_O = XK_o,
  KK_KEY_P = XK_p,
  KK_KEY_Q = XK_q,
  KK_KEY_R = XK_r,
  KK_KEY_S = XK_s,
  KK_KEY_T = XK_t,
  KK_KEY_U = XK_u,
  KK_KEY_V = XK_v,
  KK_KEY_W = XK_w,
  KK_KEY_X = XK_x,
  KK_KEY_Y = XK_y,
  KK_KEY_Z = XK_z
};

#endif