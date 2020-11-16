#include "wificonnect.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <netdb.h>
#include <sys/socket.h>
#include "esp_http_client.h"

// only need to call 1 time
static esp_err_t PostInit(){
    //concat request
    strcat(Part1, Url);
    strcat(Part1, Part2);
    strcat(Part1, Server);
    strcat(Part1, Part3);
    char *Post = Part1;
    return ESP_OK;
}

static esp_err_t PostHttp(char *data, int dataLength){
    esp_http_client_config_t config;
    config.url = Url;
    config.host = Server;
    config.port = Port;
    esp_http_client_handle_t cleint = esp_http_client_init(&config);
    esp_http_client_set_method(cleint,HTTP_METHOD_POST);
    esp_http_client_set_post_field(cleint,*data,dataLength);
    esp_http_client_perform(cleint);
    return ESP_OK;
}