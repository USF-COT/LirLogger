/* ConfigREST - Given a host, it makes a GET request.  Expects to receive a JSON response
 * containing the deployment configuration.  Uses JSONPP to parse the response and stuffs
 * it in a passed Json::Value object.
 */

#include <json/json.h>

#include <string.h>

// Partially based on: http://curl.haxx.se/libcurl/c/getinmemory.html

bool loadConfiguration(Json::Value* root, const std::string host);
