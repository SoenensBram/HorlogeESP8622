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

// Building of the request from the given char-arrays in the h-file. currently only array's as 2e element
static void InitArays(uint16_t sizeData, uint32_t *Data){
    uint32_t RequestSize = sizeof(REQUEST)/sizeof(REQUEST[0]);
    if(endHttpPart < 67){
        if(IsPost){
            snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,Post);
            endHttpPart = endHttpPart + locationHttp[0];
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebServer);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,"/");
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebPort);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,Path);
            snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,&Post[locationHttp[0]]);
            endHttpPart = endHttpPart + locationHttp[1]-locationHttp[0];
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebServer);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,&Post[locationHttp[1]]);
        }else{
            snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,Get);
            endHttpPart = endHttpPart + locationHttp[0];
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebServer);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,"/");
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebPort);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,Path);
            snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,&Get[locationHttp[0]]);
            endHttpPart = endHttpPart + locationHttp[1]-locationHttp[0];
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,WebServer);
            endHttpPart = endHttpPart + snprintf(&REQUEST[endHttpPart],RequestSize-endHttpPart,&Get[locationHttp[1]]);
        };
    };
    //Berekenen van de lengte van de json
    uint32_t jsonSize = sizeof(JsonData)/sizeof(JsonData[0]);
    uint16_t jsonLocation = snprintf(JsonData,jsonSize,"[");
    char tempChar[8];
    for(uint16_t i = 0; i < sizeData;i++){ //["ele1", "ele2", "ele3"]
        sprintf(tempChar,"%d,",Data[i]);
        jsonLocation = jsonLocation + snprintf(&JsonData[jsonLocation],jsonSize-jsonLocation,tempChar);
    };
    snprintf(&JsonData[jsonLocation-1],jsonSize-jsonLocation,"]");
    jsonSize = jsonLocation + (sizeof(JsonElement1)+sizeof(JsonElement2)+sizeof(JsonData1))/sizeof(char);
    uint16_t startJson = endHttpPart;
    snprintf(&REQUEST[startJson],RequestSize-startJson,Json);
    startJson = startJson + locationJson[0];
    sprintf(tempChar,"%u",jsonLocation);
    startJson = startJson + snprintf(&REQUEST[startJson],RequestSize-startJson,tempChar);
    snprintf(&REQUEST[startJson],RequestSize-startJson,&Json[locationJson[0]]);
    startJson = startJson + locationJson[1]-locationJson[0];
    startJson = startJson + snprintf(&REQUEST[startJson],RequestSize-startJson,JsonElement1);
    snprintf(&REQUEST[startJson],RequestSize-startJson,&Json[locationJson[1]]);
    startJson = startJson + locationJson[2]-locationJson[1];
    startJson = startJson + snprintf(&REQUEST[startJson],RequestSize-startJson,JsonData1);
    snprintf(&REQUEST[startJson],RequestSize-startJson,&Json[locationJson[2]]);
    startJson = startJson + locationJson[3]-locationJson[2];
    startJson = startJson + snprintf(&REQUEST[startJson],RequestSize-startJson,JsonElement2);
    snprintf(&REQUEST[startJson],RequestSize-startJson,&Json[locationJson[3]]);
    startJson = startJson + locationJson[4]-locationJson[3];
    startJson = startJson + snprintf(&REQUEST[startJson],RequestSize-startJson,JsonData);
    snprintf(&REQUEST[startJson],RequestSize-startJson,&Json[locationJson[4]]);
}

// getting the data from the spo2 sensor and putting the data in the request
static void BuildRequest(){
    uint32_t DataSamplesAFE[DataSampleSize];
    AfeGetDataArray(DataSampleSize, DataSamplesAFE, sensorData);
    //ESP_LOGI(TAG, "AFE array aquired");
    InitArays(DataSampleSize, DataSamplesAFE);
    ESP_LOGI("Request: \r\n",REQUEST);
}


// this is example code and needs to be put in it's own class
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

        BuildRequest(); //mod

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

//Here we need cleanup of all the touble shooting code
void app_main(){
    ESP_LOGI(TAG,"StartCode");
    ESP_ERROR_CHECK(Afe4404Init());
   /* Afe4404PowerUp();
    uint32_t bob =0;
    while(1){
        bob = AfeGetData(sensorData);
        bob = bob/200;
        ESP_LOGI("bob","$%u;",bob);
            uint8_t configData[3];
            configData[0]=(uint8_t)(Value[5] >>16);
            configData[1]=(uint8_t)(((Value[5] & 0x00FFFF) >>8));
            configData[2]=(uint8_t)(((Value[5] & 0x0000FF)));
        I2cMasterAfe4404Write(Address[5], configData, 3);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }*/
    //char bruh[10] = "bruh";
    //int bob = 30;
    //ESP_LOGI("Printing BRUH: ", bruh);
    //char tmepem[16];
    //sprintf(tmepem,"%u",bob);
    //ESP_LOGI("Printing BOB: ", tmepem);
    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(DataReadyInterupt);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(DataReadyInterupt, EspSpo2Data, (void *) DataReadyInterupt);


    //b while (1) {
    //b     if(DataReady)EspSpo2Data();
    //b     vTaskDelay(1 / portTICK_RATE_MS);
    //b }

    //while (1){
    //    vTaskDelay(10000 / portTICK_PERIOD_MS);
    //    EspSpo2Data();
    //}
    
  
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_post, "http_get_task", 40000, NULL, 5, NULL);


    //ESP_ERROR_CHECK(Afe4404PowerUp());


    //ESP_LOGI(TAG, "Enter While");
    //while(1){}
    //ESP_LOGI(TAG, "End of MainApp");
}
