#include <string.h>
#include <stdio.h>

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
#include "afe4404.c"
#include "heartbeat.h"



static const char *TAG = "ProjectHeartBeats";

static void InitArays(){
    if(!endHttpPart == 0){
        if(IsPost){
            strcpy(REQUEST[endHttpPart],Post);
            endHttpPart =+ locationHttp[0];
            strcpy(REQUEST[endHttpPart],WebServer);
            endHttpPart =+ (sizeof(WebServer)/sizeof(WebServer[0])); 
            strcpy(REQUEST[endHttpPart],"/");
            endHttpPart =+ 1; 
            strcpy(REQUEST[endHttpPart],WebPort);
            endHttpPart =+ (sizeof(WebPort)/sizeof(WebPort[0]));
            strcpy(REQUEST[endHttpPart],Path);
            endHttpPart =+ (sizeof(Path)/sizeof(Path[0]));
            strcpy(REQUEST[endHttpPart],Post[locationHttp[0]]);
            endHttpPart =+ locationHttp[1]-locationHttp[0];
            strcpy(REQUEST[endHttpPart],WebServer);
            endHttpPart =+ (sizeof(WebServer)/sizeof(WebServer[0])); 
            strcpy(REQUEST[endHttpPart],Post[locationHttp[1]]);
        }else{
            strcpy(REQUEST[endHttpPart],Get);
            endHttpPart =+ locationHttp[0];
            strcpy(REQUEST[endHttpPart],WebServer);
            endHttpPart =+ (sizeof(WebServer)/sizeof(WebServer[0])); 
            strcpy(REQUEST[endHttpPart],"/");
            endHttpPart =+ 1; 
            strcpy(REQUEST[endHttpPart],WebPort);
            endHttpPart =+ (sizeof(WebPort)/sizeof(WebPort[0]));
            strcpy(REQUEST[endHttpPart],Path);
            endHttpPart =+ (sizeof(Path)/sizeof(Path[0]));
            strcpy(REQUEST[endHttpPart],Get[locationHttp[0]]);
            endHttpPart =+ locationHttp[1]-locationHttp[0];
            strcpy(REQUEST[endHttpPart],WebServer);
            endHttpPart =+ (sizeof(WebServer)/sizeof(WebServer[0])); 
            strcpy(REQUEST[endHttpPart],Get[locationHttp[1]]);
        };
    }
    uint8_t startJson = endHttpPart;
    for(uint8_t i = 0; i<(sizeof(locationHttp)/sizeof(locationHttp[0]));i++){
        strcpy(REQUEST[i],Json);

    };
    
        
}

static void http_post(void *pvParameters){
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    unsigned int k = 0;
    while(k<1){

        int err = getaddrinfo(WebServer, WebPort, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

            Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        ESP_LOGI(TAG, REQUEST);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        else{
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "... socket send success");
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);
        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d Recieved=%s\r\n", r, errno,recv_buf);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Done!");
        //k = 10;
    }
}



void app_main()
{
    InitArays();
    ESP_ERROR_CHECK(Afe4404PowerUp());
    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(DataReadyInterupt);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(DataReadyInterupt, EspSpo2Data, (void *) DataReadyInterupt);


    //while (1) {
    //    if(DataReady)EspSpo2Data();
    //    vTaskDelay(1 / portTICK_RATE_MS);
    //}

    //while (1){
    //    vTaskDelay(10000 / portTICK_PERIOD_MS);
    //    EspSpo2Data();
    //}
    
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_post, "http_get_task", 32000, NULL, 5, NULL);

    //ESP_ERROR_CHECK(Afe4404PowerUp());


    //ESP_LOGI(TAG, "Enter While");
    //while(1){}
    //ESP_LOGI(TAG, "End of MainApp");
}
