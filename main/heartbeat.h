

//char stringdata[100];
//char *REQUEST ="POST http://192.168.4.30:8000/stress/meting HTTP/1.0\r\nHost:http://192.168.4.30\r\nUser-Agent:esp-idf/1.0esp32\r\nConnection:keep-alive\r\nAccept:*/*\r\nAccept-Encoding:gzip,deflate,br\r\nContent-Length:44\r\nContent-Type:application/json\r\n\r\n{\"PersonID\":2,\"curHartslag\":95,\"curSPO2\":75}";
//"GET  http://192.168.4.30:8000/stress/ HTTP/1.0\r\nHost: 192.168.4.30\r\nUser-Agent: esp-idf/1.0 esp8266\r\n\r\n";
//"POST http://192.168.4.30:256/device/e864877977f2b070210a80bd094eb857/playMedia HTTP/1.0\r\nHost: 94.110.140.22\r\nUser-Agent: esp-idf/1.0 esp32\r\nContent-Length: 68\r\nContent-Type: application/json\r\n\r\n[{\"mediaType\":\"MP3\",\"mediaUrl\":\"http://flandersrp.be/song2.mp3\"}]\r\n\r\n";
//QUEST ="POST http://192.168.4.30:8000/stress/meting HTTP/1.0\r\nHost:http://192.168.4.30\r\nUser-Agent:esp-idf/1.0esp32\r\nConnection:keep-alive\r\nAccept:*/*\r\nAccept-Encoding:gzip,deflate,br\r\nContent-Length:44\r\nContent-Type:application/json\r\n\r\n{\"PersonID\":2,\"curHartslag\":95,\"curSPO2\":75}",

char 
REQUEST[6400],                                                              // allocated space for putting the request
*WebServer =           "192.168.004.030",                                   // ipaddress or name of the target server
*WebPort =             "8000",                                              // target port for posting the data
*Path =                "/stress/meting  ",                                  // Path that is added after the name/address and port to build the post link
*JsonElement1 =        "PersonID",                                          // Identifier for the current watch
*JsonData1 =           "1",                                                 // Value of Identifier for the current watch
*JsonElement2 =        "SampleData",                                        // Identifier for the data that is read from teh senor
Get[]  = "GET  http:// HTTP/1.0\r\nHost: \r\nUser-Agent: esp-idf/1.0 esp8266\r\n\r\n",      //static parts to build a get request
Post[] = "POST http:// HTTP/1.0\r\nHost: \r\nUser-Agent: esp-idf/1.0 esp8266\r\n",          //static parts to build a post request
Json[] = "Content-Length: \r\nContent-Type: application/json\r\n\r\n{\"\":,\"\":}",         //static parts of a json to add at hte end of a request
JsonData[6144];                                                             // allocated space for putting the json data
int
locationHttp[] = {                                  //Locations in the static array for building (http request)
    12,             //locationlink
    29              //locationHost
},
locationJson[] = {                                  //Locations in the static array for building (json)
    16,             //locationLenghtJson
    54,             //locationElement1
    56,             //locationData1
    58,             //locationElement2
    60              //LocationData2
    };
uint32_t endHttpPart = 0;                           //position tracker for tracking the position in het request array of where the http request ends and the json part starts
bool IsPost = 1;                                    //Are we building a post(=1) or a get(=0)
uint16_t DataSampleSize = 512;                      //amount of datasamples that needs to be sampled
enum Sensor sensorData = Led1;                      //The chosen sensor where the data is comming from. {Led2, aLed2, Led1, aLed1, diffLed2, diffLed1, AvgDiffLed2, AvgDiffLed1}
