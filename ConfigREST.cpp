
#include "ConfigREST.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <curl/curl.h>
#include <json/json.h>

#include <string.h>
#include <sstream>

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

bool loadConfiguration(Json::Value* root, const std::string host, const unsigned int deploymentID){
    CURL *handle;
    CURLcode res;
    bool loadSuccessful = false;
    Json::Reader reader;

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    handle = curl_easy_init();
    if(handle){
        std::stringstream ss;
        ss << host << "/deployment/"  << deploymentID;
        std::string RESTEndpoint = ss.str();
        curl_easy_setopt(handle, CURLOPT_URL, RESTEndpoint.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&chunk);
        res = curl_easy_perform(handle);
        if(res == CURLE_OK){
            syslog(LOG_DAEMON|LOG_INFO, "Parsing JSON: %s", chunk.memory);
            loadSuccessful = reader.parse(chunk.memory, *root);
        } else {
            syslog(LOG_DAEMON|LOG_ERR, "curl_easy_perform() error: %s", curl_easy_strerror(res));
        } 

        curl_easy_cleanup(handle);
    } else {
        syslog(LOG_DAEMON|LOG_ERR, "Unable to create CURL handle");
    }
    free(chunk.memory);

    return loadSuccessful;
}

