#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>

static const char *GITHUB_API_URL = "https://api.github.com/graphql";
static const char *GITHUB_TOKEN = "TOKEN";
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

CURLcode print_contrib_data(struct CURLInstance curl, const char *username) {
    json_object *root = json_object_new_object();
    json_object_object_add(root, "query", json_object_new_string(CONTRIBUTION_QUERY));
    json_object *variables = json_object_new_object();
    json_object_object_add(variables, "userName", json_object_new_string(username));
    json_object_object_add(root, "variables", variables);
    printf("%s\n", json_object_to_json_string(root));

    curl_easy_setopt(curl.handle, CURLOPT_POSTFIELDS, json_object_to_json_string(root));
    CURLcode code = curl_easy_perform(curl.handle);

    json_object_put(root);

    return code;
}

int main() {
    struct CURLInstance curl = init_curl();
    CURLcode code = print_contrib_data(curl, "janBorowy");
    deinit_curl(curl);
    return 0;
}
