#include <klingklang/base.h>
#include <klingklang/library.h>
#include <klingklang/str.h>
#include <klingklang/util.h>

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#define get_array_len(x) \
  (sizeof (x) / sizeof ((x)[0]))

static const char *extensions[] = {
  "aac", "flac", "m4a", "mp3", "ogg", "wav", "wma"
};

static const char *cover_filenames[] = {
  "albumart.jpg", "albumart.jpeg", "albumart.png", "cover.jpg", "cover.jpeg", "cover.png"
};

static int
is_regular_file (const char *path)
{
  struct stat sbuf;

  if (stat (path, &sbuf) != 0)
    return 0;
  return (S_ISREG (sbuf.st_mode));
}

/**
 * Crude check if a file is an audio file. It's based on the filename only (!!!)
 * and just based checks for known file extension. Since we use it on
 * readdir results only, we already know filename belongs to a regular file.
 */
static int
is_audio_file (const char *filename)
{
  size_t i;

  /* Find extension part */
  filename = strrchr (filename, '.');
  if (filename == NULL)
    return 0;
  filename++;

  if (*filename) {
    for (i = 0; i < get_array_len (extensions); i++) {
      if (strcasecmp (filename, extensions[i]) == 0)
        return 1;
    }
  }
  return 0;
}

static size_t
path_append (char *dst, const char *src, size_t len, int append_pathsep)
{
  size_t out;

  out = kk_str_cat (dst, src, len);
  if (out >= len)
    return out;

  if (append_pathsep) {
    if (out < len) {
      if ((out == 0) | ((out > 0) && (dst[out - 1] != '/')))
        dst[out++] = '/';
    }
    else
      out++;                    /* dst too small. We need a buffer of src + 1x '/' + 1x '\0' */
  }
  return out;
}

size_t
kk_library_dir_get_path (kk_library_dir_t *dir, char *dst, size_t len)
{
  size_t out;

  *dst = '\0';
  out = path_append (dst, dir->root, len, 1);
  if (out >= len)
    return out;
  return path_append (dst, dir->base, len, 1);
}

size_t
kk_library_file_get_path (kk_library_file_t *file, char *dst, size_t len)
{
  size_t out;

  *dst = '\0';
  out = kk_library_dir_get_path (file->parent, dst, len);
  if (out >= len)
    return out + NAME_MAX;
  return path_append (dst, file->name, len, 0);
}

size_t
kk_library_file_get_album_cover_path (kk_library_file_t *file, char *dst, size_t len)
{
  size_t i;
  size_t out;
  size_t rem;
  char *end;

  out = kk_library_dir_get_path (file->parent, dst, len);
  if (out >= len)
    goto error;

  end = dst + out;
  rem = len - out;
  for (i = 0; i < get_array_len (cover_filenames); i++) {
    out = kk_str_cpy (end, cover_filenames[i], rem);
    if (out >= rem)
      goto error;

    if (is_regular_file (dst))
      return out + (len - rem);
  }
  return 0;
error:
  return out + 32;              /* 32 = space for album cover filename */
}

static int
library_dir_load (kk_library_dir_t *dir)
{
  size_t len_root;
  size_t len_base;
  size_t len;

  struct dirent *ent = NULL;
  DIR *dirst = NULL;

  char *pfst;
  char *plst;

  len_root = strlen (dir->root);
  len_base = strlen (dir->base);

  /**
   * If these two paths don't end with a slash, we have to add one
   * for each and the required buffer size increases.
   */
  len_root += (len_root > 0) && (dir->root[len_root - 1] != '/');
  len_base += (len_base > 0) && (dir->base[len_base - 1] != '/');

  /**
   * Build string "root/base/", so that we can append the names of the
   * directory entries to get a valid path.
   *
   *                    root/base/\0
   *                    ^         ^
   *                    pfst      plst
   *
   * pfst points to the first char of the path. plst points to the terminating
   * zero char. Calling strcpy (plst, dirent->d_name) leads to the full path
   * of a given dirent.
   */
  len = kk_get_next_pow2 (len_root + len_base + NAME_MAX + 1);

  pfst = calloc (len, sizeof (char));
  if (pfst == NULL)
    goto error;

  len_root = path_append (pfst, dir->root, len, 1);
  len_base = path_append (pfst, dir->base, len, 1);
  plst = pfst + len_base;

  dirst = opendir (pfst);
  if (dirst == NULL)
    goto error;

  for (;;) {
    ent = readdir (dirst);

    if (ent == NULL)
      break;

    /* Ignore "." and ".." */
    if (ent->d_name[0] == '.') {
      if ((ent->d_name[1] == '\0') || ((ent->d_name[1] == '.') & (ent->d_name[2] == '\0')))
        continue;
    }

    if (ent->d_type == DT_DIR) {
      kk_library_dir_t *next;
      kk_library_dir_t *temp;

      next = calloc (1, sizeof (kk_library_dir_t));
      if (next == NULL)
        goto error;

      /**
       * kk_str_cpy () should never cause this error, but if it does, just
       * ignore this entry instead of giving up...
       */
      if (kk_str_cpy (plst, ent->d_name, NAME_MAX) >= NAME_MAX) {
        free (next);
        continue;
      }

      next->root = dir->root;
      next->base = strdup (pfst + len_root);
      library_dir_load (next);

      /**
       * If the result has no children, it doesn't contain any regular files
       * but it might contain directories containing regular files, so we
       * set next to next->next instead of NULL.
       */
      if (next->children == NULL) {
        temp = next;
        next = next->next;
        free (temp->base);
        free (temp);
      }

      /**
       * If next != NULL, we found one or mire directories in dir that
       * cointained some music files and we append it to the linked list
       * of directories.
       */
      if (next) {
        temp = dir;
        while (temp && temp->next)
          temp = temp->next;
        temp->next = next;
      }
    }

    if ((ent->d_type == DT_REG) && (is_audio_file (ent->d_name))) {
      kk_library_file_t *next;

      next = calloc (1, sizeof (kk_library_file_t));
      if (next == NULL)
        goto error;

      next->parent = dir;
      next->name = strdup (ent->d_name);
      next->next = dir->children;
      dir->children = next;
    }
  }
  closedir (dirst);
  free (pfst);
  return 0;
error:
  if (dirst)
    closedir (dirst);
  free (pfst);
  return -1;
}

