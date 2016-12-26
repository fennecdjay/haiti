#define _POSIX_C_SOURCE 200809L // strndup
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

#include "cJSON.h"
#include "ini.h"

#define TOKEN_ENV_NAME "GITHUB_TOKEN"
#define GIT_CONFIG	   ".git/config"
struct Config
{
  char* addr;
  char* token;
};

struct string
{
  char *ptr;
  size_t len;
};

CURL*    curl;
CURLcode res;
static struct string s;
static const char* user_agent = "you talking to me?";
static char base_addr[256];
static struct curl_slist* headers = NULL;
static int haiti_config_handler(void* user, const char* section, const char* name,
    const char* value)
{
  struct Config* config = (struct Config*)user;
#define MATCH(s, n) !strcmp(section, s) && !strcmp(name, n)
  if (MATCH("remote \"origin\"", "url")) {
    config->addr = malloc(strlen(value) + 16 * sizeof(char));
    char* tmp = strndup(strstr(value, "github.com"), strlen(value) - 8);
    char* tmp2;
    char ext = 0;
    char c[256];
    memset(c, 0, 256);
    strcat(c, tmp);
    tmp2 = strstr(c, "/");
    if(strstr(tmp2, ".git")) {
      tmp2 = strndup(tmp2, strlen(tmp2) - 4);
      ext = 1;
    }
    sprintf(config->addr, "https://api.github.com/repos%s/issues", tmp2);
    free(tmp);
    if(ext)
      free(tmp2);
  } else {
    return 0;  /* unknown section/name, error */
  }
  return 1;
}

static int haiti_config_get()
{
  struct Config config;
  char c[256];
  config.addr = NULL;
  if(!(config.token = getenv(TOKEN_ENV_NAME))) {
    fprintf(stderr, "No 0auth token found. Please set GITHUB_TOKEN environment variable.\n");
    return 1;
  }
  memset(c, 0, sizeof(c));
  strcat(strcat(c , "Authorization: token "), config.token);
  headers = curl_slist_append(headers, c);
  if (ini_parse(GIT_CONFIG, haiti_config_handler, &config) < 0) {
    fprintf(stderr, "Can't load '" GIT_CONFIG "'.\n");
    return 1;
  }
  if(!config.addr) {
    fprintf(stderr, "Found '" GIT_CONFIG "', but did not find remote.\n");
    fprintf(stderr, "repository might be uninitialized.\n");
    return 1;
  }
  memset(base_addr, 0, sizeof(base_addr));
  strcat(base_addr, config.addr);
  free(config.addr);
  return 0;
}

void init_string(struct string *s)
{
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

static int issue_init()
{
  if(haiti_config_get())
    return 1;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if(!curl)
    return 1;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  return 0;
}

static char issue_send(char* addr, char* postthis)
{
  init_string(&s);
  curl_easy_setopt(curl, CURLOPT_URL, addr);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
  return curl_easy_perform(curl);
}

static int issue_create(char* title, char* l)
{
  cJSON* json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "title", title);
  const char* label[] = { l };
  cJSON* labels = cJSON_CreateStringArray(label, 1);
  cJSON_AddItemToObject(json, "labels", labels);
  char* data = cJSON_Print(json);
  int ret;
  if(issue_send(base_addr, data)!= CURLE_OK)
    return -1;
  cJSON* root = cJSON_Parse(s.ptr);
  cJSON* id = cJSON_GetObjectItem(root, "number");
  ret = id->valueint;
  free(data);
  cJSON_Delete(json);
  cJSON_Delete(root);
  free(s.ptr);
  return ret;
}

static char issue_handle(char* issue, char state)
{
  char addr[256];
  memset(addr, 0, 256);
  strcat(addr, base_addr);
  strcat(addr, "/");
  strcat(addr, issue);
  cJSON* json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "state", state ? "open": "closed");
  char* data = cJSON_Print(json);
  char ret;
  if((ret = issue_send(addr, data)))
    return ret;
  cJSON_Delete(json);
  free(data);
  free(s.ptr);
  return ret;
}

static void issue_clean()
{
  if(headers)
    curl_slist_free_all(headers);
  if(curl)
    curl_easy_cleanup(curl);
  curl_global_cleanup();
}

static char* issue_grab(char* s)
{
  int count = 0;
  while(isdigit(s[count]))
      count++;
  return strndup(s, count);
}

static void issue_sync(char* old)
{
  int line= 0;
  char new[256];
  memset(new, 0, 256);
  strcat(new, old);
  strcat(new, ".tmp");
  FILE* f = fopen(old, "r");
  FILE* g = fopen(new, "w");
  char *text = NULL;
  size_t len = 0;
  ssize_t read;
  while((read = getline(&text, &len, f)) != -1) {
    line++;
    char* mark = (strstr(text, "//"));
    if(mark)
    {
      if((mark[3]) == '#') {
        if(mark[4] == '?')
        {
          int offset = 5;
          char label[256];
          int pos = 0;
          memset(label, 0, sizeof(label));
          // [github] here create issue.
          if(strstr(mark, "[ ]") || strstr(mark, "[S]") ||
              strstr(mark, "[*]") || strstr(mark, "[X]"))
            offset += 4;
          char* c = strdup(text);
          // search for name
          if(strstr((mark+offset), "[") && strstr(mark+offset, "]"))
          {
            offset += 2;
            while((mark+offset)[0] != ']') {
              /*offset++;*/
              label[pos] = (mark + offset)[0];
              pos++;
              offset++;
            }
            offset += 3;
          }
          offset -= 6;
          char* d = strsep(&c, "?");
          fprintf(g, "%s%i%s", d, issue_create(c + offset, label), c);

          free(d);
        }
        else {
          char* n = issue_grab(mark + 4);
          issue_handle(n, 1);
          free(n);
        }
      }
      else {
        if(mark[3] == '%') {
          char* n = issue_grab(mark + 4);
          issue_handle(n, 0);
          free(n);
        }
        fprintf(g, "%s", text);
      }
    }
    else fprintf(g, "%s", text);

  }
  free(text);
  fclose(f);
  fclose(g);
  rename(new, old);
}

void haiti_github(char** file, int n)
{
  int i;
  if(issue_init())
    goto clean;
  for(i = 0; i < n; i++)
    issue_sync(file[i]);
clean:
  issue_clean();
}
