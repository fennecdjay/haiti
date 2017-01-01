#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "haiti.h"

typedef struct Line_* Line;
struct Line_
{
  uint  line;
  uint  note; // comment level
  enum box_type type;
  cstr         text;
  cstr         name; // name of the tag
  Line          more; // comments, from start
  Line          last; // last comment
  Line          next;
  cstr         ghis; // GitHub Issue String;
};

typedef struct File_* File;
struct File_
{
  cstr name;
  Line more, last;
  Line head, foot;
  File next;
};

static cstr curr_tag;
static bool term = 0;
static pthread_t* pass_list;
static File*      file_list;

static Line new_Line(uint line, enum box_type type, cstr text, cstr name, cstr ghis, uint note)
{
  Line a  = calloc(1, sizeof(struct Line_));
  a->line = line;
  a->type = type;
  a->text = text;
  a->name = name;
  a->ghis = ghis;
  a->note = note;
  return a;
}

static void free_Line(Line a)
{
  if(a->next)
    free_Line(a->next);
  if(a->more)
    free_Line(a->more);
  if(a->name)
    free(a->name);
  if(a->ghis)
    free(a->ghis);
  free(a->text);
  free(a);
}

static void free_File(File a)
{
  if(!a)
    return;
  free(a->name);
  if(a->more)
    free_Line(a->more);
  if(a->head)
    free_Line(a->head);
  free(a);
}

static cstr grab_ghis(cstr s, uint* offset)
{
  s += *offset;
  while(isspace(s[0])) {
    s++;
    *offset++;
  }
  if(!strncmp(s, "#", 1) || !strncmp(s, "%", 1)) {
    uint count = 1;
    if(s[1] == '?') {
      *offset += 2;
      return strndup(s, count++);
    }
    while(isdigit(s[count]))
      count++;
    *offset += count;
    return strndup(s, count);
  }
  return NULL;
}

static uint grab_note(cstr s, uint* offset)
{
  uint i = 0;
  s += *offset;
  while(isspace(s[0])) {
    s++;
    *offset++;
  }
  while(s[i] == '|')
    i++;
  if(i)
    *offset += i;
  return i;
}

static cstr grab_name(cstr s, uint* offset)
{
  uint i = 0;
  s += *offset;
  while(isspace(s[0])) {
    s++;
    *offset++;
  }
  if(s[0] != '[')
    return NULL;
  s++;
  while(s[i] != ']')
    i++;
  *offset += i + 2;
  return strndup(s, i);
}

static enum box_type grab_type(cstr s, uint* offset)
{
  s += *offset;
  while(isspace(s[0])) {
    s++;
    *offset++;
  }
  enum box_type ret;
  if(s[0] != '[' && s[2] != ']')
    ret = BOX_NONE;
  else if(s[1] == ' ')
    ret = BOX_TODO;
  else if(s[1] == 'S')
    ret = BOX_SKIP;
  else if(s[1] == '*')
    ret = BOX_TEST;
  else if(s[1] == 'X')
    ret = BOX_DONE;
  else
    ret = BOX_NONE;
  if(ret != BOX_NONE)
    *offset += 3;
  return ret;
}

static cstr grab_text(cstr s, uint* offset)
{
  s += *offset;
  while(isspace(s[0])) {
    s++;
    *offset++;
  }
  return strndup(s, strlen(s) - 1);
}

static void append_line(File f, Line l)
{
  uint level = l->note;
  if(level) {
    level--;
    Line m = f->foot;
    if(!m) {
      if(!f->more) {
        f->more = (f->last = l);
        return;
      }
      m = f->last;
    }
    while(level) {
      if(!m->more)
        break;
      m = m->last;
      level--;
    }
    if(!m->last)
      m->more = (m->last = l);
    else
      m->last = (m->last->next = l);
    return;
  }
  if(f->foot)
    f->foot = (f->foot->next = l);
  else
    f->foot = (f->head  = l);
}

static void parse_file(File f)
{

  uint line = 0;
  size_t  size  = 0;
  ssize_t read;
  cstr    text = NULL;
  cstr    str = file_ext(f->name);
  FILE*   file = fopen(f->name, "r");

  while((read = getline(&text, &size, file)) != -1) {
    uint offset;
    struct Line_ l;
    cstr base = strstr(text, str);
    line++;
    if(!base || (strlen(base) < strlen(str) + 4))
      continue;
    offset = strlen(str) ? strlen(str) + 1 : 0;
    l.ghis = grab_ghis(base, &offset);
    l.note = grab_note(base, &offset);
    l.type = grab_type(base, &offset);
    l.name = grab_name(base, &offset);
    l.text = grab_text(base, &offset);
    if(!l.name && !l.note && l.type == BOX_NONE) {
      if(l.ghis)
        free(l.ghis);
      if(l.text)
        free(l.text);
      continue;
    }
    append_line(f,  new_Line(line,  l.type, l.text, l.name, l.ghis, l.note));
  }
  free(text);
  fclose(file);
}