int
kk_library_init (kk_library_t **lib, const char *path)
{
  kk_library_dir_t *result;

  result = calloc (1, sizeof (kk_library_dir_t));
  if (result == NULL)
    goto error;

  /* strdup these so we can call free on all strings later  */
  result->root = strdup (path);
  result->base = strdup ("");

  if ((result->root == NULL) || (result->base == NULL))
    goto error;

  if (library_dir_load (result) != 0)
    goto error;

  if (result->children == NULL) {
    kk_library_dir_t *next = result->next;

    if (next == NULL)
      free (result->root);

    free (result->base);
    free (result);
    result = next;
  }

  *lib = result;
  return 0;
error:
  if (result)
    kk_library_free (result);
  *lib = NULL;
  return -1;
}

int
kk_library_free (kk_library_t *lib)
{
  kk_library_dir_t *dir = lib;
  kk_library_file_t *file;

  void *temp;

  if (lib == NULL)
    return 0;

  /* All dir structs share the same root pointer, so we only free it once */
  free (dir->root);

  while (dir) {
    file = dir->children;

    while (file) {
      temp = file->next;
      free (file->name);
      free (file);
      file = temp;
    }

    temp = dir->next;
    free (dir->base);
    free (dir);
    dir = temp;
  }

  /**
   * No need to free lib since we already freed it in the first
   * while-iteration
   */
  return 0;
}

static int
library_file_cmp (const void *a, const void *b)
{
  const kk_library_file_t *fa = (const kk_library_file_t *) a;
  const kk_library_file_t *fb = (const kk_library_file_t *) b;

  int result;

  result = kk_str_natcmp (fa->parent->base, fb->parent->base);
  if (result == 0)
    result = kk_str_natcmp (fa->name, fb->name);
  return result;
}

int
kk_library_find (kk_library_t *lib, const char *keyword, kk_list_t **sel)
{
  kk_list_t *result = NULL;
  kk_library_file_t *file;
  kk_library_dir_t *dir;

  kk_str_search_t *search = NULL;

  kk_str_match_t match_base;
  kk_str_match_t match_file;

  if ((keyword == NULL) || (*keyword == '\0'))
    goto error;

  if (kk_list_init (&result) != 0)
    goto error;

  if (kk_str_search_init (&search, keyword, " ") != 0)
    goto error;

  for (dir = lib; dir != NULL; dir = dir->next) {
    /* Search directory name */
    kk_str_search_find_all (search, dir->base, &match_base);

    for (file = dir->children; file != NULL; file = file->next) {
      /* Search file name */
      kk_str_search_find_all (search, file->name, &match_file);

      /* If matches in directory name and file name contain all patterns */
      if (kk_str_search_matches_all (search, match_base | match_file)) {
        if (kk_list_append (result, file) != 0)
          goto error;
      }
    }
  }

  kk_str_search_free (search);
  if (result->len)
    kk_list_sort (result, library_file_cmp);
  *sel = result;
  return 0;
error:
  kk_str_search_free (search);
  kk_list_free (result);
  *sel = NULL;
  return -1;
}
