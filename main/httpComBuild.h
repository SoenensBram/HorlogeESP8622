char 
REQUEST[8192],                                                              // allocated space for putting the request
*WebServer =           "192.168.7.40",                                   // ipaddress or name of the target server
*WebPort =             "8000",                                              // target port for posting the data
*Path =                "/stress/meting  ",                                  // Path that is added after the name/address and port to build the post link
JsonElement[][] = {{"Led1"},{"Led1Ambiant"},{"Led2"},{"Led2Ambiant"}},      // Identifier for the data that is read from teh senor
Get[][]= {{"GET "},{" http://"},{" HTTP/1.0\r\nHost:"},{" \r\nUser-Agent:"},{" esp-idf/1.0 esp8266\r\n\r\n"}},
Post[][]= {{"POST "},{" http://"},{" HTTP/1.0\r\nHost:"},{" \r\nUser-Agent:"},{" esp-idf/1.0 esp8266\r\n"}},
Json[][]= {{"Content-Length:"},{" \r\nContent-Type:"},{""},{" application/json\r\n\r\n{"}},
JsonFormating[][] = {{"\""},{"\":"},{",\""}},
JsonData[8192];  
uint16_t RequestSize = 8192; 