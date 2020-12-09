

//char stringdata[100];
//char *REQUEST ="POST http://192.168.4.30:8000/stress/meting HTTP/1.0\r\nHost:http://192.168.4.30\r\nUser-Agent:esp-idf/1.0esp32\r\nConnection:keep-alive\r\nAccept:*/*\r\nAccept-Encoding:gzip,deflate,br\r\nContent-Length:44\r\nContent-Type:application/json\r\n\r\n{\"PersonID\":2,\"curHartslag\":95,\"curSPO2\":75}";
//"GET  http://192.168.4.30:8000/stress/ HTTP/1.0\r\nHost: 192.168.4.30\r\nUser-Agent: esp-idf/1.0 esp8266\r\n\r\n";
//"POST http://192.168.4.30:256/device/e864877977f2b070210a80bd094eb857/playMedia HTTP/1.0\r\nHost: 94.110.140.22\r\nUser-Agent: esp-idf/1.0 esp32\r\nContent-Length: 68\r\nContent-Type: application/json\r\n\r\n[{\"mediaType\":\"MP3\",\"mediaUrl\":\"http://flandersrp.be/song2.mp3\"}]\r\n\r\n";
//QUEST ="POST http://192.168.4.30:8000/stress/meting HTTP/1.0\r\nHost:http://192.168.4.30\r\nUser-Agent:esp-idf/1.0esp32\r\nConnection:keep-alive\r\nAccept:*/*\r\nAccept-Encoding:gzip,deflate,br\r\nContent-Length:44\r\nContent-Type:application/json\r\n\r\n{\"PersonID\":2,\"curHartslag\":95,\"curSPO2\":75}",

char 
REQUEST[6144],
WebServer[] =           "192.168.004.030",
WebPort[] =             "8000",
Path[] =                "/stress/meting  ",
JsonElement1[] =        "PersonID",
JsonData1[] =           "1",
JsonElement2[] =        "SampleData",
Get[]  = "GET  http:// HTTP/1.0\r\nHost: \r\nUser-Agent: esp-idf/1.0 esp8266\r\n\r\n",
Post[] = "POST http:// HTTP/1.0\r\nHost: \r\nUser-Agent: esp-idf/1.0 esp8266\r\n",
Json[] = "Content-Length: \r\nContent-Type: application/json\r\n\r\n{\"\":,\"\":}",
JsonData[3584];
int
locationHttp[] = {
    12,             //locationlink
    31              //locationHost
},
locationJson[] = {
    17,             //locationLenghtJson
    55,             //locationElement1
    57,             //locationData1
    59,             //locationElement2
    61              //LocationData2
    },
endHttpPart = 0;
bool IsPost = 1;
uint16_t DataSampleSize = 512;
enum Sensor sensorData = Led1;
