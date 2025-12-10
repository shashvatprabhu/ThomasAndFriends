#include "taf_http_server.h"

static const char *TAG = "taf_http_server";
static char scratch[SCRATCH_BUFSIZE];
static weight_data_t weight_data = {.weight_in_air = 0.0, .weight_in_water = 0.0, .density = 0.0, .val_changed = false};

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

static esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX] = WEB_MOUNT_POINT;

    if (strlen(req->uri) > 0 && req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Handler for getting current weight data */
static esp_err_t weight_data_get_handler(httpd_req_t *req)
{
    char response[256];
    snprintf(response, sizeof(response),
             "{\"weight_in_air\":%.3f,\"weight_in_water\":%.3f,\"density\":%.3f}",
             weight_data.weight_in_air, weight_data.weight_in_water, weight_data.density);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);
    return ESP_OK;
}

/* Handler for posting weight measurements */
static esp_err_t weight_data_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    int received = 0;

    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post weight value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "invalid json response");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Check if weight_in_air is present
    if (cJSON_HasObjectItem(root, "weight_in_air"))
    {
        weight_data.weight_in_air = (float)cJSON_GetObjectItem(root, "weight_in_air")->valuedouble;
        ESP_LOGI(TAG, "Received weight in air: %.3f g", weight_data.weight_in_air);
    }

    // Check if weight_in_water is present
    if (cJSON_HasObjectItem(root, "weight_in_water"))
    {
        weight_data.weight_in_water = (float)cJSON_GetObjectItem(root, "weight_in_water")->valuedouble;
        ESP_LOGI(TAG, "Received weight in water: %.3f g", weight_data.weight_in_water);
    }

    // Calculate density if both weights are available
    if (weight_data.weight_in_air > 0 && weight_data.weight_in_water > 0)
    {
        float water_density = 1.0; // g/cm³
        float buoyant_force = weight_data.weight_in_air - weight_data.weight_in_water;
        if (buoyant_force > 0)
        {
            weight_data.density = (weight_data.weight_in_air * water_density) / buoyant_force;
            ESP_LOGI(TAG, "Calculated density: %.3f g/cm³", weight_data.density);
        }
        else
        {
            weight_data.density = 0.0;
            ESP_LOGW(TAG, "Invalid buoyant force, cannot calculate density");
        }
    }

    cJSON_Delete(root);

    // Send back the calculated data
    char response[256];
    snprintf(response, sizeof(response),
             "{\"weight_in_air\":%.3f,\"weight_in_water\":%.3f,\"density\":%.3f}",
             weight_data.weight_in_air, weight_data.weight_in_water, weight_data.density);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);

    weight_data.val_changed = true;
    return ESP_OK;
}

static esp_err_t start_taf_http_server_private()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "start server failed");
        return ESP_FAIL;
    }

    // POST endpoint for weight data
    httpd_uri_t weight_data_post_uri = {
        .uri = "/api/v1/weight",
        .method = HTTP_POST,
        .handler = weight_data_post_handler,
        .user_ctx = NULL
    };
    if (httpd_register_uri_handler(server, &weight_data_post_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    // GET endpoint for weight data
    httpd_uri_t weight_data_get_uri = {
        .uri = "/api/v1/weight",
        .method = HTTP_GET,
        .handler = weight_data_get_handler,
        .user_ctx = NULL
    };
    if (httpd_register_uri_handler(server, &weight_data_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register get uri failed");
        return ESP_FAIL;
    }

    // Wildcard GET handler for serving static files
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = NULL
    };
    if(httpd_register_uri_handler(server, &common_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register get uri failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

weight_data_t read_weight_data()
{
    return weight_data;
}

void reset_val_changed_weight_data()
{
    weight_data.val_changed = false;
}

void start_taf_http_server()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(MDNS_HOST_NAME);

    connect_to_wifi();
    ESP_ERROR_CHECK(init_fs());
    ESP_ERROR_CHECK(start_taf_http_server_private());
}
