#include "heartbeat.h"

static void I2cTaskAfe4404Spo2(void *arg){

}

static void http_get_task(void *pvParameters){

}


void app_main()
{
    //ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());


    //ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_get_task, "netcontrol", 16384, NULL, 5, NULL);
    xTaskCreate(&I2cTaskAfe4404Spo2, "Spo2", 2048, NULL, 10, NULL);
}
