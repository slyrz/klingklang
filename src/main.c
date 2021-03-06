/**
 * Copyright (c) 2013, the klingklang developers.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <klingklang/base.h>
#include <klingklang/library.h>
#include <klingklang/player.h>
#include <klingklang/ui/cover.h>
#include <klingklang/ui/image.h>
#include <klingklang/ui/progressbar.h>
#include <klingklang/ui/window.h>
#include <klingklang/util.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define KK_WINDOW_WIDTH         300
#define KK_WINDOW_HEIGHT        220

typedef struct kk_context kk_context_t;

struct kk_context {
  kk_event_loop_t *loop;
  kk_library_t *library;
  kk_player_t *player;
  kk_window_t *window;
};

static void
on_player_pause (kk_context_t *ctx, kk_player_event_pause_t *event)
{
  (void) ctx;
  (void) event;

  kk_log (KK_LOG_INFO, "Player pause toggled.");
  return;
}

static void
on_player_progress (kk_context_t *ctx, kk_player_event_progress_t *event)
{
  kk_log (KK_LOG_DEBUG, "Player progress %f [0,1].", (double) event->progress);
  kk_progressbar_set_value (ctx->window->progressbar, event->progress);
}

static void
on_player_seek (kk_context_t *ctx, kk_player_event_seek_t *event)
{
  kk_log (KK_LOG_DEBUG, "Player seeked position %f [0,1].", (double) event->perc);
  kk_progressbar_set_value (ctx->window->progressbar, event->perc);
  kk_widget_invalidate ((kk_widget_t*) ctx->window->progressbar);
}

static void
on_player_start (kk_context_t *ctx, kk_player_event_start_t *event)
{
  kk_log (KK_LOG_INFO, "Player started playing '%s'.", event->file->name);

  kk_progressbar_set_value (ctx->window->progressbar, 0.0);
  kk_cover_load (ctx->window->cover, event->file);
  kk_window_set_title (ctx->window, event->file->name);
  kk_window_update (ctx->window);
}

static void
on_player_stop (kk_context_t *ctx, kk_player_event_stop_t *event)
{
  (void) event;

  kk_log (KK_LOG_INFO, "Player stopped.");
  kk_progressbar_set_value (ctx->window->progressbar, 1.0);
  kk_window_update (ctx->window);
}

static void
on_window_input (kk_context_t *ctx, kk_window_event_input_t *event)
{
  kk_list_t *sel = NULL;

  if (*event->text == '\0')
    goto cleanup;

  if (kk_library_find (ctx->library, event->text, &sel) < 0) {
    kk_log (KK_LOG_ERROR, "Searching for '%s' in library failed.", event->text);
    goto cleanup;
  }

  kk_log (KK_LOG_INFO, "%d files matching '%s'.", sel->len, event->text);
  if (sel->len == 0)
    goto cleanup;

  if (kk_player_queue_add (ctx->player->queue, sel) != 0) {
    kk_log (KK_LOG_ERROR, "Could not add search result for '%s' to player queue", event->text);
    goto cleanup;
  }

  if (kk_player_start (ctx->player) != 0)
    kk_log (KK_LOG_ERROR, "Player start failed.");

cleanup:
  kk_list_free (sel);
  free (event->text);
}

static void
on_window_key_press (kk_context_t *ctx, kk_window_event_key_press_t *event)
{
  if ((event->mod & KK_MOD_CONTROL) != KK_MOD_CONTROL)
    return;

  switch (event->key) {
    case KK_KEY_R:
      kk_player_seek (ctx->player, 0.0f);
      break;
    case KK_KEY_A:
      kk_window_get_input (ctx->window);
      break;
    case KK_KEY_N:
      kk_player_next (ctx->player);
      break;
    case KK_KEY_P:
      kk_player_pause (ctx->player);
      break;
    case KK_KEY_C:
      kk_player_queue_clear (ctx->player->queue);
      break;
    default:
      break;
  }
}

static void
on_player_event (kk_event_loop_t *loop, int fd, kk_context_t *ctx)
{
  kk_event_t event;
  ssize_t rs;

  (void) loop;

  while (rs = read (fd, &event, sizeof (kk_event_t)), rs > 0) {
    /* Sanity check... should not be necessary */
    if (rs != sizeof (kk_event_t)) {
      kk_log (KK_LOG_WARNING, "Invalid size read from event loop.");
      continue;
    }

    switch (event.type) {
      case KK_PLAYER_SEEK:
        on_player_seek (ctx, (kk_player_event_seek_t *) &event);
        break;
      case KK_PLAYER_START:
        on_player_start (ctx, (kk_player_event_start_t *) &event);
        break;
      case KK_PLAYER_STOP:
        on_player_stop (ctx, (kk_player_event_stop_t *) &event);
        break;
      case KK_PLAYER_PAUSE:
        on_player_pause (ctx, (kk_player_event_pause_t *) &event);
        break;
      case KK_PLAYER_PROGRESS:
        on_player_progress (ctx, (kk_player_event_progress_t *) &event);
        break;
      default:
        kk_log (KK_LOG_WARNING, "Read unkown player event.");
        break;
    }
  }
}

static void
on_window_event (kk_event_loop_t *loop, int fd, kk_context_t *ctx)
{
  kk_event_t event;
  ssize_t rs;

  while (rs = read (fd, &event, sizeof (kk_event_t)), rs > 0) {
    /* Sanity check... should not be necessary */
    if (rs != sizeof (kk_event_t)) {
      kk_log (KK_LOG_WARNING, "Invalid size read from event loop.");
      continue;
    }

    switch (event.type) {
      case KK_WINDOW_KEY_PRESS:
        on_window_key_press (ctx, (kk_window_event_key_press_t *) &event);
        break;
      case KK_WINDOW_INPUT:
        on_window_input (ctx, (kk_window_event_input_t *) &event);
        break;
      case KK_WINDOW_CLOSE:
        kk_event_loop_exit (loop);
        break;
      default:
        kk_log (KK_LOG_WARNING, "Read unkown window event.");
        break;
    }
  }
}

int
main (int argc, char **argv)
{
  static kk_context_t context;
  char *path;

  if (argc < 2)
    path = getenv ("MUSICPATH");
  else
    path = argv[1];

  if (kk_player_init (&context.player) < 0)
    kk_err (EXIT_FAILURE, "Could not init player.");

  if (kk_library_init (&context.library, path) < 0)
    kk_err (EXIT_FAILURE, "Could not open music library.");

  if (kk_window_init (&context.window, KK_WINDOW_WIDTH, KK_WINDOW_HEIGHT) < 0)
    kk_err (EXIT_FAILURE, "Could not initialize window.");

  if (kk_event_loop_init (&context.loop, 2) != 0)
    kk_err (EXIT_FAILURE, "Could not initialize event loop.");

  kk_event_loop_add (context.loop, kk_player_get_event_fd (context.player),
      (kk_event_func_f) on_player_event, &context);
  kk_event_loop_add (context.loop, kk_window_get_event_fd (context.window),
      (kk_event_func_f) on_window_event, &context);

  kk_window_show (context.window);
  kk_player_start (context.player);
  kk_event_loop_run (context.loop);
  kk_player_stop (context.player);

  kk_event_loop_free (context.loop);
  kk_library_free (context.library);
  kk_player_free (context.player);
  kk_window_free (context.window);

  return EXIT_SUCCESS;
}
