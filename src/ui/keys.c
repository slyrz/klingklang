#include <klingklang/ui/keys.h>
#include <klingklang/base.h>

const struct xkb_rule_names rules = {
  .rules = KK_KEYS_RULES,
  .model = KK_KEYS_MODEL,
  .layout = KK_KEYS_LAYOUT,
  .variant = KK_KEYS_VARIANT,
  .options = KK_KEYS_OPTIONS
};

int
kk_keys_init (kk_keys_t ** keys)
{
  kk_keys_t *result;

  result = calloc (1, sizeof (kk_keys_t));
  if (result == NULL)
    goto error;

  result->context = xkb_context_new (0);
  if (result->context == NULL)
    goto error;

  result->keymap = xkb_keymap_new_from_names (result->context, &rules, 0);
  if (result->keymap == NULL)
    goto error;

  result->state = xkb_state_new (result->keymap);
  if (result->state == NULL)
    goto error;

  *keys = result;
  return 0;
error:
  kk_keys_free (result);
  *keys = NULL;
  return -1;
}

int
kk_keys_free (kk_keys_t * keys)
{
  if (keys == NULL)
    return 0;

  /**
   * All three unref functions are safe to call with NULL pointers, according
   * to the xkbcommon documentation.
   */
  xkb_state_unref (keys->state);
  xkb_keymap_unref (keys->keymap);
  xkb_context_unref (keys->context);

  free (keys);
  return 0;
}

int
kk_keys_get_symbol (kk_keys_t * keys, uint32_t code)
{
  return (int) xkb_state_key_get_one_sym (keys->state, code);
}
