
#include "ConfigREST.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <curl/curl.h>
#include <json/json.h>

#include <string.h>

// Partially based on: http://curl.haxx.se/libcurl/c/getinmemory.html

struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL){
        syslog(LOG_DAEMON|LOG_ERR,"Not enough memory (realloc returned NULL)");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

bool loadConfiguration(Json::Value* root, const std::string host){
    CURL *handle;
    CURLcode res;
    bool loadSuccessful = false;
    Json::Reader reader;

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    handle = curl_easy_init();
    if(handle){
        curl_easy_setopt(handle, CURLOPT_URL, host.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&chunk);
        res = curl_easy_perform(handle);
        if(res == CURLE_OK){
            loadSuccessful = reader.parse(chunk.memory, *root);
        } 

        curl_easy_cleanup(handle);
    }
    free(chunk.memory);

    return loadSuccessful;
}
