#include "taf_websocket_handler.h"

static const char *TAG = "websocket_server";

static weight_data_t weight_data = {
    .weight_in_air = 0.0,
    .weight_in_water = 0.0,
    .density = 0.0,
    .new_data = false
};

static QueueHandle_t client_queue;
const static int client_queue_size = 10;

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

void websocket_callback(uint8_t num, WEBSOCKET_TYPE_t type, char *msg, uint64_t len)
{
    switch (type)
    {
    case WEBSOCKET_CONNECT:
        ESP_LOGI(TAG, "client %i connected!", num);
        break;
    case WEBSOCKET_DISCONNECT_EXTERNAL:
        ESP_LOGI(TAG, "client %i sent a disconnect message", num);
        break;
    case WEBSOCKET_DISCONNECT_INTERNAL:
        ESP_LOGI(TAG, "client %i was disconnected", num);
        break;
    case WEBSOCKET_DISCONNECT_ERROR:
        ESP_LOGI(TAG, "client %i was disconnected due to an error", num);
        break;
    case WEBSOCKET_TEXT:
        if (len)
        {
            weight_data.new_data = true;

            switch (msg[0])
            {
            case 'A': // Weight in Air
                ESP_LOGI(TAG, "got weight in air: %s", &(msg[1]));
                weight_data.weight_in_air = atof(&msg[1]);

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
                }
                break;
            case 'W': // Weight in Water
                ESP_LOGI(TAG, "got weight in water: %s", &(msg[1]));
                weight_data.weight_in_water = atof(&msg[1]);

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
                }
                break;
            default:
                ESP_LOGI(TAG, "got an unknown message with length %i", (int)len);
                break;
            }
        }
        break;
    case WEBSOCKET_BIN:
        ESP_LOGI(TAG, "client %i sent binary message of size %lu", num, (uint32_t)len);
        break;
    case WEBSOCKET_PING:
        ESP_LOGI(TAG, "client %i pinged us with message of size %lu", num, (uint32_t)len);
        break;
    case WEBSOCKET_PONG:
        ESP_LOGI(TAG, "client %i responded to the ping", num);
        break;
    }
}

void send_density_data()
{
    char out[100];
    sprintf(out, "%.3f,%.3f,%.3f",
            weight_data.weight_in_air,
            weight_data.weight_in_water,
            weight_data.density);
    ws_server_send_text_all(out, strlen(out));
}

static void http_server(struct netconn *conn)
{
    const static char HTML_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";

    struct netbuf *inbuf;
    static char *buf;
    static uint16_t buflen;
    static err_t err;

    // default page
    extern const uint8_t root_html_start[] asm("_binary_index_html_start");
    extern const uint8_t root_html_end[] asm("_binary_index_html_end");
    const uint32_t root_html_len = root_html_end - root_html_start;

    netconn_set_recvtimeout(conn, 1000);
    ESP_LOGI(TAG, "reading from client...");
    err = netconn_recv(conn, &inbuf);
    ESP_LOGI(TAG, "read from client");
    if (err == ERR_OK)
    {
        netbuf_data(inbuf, (void **)&buf, &buflen);
        if (buf)
        {
            // default page
            if (strstr(buf, "GET / ") && !strstr(buf, "Upgrade: websocket"))
            {
                ESP_LOGI(TAG, "Sending /");
                netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER) - 1, NETCONN_NOCOPY);
                netconn_write(conn, root_html_start, root_html_len, NETCONN_NOCOPY);
                netconn_close(conn);
                netconn_delete(conn);
                netbuf_delete(inbuf);
            }
            // default page websocket
            else if (strstr(buf, "GET / ") && strstr(buf, "Upgrade: websocket"))
            {
                ESP_LOGI(TAG, "Requesting websocket on /");
                ws_server_add_client(conn, buf, buflen, "/", websocket_callback);
                netbuf_delete(inbuf);
            }
            else
            {
                ESP_LOGI(TAG, "Unknown request");
                netconn_close(conn);
                netconn_delete(conn);
                netbuf_delete(inbuf);
            }
        }
        else
        {
            ESP_LOGI(TAG, "Unknown request (empty?...)");
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }
    }
    else
    {
        ESP_LOGI(TAG, "error on read, closing connection");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
    }
}

static void server_task(void *pvParameters)
{
    struct netconn *conn, *newconn;
    static err_t err;
    client_queue = xQueueCreate(client_queue_size, sizeof(struct netconn *));

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    ESP_LOGI(TAG, "server listening");
    do
    {
        err = netconn_accept(conn, &newconn);
        ESP_LOGI(TAG, "new client");
        if (err == ERR_OK)
        {
            xQueueSendToBack(client_queue, &newconn, portMAX_DELAY);
        }
        vTaskDelay(10);
    } while (err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
    ESP_LOGE(TAG, "task ending, rebooting board");
    esp_restart();
}

static void server_handle_task(void *pvParameters)
{
    struct netconn *conn;
    ESP_LOGI(TAG, "task starting");
    for (;;)
    {
        xQueueReceive(client_queue, &conn, portMAX_DELAY);
        if (!conn)
            continue;
        http_server(conn);
        vTaskDelay(10);
    }
    vTaskDelete(NULL);
}

weight_data_t read_weight_data()
{
    return weight_data;
}

void reset_new_data_flag()
{
    weight_data.new_data = false;
}

void start_websocket_server()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(MDNS_HOST_NAME);

    connect_to_wifi();

    ws_server_start();
    xTaskCreatePinnedToCore(&server_task, "server_task", 3000, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(&server_handle_task, "server_handle_task", 4000, NULL, 3, NULL, 1);
}
