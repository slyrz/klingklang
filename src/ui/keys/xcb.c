#include <klingklang/ui/keys.h>

/**
 * The xcb_keysyms implementation should only be used on systems where no
 * libxkbcommon is available.
 */
struct kk_keys {
  xcb_connection_t *connection;
  xcb_key_symbols_t *symbols;
};

int
kk_keys_init (kk_keys_t **keys)
{
  kk_keys_t *result;

  result = calloc (1, sizeof (kk_keys_t));
  if (result == NULL)
    goto error;

  result->connection = xcb_connect (NULL, NULL);
  if (result->connection == NULL)
    goto error;

  result->symbols = xcb_key_symbols_alloc (result->connection);
  if (result->symbols == NULL)
    goto error;

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

  if (keys->symbols)
    xcb_key_symbols_free (keys->symbols);

  if (keys->connection != NULL)
    xcb_disconnect (keys->connection);

  free (keys);
  return 0;
}

int
kk_keys_get_symbol (kk_keys_t *keys, uint32_t code)
{
  return (int) xcb_key_symbols_get_keysym (keys->symbols, (xcb_keycode_t) code, 0);
}

int
kk_keys_get_modifiers (kk_keys_t *keys)
{
  (void) keys;
  return 0;
}

int
kk_keys_set_modifiers (kk_keys_t *keys, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  (void) keys;
  (void) mods_depressed;
  (void) mods_latched;
  (void) mods_locked;
  (void) group;
  return 0;
}
