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

//class wificonnect{
//    public:
//    private:
        static const char *TAG = "example";
        char
        *Server = "192.168.0.100",
        *Url = "example.com:80/url",
        *Part1 = "GET ",
        *Part2 = "\r\n Host: ",
        *Part3 = "\r\n User-Agent: esp-idf/1.0 esp32\r\n \r\n";
        int Port = 80;
//}