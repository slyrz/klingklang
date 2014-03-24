#ifndef KK_LIBRARY_H
#define KK_LIBRARY_H

#include <klingklang/base.h>
#include <klingklang/list.h>

typedef struct kk_library_dir kk_library_dir_t;
typedef struct kk_library_file kk_library_file_t;

typedef kk_library_dir_t kk_library_t;  /* shh.. don't tell anyone */

struct kk_library_dir {
  kk_library_dir_t *next;
  kk_library_file_t *children;
  char *root;
  char *base;
};

struct kk_library_file {
  kk_library_file_t *next;
  kk_library_dir_t *parent;
  char *name;
};

size_t kk_library_dir_get_path (kk_library_dir_t *dir, char *dst, size_t len);
size_t kk_library_file_get_path (kk_library_file_t *file, char *dst, size_t len);
size_t kk_library_file_get_album_cover_path (kk_library_file_t *file, char *dst, size_t len);

int kk_library_init (kk_library_t **lib, const char *path);
int kk_library_free (kk_library_t *lib);
int kk_library_find (kk_library_t *lib, const char *keyword, kk_list_t **selection);

#endif