static void* do_File(void* arg)
{
  File a = calloc(1, sizeof(struct File_));
  a->name = strdup((cstr)arg);
  parse_file(a);
  pthread_exit(a);
}

static void print_file(cstr s)
{
  if(term)
    printf("📖  \033[38;5;214m%s\033[0m 📖\n", s);
  else
    printf("\t{ %s }\n", s);
}

static void print_ghis(Line l)
{
  if(!l->ghis)
    return;
  if(term)
    printf("\033[38;5;122m%s\033[0m ", l->ghis);
  else
    printf("%s ", l->ghis);
}

static void print_name(cstr s)
{
  if(!s)
    return;
  if(term)
    printf("📝\t[ \033[38;5;180m%s\033[0m ]\n", s);
  else
    printf("[ %s ]\n", s);
}

static void print_note(Line l)
{
  uint i;
  for(i = 0; i < l->note; i++)
    putchar('|');
  putchar(' ');
}

static void print_type(Line l)
{
  switch(l->type) {
    case BOX_NONE:
      break;
    case BOX_TODO:
      if(term)
        printf("\033[38;5;196m[ ]\033[0m ");
      else
        printf("[ ] ");
      break;
    case BOX_SKIP:
      if(term)
        printf("\033[38;5;136m[S]\033[0m ");
      else
        printf("[S] ");
      break;
    case BOX_TEST:
      if(term)
        printf("\033[38;5;69m[*]\033[0m ");
      else
        printf("[*] ");
      break;
    case BOX_DONE:
      if(term)
        printf("\033[38;5;46m[X]\033[0m ");
      else
        printf("[X] ");
  }
}

static void print_more(Line l)
{
  if(!l)
    return;
  if(term) {
    printf("💬\t\033[38;5;13%im", l->note *2);
    print_note(l);
    print_type(l);
    printf("\033[38;5;13%im%s\033[0m\n", l->note * 2, l->text);
  } else {
    print_note(l);
    print_type(l);
    printf("\t%s\n", l->text);
  }
  print_more(l->more);
  print_more(l->next);
}

static void print_text(Line l)
{
  printf("%s\n", l->text);
}

static void print_line(Line l)
{
  if(term)
    printf("\033[1m%i\033[0m: ", l->line);
  else
    printf("%i: ", l->line);
}

static void print_init()
{
  uint i;
  term = isatty(fileno(stdout));
  pass_list = malloc(n_file * sizeof(pthread_t));
  file_list = malloc(n_file * sizeof(struct File_));
  for(i = 0; i < n_file; i++)
    pthread_create(&pass_list[i], NULL, do_File, file[i]);
  for(i = 0; i < n_file; i++)
    pthread_join(pass_list[i], (void**)&file_list[i]);
}

static void print_clean()
{
  uint i;
  for(i = 0; i < n_file; i++)
    free_File(file_list[i]);
  free(file_list);
  free(pass_list);
}

void haiti_print_file()
{
  uint i;
  print_init();
  for(i = 0; i < n_file; i++) {
    Line l;
    bool has_tags = 1;
    if(!file_list[i]->head)
      continue;
    curr_tag = NULL;
    l = file_list[i]->head;
    if(tags) {
      has_tags = 0;
      while(l) {
        if(find(l->name, tags, n_tags)) {
          has_tags = 1;
          break;
        }
        l = l->next;
      }
    }
    if(has_tags)
    {
      print_file(file_list[i]->name);
      print_more(file_list[i]->more);
    }
    l = file_list[i]->head;
    while(l) {
      if(!n_tags || find(l->name, tags, n_tags)) {
        if(!curr_tag ||  (l->name && strcmp(l->name, curr_tag)))
          print_name(l->name);
        print_line(l);
        print_ghis(l);
        print_type(l);
        print_text(l);
        print_more(l->more);
        curr_tag = l->name ? l->name : curr_tag;
      }
      l = l->next;
    }
  }
  print_clean();
}

void haiti_print_tags()
{
  uint i, j;
  print_init();
  for(j = 0; j < n_tags; j++) {
    print_name(tags[j]);
    for(i = 0; i < n_file; i++) {
      Line l;
      bool has_tags = 1;
      if(!file_list[i]->head)
        continue;
      curr_tag = NULL;
      l = file_list[i]->head;
      if(tags && n_tags) {
        has_tags = 0;
        while(l) {
          if(!strcmp(l->name, tags[j])) {
            has_tags = 1;
            break;
          }
          l = l->next;
        }
      }
      if(!has_tags)
        continue;
      print_file(file_list[i]->name);
      print_more(file_list[i]->more);
      l = file_list[i]->head;
      while(l) {
        if(!strcmp(tags[j], l->name)) {
          print_line(l);
          print_ghis(l);
          print_type(l);
          print_text(l);
          print_more(l->more);
        }
        l = l->next;
      }
    }
  }
  print_clean();
}

