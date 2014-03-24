#ifndef KK_FORMAT_H
#define KK_FORMAT_H

#include <klingklang/base.h>

#ifdef WORDS_BIGENDIAN
#  define KK_BYTE_ORDER_NATIVE KK_BYTE_ORDER_BIG_ENDIAN
#else
#  define KK_BYTE_ORDER_NATIVE KK_BYTE_ORDER_LITTLE_ENDIAN
#endif

typedef struct kk_format kk_format_t;
typedef enum kk_bits kk_bits_t;
typedef enum kk_byte_order kk_byte_order_t;
typedef enum kk_channels kk_channels_t;
typedef enum kk_layout kk_layout_t;
typedef enum kk_type kk_type_t;

/**
 * Important Note: Adding, removing or rearranging enum members will most
 * likely fuck shit up. What I'm trying to say is: don't touch this.
 */
enum kk_bits {
  KK_BITS_8,
  KK_BITS_16,
  KK_BITS_24,
  KK_BITS_32,
  KK_BITS_64,
};

enum kk_byte_order {
  KK_BYTE_ORDER_LITTLE_ENDIAN,
  KK_BYTE_ORDER_BIG_ENDIAN,
};

enum kk_channels {
  KK_CHANNELS_1,
  KK_CHANNELS_2,
};

enum kk_layout {
  KK_LAYOUT_PLANAR,
  KK_LAYOUT_INTERLEAVED,
};

enum kk_type {
  KK_TYPE_UINT,
  KK_TYPE_SINT,
  KK_TYPE_FLOAT,
};

struct kk_format {
  kk_bits_t bits;
  kk_byte_order_t byte_order;
  kk_channels_t channels;
  kk_layout_t layout;
  kk_type_t type;
  unsigned int sample_rate;
};

int kk_format_get_channels (kk_format_t *fmt);
int kk_format_get_bits (kk_format_t *fmt);

const char *kk_format_get_type_str (kk_format_t *fmt);
const char *kk_format_get_layout_str (kk_format_t *fmt);
const char *kk_format_get_byte_order_str (kk_format_t *fmt);

void kk_format_print (kk_format_t *fmt);

#endif
