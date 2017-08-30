#ifndef _CONFIG_H
#define _CONFIG_H

#define WIFI_SSID "CMU"
#define WIFI_PASSWORD ""

//"testtesttesttest"
#define ENCRYPT_KEY  {0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74}

//"testtesttesttest"
#define AUTH_KEY     {0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74,\
                      0x74, 0x65, 0x73, 0x74}

#define AUTH_IV      {0x00, 0x00, 0x00, 0x00,\
                      0x00, 0x00, 0x00, 0x00}

#define BLOOM_SIZE 256
// in bytes
#define BLOOM_HASH_VIARATIONS 8

#define API_HOSTNAME "cmu-occupancy.herokuapp.com"
#define API_PORT 80

#define API_PATH "/rpc/hxdt"

#define COMM_BUFFER_SIZE 4096

#define SNIFF_TIME 30 // in seconds

#define BLINK_POWER_ON  200, 5
#define BLINK_ACTIVITY  20, 1
#define BLINK_REPORT    100, 3

#endif
