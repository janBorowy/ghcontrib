#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>

static const char *GITHUB_API_URL = "https://api.github.com/graphql";
static const char *GITHUB_TOKEN = "token";
static const char *CONTRIBUTION_QUERY = "query($userName:String!) { "
                                        "  user(login: $userName){ "
                                        "    contributionsCollection { "
                                        "      contributionCalendar { "
                                        "        totalContributions "
                                        "        weeks { "
                                        "          contributionDays { "
                                        "            contributionCount "
                                        "            date "
                                        "          } "
                                        "        } "
                                        "      } "
                                        "    } "
                                        "  } "
                                        "}";

struct CURLInstance {
    CURL *handle;
    struct curl_slist *headers;
};

struct Buffer {
    char *data;
    size_t size;
};

struct CURLInstance init_curl() {
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, GITHUB_API_URL);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, curl_version());
    char authorization_header[256] = "Authorization: bearer ";
    strcat(authorization_header, GITHUB_TOKEN);
    struct curl_slist *headers = curl_slist_append(NULL, authorization_header);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    struct CURLInstance curlInstance;
    curlInstance.handle = handle;
    curlInstance.headers = headers;
    return curlInstance;
}

void deinit_curl(struct CURLInstance curl) {
    curl_slist_free_all(curl.headers);
    curl_easy_cleanup(curl.handle);
    curl_global_cleanup();
}

size_t write_callback(char *recived_data, size_t recived_size, size_t nmemb, void *userdata) {
    struct Buffer *buf = (struct Buffer *) userdata;

    char *p = realloc (buf->data, buf->size + recived_size + 1);
    if (p == NULL) {
        printf("Realloc failed: out of memory");
        return 0;
    }

    buf->data = p;
    memcpy(&(buf->data[buf->size]), recived_data, recived_size);
    buf->size += recived_size;
    buf->data[buf->size] = 0;

    return nmemb;
}

json_object *get_contrib_data(struct CURLInstance curl, const char *username) {
    json_object *root = json_object_new_object();
    json_object_object_add(root, "query", json_object_new_string(CONTRIBUTION_QUERY));
    json_object *variables = json_object_new_object();
    json_object_object_add(variables, "userName", json_object_new_string(username));
    json_object_object_add(root, "variables", variables);

    struct Buffer *buf;
    buf->data = malloc(1);
    buf->size = 0;

    curl_easy_setopt(curl.handle, CURLOPT_POSTFIELDS, json_object_to_json_string(root));
    curl_easy_setopt(curl.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl.handle, CURLOPT_WRITEDATA, (void *)buf);

    json_object_put(root);

    CURLcode code = curl_easy_perform(curl.handle);

    if (code != CURLE_OK) {
        printf("CURL finished with error\n");
        return NULL;
    }
    struct json_object *result = json_tokener_parse(buf->data);
    return result;
}

int main() {
    struct CURLInstance curl = init_curl();
    struct json_object *contrib = get_contrib_data(curl, "janBorowy");
    printf("%s", json_object_to_json_string(contrib));
    deinit_curl(curl);
    return 0;
}
