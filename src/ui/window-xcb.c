#include <klingklang/ui/window.h>
#include <klingklang/str.h>
#include <klingklang/util.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

#include <cairo.h>
#include <cairo-xcb.h>

#ifdef HAVE_XCB_ICCCM_PREFIX
#  define xcb_set_wm_protocols xcb_icccm_set_wm_protocols
#endif

#define KK_WINDOW_INPUT_PROP            "_KK_INPUT"
#define KK_WINDOW_MAX_TITLE_LEN         0x80u
#define KK_WINDOW_MAX_ATOM_LEN          0x80u
#define KK_WINDOW_MAX_PROPERTY_LEN      0xffu

struct kk_window_s {
  kk_window_fields;
  xcb_connection_t *connection;
  xcb_screen_t *screen;
  xcb_window_t window;
  xcb_atom_t input;
  struct {
    cairo_surface_t *surface;
    cairo_t *context;
  } cairo;
};

static xcb_atom_t
window_get_atom (kk_window_t *win, const char *name)
{
  xcb_intern_atom_cookie_t cookie;
  xcb_intern_atom_reply_t *reply;
  xcb_atom_t atom;

  size_t len;

  if (name == NULL)
    return XCB_NONE;

  len = kk_str_len (name, KK_WINDOW_MAX_ATOM_LEN);
  if (len == KK_WINDOW_MAX_ATOM_LEN) {
    kk_log (KK_LOG_WARNING, "Name of atom exceeds maximum atom length.");
    return XCB_NONE;
  }

  cookie = xcb_intern_atom (win->connection, 0, (uint16_t) len, name);
  reply = xcb_intern_atom_reply (win->connection, cookie, NULL);
  if (reply == NULL)
    return XCB_NONE;

  atom = reply->atom;
  free (reply);
  return atom;
}

static int
window_get_property (kk_window_t *win, xcb_atom_t property, char **dst)
{
  xcb_get_property_cookie_t cookie;
  xcb_get_property_reply_t *reply = NULL;

  char buffer[KK_WINDOW_MAX_PROPERTY_LEN];
  int ret;

  if (dst == NULL)
    goto error;

  cookie = xcb_get_property (win->connection, 0, win->window, property, XCB_ATOM_STRING, 0, KK_WINDOW_MAX_PROPERTY_LEN);
  reply = xcb_get_property_reply (win->connection, cookie, NULL);
  if (reply == NULL)
    goto error;

  ret = xcb_get_property_value_length (reply);
  if ((ret < 0) || (KK_WINDOW_MAX_PROPERTY_LEN < (size_t) (ret + 1)))
    goto error;

  /**
   * Copy property value into buffer. We can't use strl{cat,cpy} here
   * because the property value isn't null-terminated.
   */
  {
    register char *out;
    register char *inp;

    out = buffer;
    inp = xcb_get_property_value (reply);
    for (; ret > 0; out++, inp++, ret--)
      *out = *inp;
    *out = '\0';
  }

  *dst = strndup (buffer, KK_WINDOW_MAX_PROPERTY_LEN);
  if (*dst == NULL)
    goto error;
  free (reply);
  return 0;
error:
  if (dst)
    *dst = NULL;
  free (reply);
  return -1;
}

static int
window_get_xid_str (kk_window_t *win, char *dst, size_t n)
{
  int ret;

  if ((dst == NULL) || (win->window == 0))
    return -1;

  ret = snprintf (dst, n, "%d", win->window);
  if ((ret <= 0) | (ret >= (int) n))
    return -1;
  return 0;
}

static xcb_visualtype_t *
window_get_visual_type (kk_window_t *win)
{
  xcb_visualtype_t *visual_type;
  xcb_depth_iterator_t depth_iter;

  depth_iter = xcb_screen_allowed_depths_iterator (win->screen);

  visual_type = NULL;
  for (; depth_iter.rem; xcb_depth_next (&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next (&visual_iter)) {
      if (win->screen->root_visual == visual_iter.data->visual_id) {
        visual_type = visual_iter.data;
        break;
      }
    }
  }
  return visual_type;
}

static void
window_handle_configure_notify_event (kk_window_t *win, xcb_configure_notify_event_t *event)
{
  kk_window_event_resize (win->events, event->width, event->height);
}

static void
window_handle_expose_event (kk_window_t *win, xcb_expose_event_t *event)
{
  (void) event;

  kk_widget_invalidate ((kk_widget_t*) win);
  kk_window_event_expose (win->events, win->width, win->height);
}

