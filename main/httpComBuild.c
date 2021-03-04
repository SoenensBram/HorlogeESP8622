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
#include "httpComBuild.h"

static void InitPost(char *httpPost, char *Request){
    for(uint16_t i = 0; i < sizeof(httpPost)/sizeof(httpPost[0]);i++){
        snprintf(REQUEST,sizeof(REQUEST)/sizeof(REQUEST[0]),httpPost[i]);
    }
}

static uint16_t ArrayBuilderJson(uint16_t sizeData, int32_t *Data, char *Array){
    uint16_t sizeDataArray = 0;
    snprintf(Array,sizeof(Array)/sizeof(Array[0]),"[");
    for(uint16_t i = 0; i < sizeData);i++){
        sizeDataArray = sizeDataArray + snprintf(Array,RequestSize,"%d",Data[i]);
    }
    snprintf(Array,sizeof(Array)/sizeof(Array[0]),"]");
}

static void BuildJson(char *Request){
    for(uint16_t i = 0; i < sizeData);i++){
        sprintf(Request,"%d,",Data[i]);
    }
}

static void InitArays(uint16_t sizeData, int32_t *Data){}

static void InitArays(uint16_t sizeData, int32_t *Data){
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
        sprintf(tempChar,"%u,",Data[i]);
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
