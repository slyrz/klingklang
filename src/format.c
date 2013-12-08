#include <klingklang/format.h>
#include <klingklang/util.h>

/**
 * I know, these switch statements are ugly, but directly returning values
 * with every case lead to dead code warnings at the end of the switch
 * statements. This is the only way gcc / clang don't warn about anything.
 */

int
kk_format_get_channels (kk_format_t * fmt)
{
  int result = -1;

  switch (fmt->channels) {
    case KK_CHANNELS_1:
      result = 1;
      break;
    case KK_CHANNELS_2:
      result = 2;
      break;
  }
  return result;
}

int
kk_format_get_bits (kk_format_t * fmt)
{
  int result = -1;

  switch (fmt->bits) {
    case KK_BITS_8:
      result = 8;
      break;
    case KK_BITS_16:
      result = 16;
      break;
    case KK_BITS_24:
      result = 24;
      break;
    case KK_BITS_32:
      result = 32;
      break;
    case KK_BITS_64:
      result = 64;
      break;
  }
  return result;
}

const char *
kk_format_get_type_str (kk_format_t * fmt)
{
  const char *result = NULL;

  switch (fmt->type) {
    case KK_TYPE_UINT:
      result = "uint";
      break;
    case KK_TYPE_SINT:
      result = "sint";
      break;
    case KK_TYPE_FLOAT:
      result = "float";
      break;
  }
  return result;
}

const char *
kk_format_get_layout_str (kk_format_t * fmt)
{
  const char *result = NULL;

  switch (fmt->layout) {
    case KK_LAYOUT_PLANAR:
      result = "planar";
      break;
    case KK_LAYOUT_INTERLEAVED:
      result = "interleaved";
      break;
  }
  return result;
}

const char *
kk_format_get_byte_order_str (kk_format_t * fmt)
{
  const char *result = NULL;

  switch (fmt->byte_order) {
    case KK_BYTE_ORDER_LITTLE_ENDIAN:
      result = "little endian";
      break;
    case KK_BYTE_ORDER_BIG_ENDIAN:
      result = "big endian";
      break;
  }
  return result;
}