static void
window_handle_key_press_event (kk_window_t *win, xcb_key_press_event_t *event)
{
  int key = 0;
  int mod = 0;

  key = kk_keys_get_symbol (win->keys, event->detail);

  if (event->state & XCB_MOD_MASK_SHIFT)
    mod |= KK_MOD_SHIFT;

  if (event->state & XCB_MOD_MASK_CONTROL)
    mod |= KK_MOD_CONTROL;

  kk_window_event_key_press (win->events, mod, key);
}

static void
window_handle_property_notify_event (kk_window_t *win, xcb_property_notify_event_t *event)
{
  static char *value;

  if (event->atom != win->input)
    return;

  if (window_get_property (win, event->atom, &value) != 0) {
    kk_log (KK_LOG_ERROR, "Failed to retrieve input property value.");
    return;
  }

  if (*value == '\0') {
    kk_log (KK_LOG_WARNING, "Empty input value.");
    return;
  }

  kk_window_event_input (win->events, value);
}

static void
window_handle_mapping_notify_event (kk_window_t *win, xcb_mapping_notify_event_t *event)
{
  /**
   * TODO: find a way to refresh keyboard mapping or completely switch back to
   * xcb_keysyms?
   * xcb_refresh_keyboard_mapping (win->syms, event);
   */
  (void) win;
  (void) event;
}

static void *
window_event_handler (kk_window_t *win)
{
  xcb_generic_event_t *event;

  win->alive = 1;
  while ((event = xcb_wait_for_event (win->connection))) {
    switch (XCB_EVENT_RESPONSE_TYPE (event)) {
      case XCB_CONFIGURE_NOTIFY:
        window_handle_configure_notify_event (win, (xcb_configure_notify_event_t *) event);
        break;
      case XCB_EXPOSE:
        window_handle_expose_event (win, (xcb_expose_event_t *) event);
        break;
      case XCB_KEY_PRESS:
        window_handle_key_press_event (win, (xcb_key_press_event_t *) event);
        break;
      case XCB_MAPPING_NOTIFY:
        window_handle_mapping_notify_event (win, (xcb_mapping_notify_event_t *) event);
        break;
      case XCB_PROPERTY_NOTIFY:
        window_handle_property_notify_event (win, (xcb_property_notify_event_t *) event);
        break;
      case XCB_CLIENT_MESSAGE:
        kk_window_event_close (win->events);
        win->alive = 0;
        break;
      default:
        break;
    }
    free (event);
    if (!win->alive)
      break;
  }

  if (win->alive)
    kk_window_event_close (win->events);

  win->alive = 0;
  return NULL;
}

int
kk_window_init (kk_window_t **win, int width, int height)
{
  kk_window_t *result;

  if (kk_widget_init ((kk_widget_t **) &result, sizeof (kk_window_t), NULL) != 0)
    goto error;

  if (kk_event_queue_init (&result->events) != 0)
    goto error;

  if (kk_keys_init (&result->keys) != 0)
    goto error;

  result->connection = xcb_connect (NULL, NULL);
  if (result->connection == NULL)
    goto error;

  result->screen = xcb_setup_roots_iterator (xcb_get_setup (result->connection)).data;
  if (result->screen == NULL)
    goto error;

  if (pthread_create (&result->thread, 0, (void *(*)(void *)) window_event_handler, result) != 0)
    goto error;

  result->width = width;
  result->height = height;

  *win = result;
  return 0;
error:
  kk_window_free (result);
  *win = NULL;
  return -1;
}

int
kk_window_free (kk_window_t *win)
{
  if (win == NULL)
    return 0;

  if (win->alive)
    pthread_cancel (win->thread);

  if (win->thread)
    pthread_join (win->thread, NULL);

  if (win->cairo.context)
    cairo_destroy (win->cairo.context);

  if (win->cairo.surface) {
    cairo_surface_flush (win->cairo.surface);
    cairo_surface_finish (win->cairo.surface);
    cairo_surface_destroy (win->cairo.surface);
  }

  if (win->window)
    xcb_destroy_window (win->connection, win->window);

  /**
   * Disconnecting causes a memory leak if the connectionection had an error,
   * but there's nothing we can do about it. The function xcb_disconnect doesn't
   * do anything if the has_error flag of the xcb_connectionection_t struct is true.
   * However, we can't set this field to false since xcb.h exports xcb_connectionection_t
   * as opaque struct.
   * And besides ruining our valgrind reports, it really doesn't affect us.
   */
  if (win->connection)
    xcb_disconnect (win->connection);

  if (win->events)
    kk_event_queue_free (win->events);

  if (win->keys)
    kk_keys_free (win->keys);

  kk_widget_free ((kk_widget_t*) win);
  return 0;
}

