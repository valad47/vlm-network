#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#include "lua.h"
#include "lualib.h"

#define TIMEOUT 15

struct MemoryStruct {
  char *memory;
  size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    printf("error: not enough memory\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(mem->memory+mem->size, contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int vlm_RequestGet(lua_State *L){
    struct MemoryStruct body = {0}, header = {0};
    body.memory = malloc(1);
    header.memory = malloc(1);

    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, luaL_checkstring(L, 1));
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, TIMEOUT);

    struct curl_slist *list = NULL;
    if(lua_istable(L, 2)) {
        lua_pushnil(L);
        while(lua_next(L, 2) != 0) {
            curl_slist_append(list, luaL_checkstring(L, -1));
            lua_pop(L, 1);
        }
    }

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);

    CURLcode res;
    if((res = curl_easy_perform(handle)) != CURLE_OK) {
        curl_easy_cleanup(handle);
        curl_slist_free_all(list);
        free(body.memory);
        free(header.memory);
        luaL_error(L, "Failed to perform GET request: %s\n", curl_easy_strerror(res));
    }

    lua_pushstring(L, header.memory);
    lua_pushstring(L, body.memory);

    curl_easy_cleanup(handle);
    curl_slist_free_all(list);
    free(body.memory);
    free(header.memory);

    return 2;
}

int vlm_RequestPost(lua_State *L) {
    struct MemoryStruct body = {0}, header = {0};
    body.memory = malloc(1);
    header.memory = malloc(1);

    const char *data = luaL_checkstring(L, 3);
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, luaL_checkstring(L, 1));
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, TIMEOUT);

    struct curl_slist *list = NULL;
    if(lua_istable(L, 2)) {
        lua_pushnil(L);
        while(lua_next(L, 2) != 0) {
            list = curl_slist_append(list, luaL_checkstring(L, -1));
            lua_pop(L, 1);
        }
    }

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);

    CURLcode res;
    if((res = curl_easy_perform(handle)) != CURLE_OK) {
        curl_easy_cleanup(handle);
        curl_slist_free_all(list);
        free(body.memory);
        free(header.memory);
        luaL_error(L, "Failed to perform POST request: %s\n", curl_easy_strerror(res));
    }

    lua_pushstring(L, header.memory);
    lua_pushstring(L, body.memory);

    curl_easy_cleanup(handle);
    curl_slist_free_all(list);
    free(body.memory);
    free(header.memory);

    return 2;
}

int vlm_RequestPostForm(lua_State *L) {
    struct MemoryStruct body = {0}, header = {0};
    body.memory = malloc(1);
    header.memory = malloc(1);

    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, luaL_checkstring(L, 1));
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, TIMEOUT);

    curl_mime *mime = curl_mime_init(handle);
    if(lua_istable(L, 2)) {
        lua_pushnil(L);
        while(lua_next(L, 2) != 0) {
            curl_mimepart *part = curl_mime_addpart(mime);

            size_t size;
            const char *data = luaL_checklstring(L, -1, &size);
            curl_mime_data(part, data, size);
            curl_mime_name(part, luaL_checkstring(L, -2));
            lua_pop(L, 1);
        }
    }

    if(lua_istable(L, 3)) {
        lua_pushnil(L);
        while(lua_next(L, 3) != 0) {
            curl_mimepart *part = curl_mime_addpart(mime);

            const char *data = luaL_checkstring(L, -1);
            curl_mime_filedata(part, data);
            curl_mime_name(part, luaL_checkstring(L, -2));
            lua_pop(L, 1);
        }
    }


    curl_easy_setopt(handle, CURLOPT_MIMEPOST, mime);

    CURLcode res;
    if((res = curl_easy_perform(handle)) != CURLE_OK) {
        curl_easy_cleanup(handle);
        curl_mime_free(mime);
        free(body.memory);
        free(header.memory);
        luaL_error(L, "Failed to perform POST request: %s\n", curl_easy_strerror(res));
    }

    lua_pushstring(L, header.memory);
    lua_pushstring(L, body.memory);

    curl_easy_cleanup(handle);
    curl_mime_free(mime);
    free(body.memory);
    free(header.memory);

    return 2;
}
