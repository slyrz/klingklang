#include <klingklang/ui/keys.h>

const struct xkb_rule_names rules = {
  .rules   = "evdev",
  .model   = "evdev",
  .layout  = "us",
  .variant = "",
  .options = ""
};

struct kk_keys {
  struct xkb_context *context;
  struct xkb_keymap *keymap;
  struct xkb_state *state;
  struct {
    xkb_mod_mask_t control;
    xkb_mod_mask_t shift;
  } mask;
};

static xkb_mod_mask_t
keys_get_mod_mask (kk_keys_t *keys, const char *mod)
{
  return (xkb_mod_mask_t) 1 << xkb_keymap_mod_get_index (keys->keymap, mod);
}

int
kk_keys_init (kk_keys_t **keys)
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

  result->mask.control = keys_get_mod_mask (result, "Control");
  result->mask.shift = keys_get_mod_mask (result, "Shift");
  *keys = result;
  return 0;
error:
  kk_keys_free (result);
  *keys = NULL;
  return -1;
}

int
kk_keys_free (kk_keys_t *keys)
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
kk_keys_get_symbol (kk_keys_t *keys, uint32_t code)
{
  return (int) xkb_state_key_get_one_sym (keys->state, code);
}

int
kk_keys_get_modifiers (kk_keys_t *keys)
{
  xkb_mod_mask_t mask;
  int result;

  mask = xkb_state_serialize_mods (keys->state, XKB_STATE_DEPRESSED | XKB_STATE_LATCHED);
  result = 0;

  if (mask & keys->mask.control)
    result |= KK_MOD_CONTROL;

  if (mask & keys->mask.shift)
    result |= KK_MOD_SHIFT;

  return result;
}

int
kk_keys_set_modifiers (kk_keys_t *keys, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  xkb_state_update_mask (keys->state, mods_depressed, mods_latched,
      mods_locked, 0, 0, group);
  return 0;
}
