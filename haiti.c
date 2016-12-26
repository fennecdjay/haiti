#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L // c99 warning
#define _ISOC99_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "filetype.h"

#ifdef USE_GITHUB
extern void haiti_github(char** file, int n);
#endif

#include "haiti.h"

uint n_dirs = 0;
uint n_file = 0;
uint n_tags = 0;
pstr dirs = NULL;
pstr file = NULL;
pstr tags = NULL;

static bool sync   = 0;
static bool by_tag = 0;

bool find(cstr str, pstr src, uint size)
{
  uint i;
  if(!str || !src)
    return 0;
  for(i = 0; i < size; i++)
    if(!strcmp(str, src[i]))
      return 1;
  return 0;
}

const cstr file_ext(cstr s)
{
  uint i;
  cstr c;
  if(s[0] == '.')
    return NULL;
  if(!(c = strchr(s, '.')) || !(c++))
    return NULL;
  for(i = 0; i < sizeof(filetype)/sizeof(filetype[0]); i++)
    if(!strcmp(c, filetype[i][0]))
      return filetype[i][1];
  return NULL;
}

static pstr append_cstr(pstr array, cstr str, uint* i)
{
  if(!array) {
    array = malloc(sizeof(pstr));
    array[*i] = strdup(str);
  } else {
    array = realloc(array, (*i+1) * sizeof(pstr));
    array[*i] = strdup(str);
  }
  (*i)++;
  return array;
}

static pstr append_pstr(pstr dest, pstr src, uint* dest_size, uint src_size)
{
  uint i, size;
  for(i = 0; i < src_size; i++) {
    size = *dest_size + i;
    dest = append_cstr(dest, src[i], &size);
  }
  *dest_size += src_size;
  return dest;
}

static void free_array(pstr a, uint size)
{
  uint i;
  if(!a)
    return;
  for(i = 0; i < size; i++)
    free(a[i]);
  free(a);
}

static pstr remove_dup(pstr src, uint* size)
{
  uint i, count = 0;
  pstr a = NULL;
  for(i = 0; i < *size; i++)
    if(!find(src[i], a, count))
      a = append_cstr(a, src[i], &count);
  free_array(src, *size);
  *size = count;
  return a;
}

static pstr list_dir(cstr dirname, uint* n)
{
  uint i, count = 0;
  pstr ret = NULL;
  cstr path = realpath(dirname, NULL);
  struct dirent** namelist;
  *n = scandir(dirname, &namelist, NULL, NULL);
  for(i = 0; i < *n; i++)
  {
    char c[strlen(path) + 1 + strlen(namelist[i]->d_name)];
    if(!file_ext(namelist[i]->d_name))
      goto next;
    memset(c, 0, sizeof(c));
    strcat(strcat(strcat(c, path), "/"), namelist[i]->d_name);
    ret = append_cstr(ret, c, &count);
next:
    free(namelist[i]);
  }
  free(namelist);
  free(path);
  *n = count;
  return ret;
}

static int sort(const void* a, const void* b)
{
  return strcmp(*(pstr)a, *(pstr)b);
}

static void haiti_sort()
{
  uint i;
  for(i = 0; i < n_dirs; i++) {
    uint size = 0;
    pstr list = list_dir(dirs[i], &size);
    file = append_pstr(file, list, &n_file, size);
    free_array(list, size);
  }
  file = remove_dup(file, &n_file);
  qsort(file, n_file, sizeof(pstr), sort);
  tags = remove_dup(tags, &n_tags);
  qsort(tags, n_tags, sizeof(pstr), sort);
}

static void haiti_parse(int argc, char** argv)
{
  uint i;
  for(i = 1; i < argc; i++) {
    struct stat s;
    if(!strcmp(argv[i], "-sync")) {
      sync = 1;
      continue;
    }
    else if(!strcmp(argv[i], "-tags")) {
      by_tag = 1;
      continue;
    }
    if(stat(argv[i],&s) == 0) {
      cstr path = realpath(argv[i], NULL);
      if(s.st_mode & S_IFDIR)
        dirs = append_cstr(dirs, path, &n_dirs);
      else if(s.st_mode & S_IFREG)
        file = append_cstr(file, path, &n_file);
      else
        printf("'%s' is not a regular file\n", argv[i]);
      if(path)
        free(path);
    }
    else
      tags = append_cstr(tags, argv[i], &n_tags);
  }
}

static void haiti_sync()
{
  if(sync)
#ifdef USE_GITHUB
    haiti_github(file, n_file);
#else
  fprintf(stderr, "-sync option not enabled. please recompile with '-DUSE_GITHUB'.\n");
#endif
}

static void haiti_print()
{
  if(by_tag)
    haiti_print_tags();
  else
    haiti_print_file();
}

static void haiti_clean()
{
  free_array(dirs, n_dirs);
  free_array(tags, n_tags);
  free_array(file, n_file);
}

int main(int argc, pstr argv)
{
  haiti_parse(argc, argv);
  haiti_sort();
  haiti_sync();
  haiti_print();
  haiti_clean();
  return 0;
}