int
kk_window_set_title (kk_window_t *win, const char *title)
{
  size_t len;

  if (win->window == 0u)
    return -1;

  len = kk_str_len (title, KK_WINDOW_MAX_TITLE_LEN);
  if (len == KK_WINDOW_MAX_TITLE_LEN)
    kk_log (KK_LOG_WARNING, "Title exceeds maximum title length.");

  xcb_change_property (win->connection, XCB_PROP_MODE_REPLACE, win->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, (uint32_t) len, title);
  xcb_flush (win->connection);
  return 0;
}

int
kk_window_show (kk_window_t *win)
{
  xcb_visualtype_t *visual_type;
  uint32_t value_mask;
  uint32_t value_list[3];

  value_mask = XCB_CW_BACK_PIXEL
    | XCB_CW_OVERRIDE_REDIRECT
    | XCB_CW_EVENT_MASK;

  value_list[0] = win->screen->black_pixel;
  value_list[1] = 0;
  value_list[2] = XCB_EVENT_MASK_KEY_PRESS
    | XCB_EVENT_MASK_EXPOSURE
    | XCB_EVENT_MASK_PROPERTY_CHANGE
    | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  if ((win->width < 0) | (win->width > (int) UINT16_MAX) | (win->height < 0) | (win->height > (int) UINT16_MAX)) {
    kk_log (KK_LOG_WARNING, "Unsupported window size encounterd.");
    return -1;
  }

  win->window = xcb_generate_id (win->connection);
  xcb_create_window (win->connection,
      win->screen->root_depth,
      win->window,
      win->screen->root,
      0,
      0,
      (uint16_t) win->width,
      (uint16_t) win->height,
      0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      win->screen->root_visual,
      value_mask,
      value_list);

  /**
   * The following code allows us to receive a XCB_CLIENT_MESSAGE
   * event when the window get's closed. This allows us to shut down
   * the xcb session gracefully.
   */
  xcb_atom_t del = window_get_atom (win, "WM_DELETE_WINDOW");
  xcb_atom_t prt = window_get_atom (win, "WM_PROTOCOLS");

  if ((del != XCB_NONE) && (prt != XCB_NONE))
    xcb_set_wm_protocols (win->connection, win->window, prt, 1, &del);

  win->input = window_get_atom (win, KK_WINDOW_INPUT_PROP);

  xcb_map_window (win->connection, win->window);
  xcb_flush (win->connection);

  visual_type = window_get_visual_type (win);
  if (visual_type == NULL)
    return -1;

  win->cairo.surface = cairo_xcb_surface_create (win->connection, win->window, visual_type, win->width, win->height);
  if (cairo_surface_status (win->cairo.surface) != CAIRO_STATUS_SUCCESS)
    return -1;

  win->cairo.context = cairo_create (win->cairo.surface);
  if (cairo_status (win->cairo.context) != CAIRO_STATUS_SUCCESS)
    return -1;

  kk_window_set_title (win, PACKAGE_NAME);
  kk_widget_invalidate ((kk_widget_t*) win);
  return 0;
}

int
kk_window_draw (kk_window_t *win)
{
  if (win->resized)
    cairo_xcb_surface_set_size (win->cairo.surface, win->width, win->height);
  kk_widget_draw ((kk_widget_t*) win, win->cairo.context);
  xcb_flush (win->connection);
  return 0;
}

int
kk_window_get_input (kk_window_t *win)
{
  static char xid[32];

  static char args_shell[] = "/bin/sh";
  static char args_param[] = "-c";
  static char args_code[] = "val=`cat /dev/null | dmenu`; xprop -id $0 -f " KK_WINDOW_INPUT_PROP " 8s -set " KK_WINDOW_INPUT_PROP " \"$val\"";

  if (window_get_xid_str (win, xid, 32) != 0)
    return -1;

  char *const *args = (char *const[]) {
    args_shell,
    args_param,
    args_code,
    xid,
    NULL
  };

  if (fork () == 0) {
    setsid ();
    execvp (args[0], args);
    kk_err (EXIT_FAILURE, "Command %s failed.", args[0]);
  }
  return 0;
}

int
kk_window_get_event_fd (kk_window_t *win)
{
  return kk_event_queue_get_read_fd (win->events);
}
