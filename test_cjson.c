#include <stdio.h>
#include <cjson/cJSON.h>

int main() {
    const char* json = "{\"hello\": \"world\"}";
    cJSON* root = cJSON_Parse(json);

    if (root) {
        cJSON* hello = cJSON_GetObjectItem(root, "hello");
        if (cJSON_IsString(hello)) {
            printf("hello: %s\n", hello->valuestring);
        }
        cJSON_Delete(root);
    } else {
        printf("Failed to parse JSON\n");
    }

    return 0;
}
