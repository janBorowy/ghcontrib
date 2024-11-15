#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <string.h>
#include <time.h>

static const char *GITHUB_API_URL = "https://api.github.com/graphql";
static const char *GITHUB_TOKEN = "SECRET";
static const char *CONTRIBUTION_QUERY = "query($userName:String!) { "
                                        "  user(login: $userName){ "
                                        "    contributionsCollection { "
                                        "      contributionCalendar { "
                                        "        totalContributions "
                                        "        weeks { "
                                        "          contributionDays { "
                                        "            contributionCount "
                                        "            date "
                                        "            color "
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

struct ContributionDay {
    const char *date;
    const char *color;
    int count;
};

struct ContributionYear {
    int totalContributions;
    struct ContributionDay *days;
    int days_len;
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

size_t write_callback(char *recived_data, size_t size, size_t received_size, void *userdata) {
    struct Buffer *buf = (struct Buffer *) userdata;

    char *p = (char *) realloc (buf->data, buf->size + received_size + 1);
    if (p == NULL) {
        printf("Realloc failed: out of memory");
        return 0;
    }

    buf->data = p;
    memcpy(&(buf->data[buf->size]), recived_data, received_size);
    buf->size += received_size;
    buf->data[buf->size] = 0;

    return received_size;
}

json_object *get_contrib_data(const struct CURLInstance curl, const char *username) {
    json_object *root = json_object_new_object();
    json_object_object_add(root, "query", json_object_new_string(CONTRIBUTION_QUERY));
    json_object *variables = json_object_new_object();
    json_object_object_add(variables, "userName", json_object_new_string(username));
    json_object_object_add(root, "variables", variables);

    struct Buffer *buf = (struct Buffer *) malloc(sizeof(struct Buffer));
    buf->data = (char *) malloc(1);
    buf->size = 0;

    curl_easy_setopt(curl.handle, CURLOPT_POSTFIELDS, json_object_to_json_string(root));
    curl_easy_setopt(curl.handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl.handle, CURLOPT_WRITEDATA, (void *)buf);


    CURLcode code = curl_easy_perform(curl.handle);

    json_object_put(root);
    if (code != CURLE_OK) {
        printf("CURL finished with error\n");
        return NULL;
    }
    struct json_object *result = json_tokener_parse(buf->data);
    return result;
}

int count_contribution_days(const json_object *weeks) {
    int result = 0;
    for (int i = 0; i < json_object_array_length(weeks); i++) {
        result += json_object_array_length(json_object_object_get(json_object_array_get_idx(weeks, i), "contributionDays"));
    }
    return result;
}

struct ContributionYear *serialize_contrib_data(const json_object *contrib_data) {
    json_object *calendar = json_object_object_get(
            json_object_object_get(
                json_object_object_get(
                    json_object_object_get(
                        contrib_data, "data"
                        ), "user"
                    ), "contributionsCollection"
                ), "contributionCalendar"
            );
    struct ContributionYear *result = (struct ContributionYear *) malloc(sizeof(struct ContributionYear));
    const json_object *weeks = json_object_object_get(calendar, "weeks");
    const int day_count = count_contribution_days(weeks);
    result->totalContributions = json_object_get_int(json_object_object_get(calendar, "totalContributions"));
    result->days = (struct ContributionDay *) calloc(day_count, sizeof(struct ContributionDay));
    result->days_len = day_count;
    int current_day_idx = 0;
    for (int i = 0; i < json_object_array_length(weeks); i++) {
        json_object *days = json_object_object_get(json_object_array_get_idx(weeks, i), "contributionDays");
        for (int j = 0; j < json_object_array_length(days); j++) {
            json_object *day = json_object_array_get_idx(days, j);
            result->days[current_day_idx].count = json_object_get_int(json_object_object_get(day, "contributionCount"));
            result->days[current_day_idx].date = json_object_get_string(json_object_object_get(day, "date"));
            current_day_idx++;
            result->days[current_day_idx].color = json_object_get_string(json_object_object_get(day, "color"));
        }
    }

   return result;
}

int main() {
    struct CURLInstance curl = init_curl();
    struct json_object *contrib_data = get_contrib_data(curl, "ThePrimeagen");
    struct ContributionYear *year = serialize_contrib_data(contrib_data);
    free(contrib_data);
    free(year);
    deinit_curl(curl);
    return 0;
}
