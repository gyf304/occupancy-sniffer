#ifndef _CONFIG_H
#define _CONFIG_H

#define SSID "5875Darlington"
#define PASSWORD "ctoctoctocto"

#define ENCRYPT_KEY "testtesttesttest"
#define AUTH_KEY "testtesttesttest"

#define AUTH_IV {0x00,0x00,0x00,0x00,\
                0x00,0x00,0x00,0x00}

#define CHANNEL_DWELL_TIME 0
// in ms, 0 for no channel switching

#define BLOOM_SIZE 256
// in bytes
#define BLOOM_HASH_VIARATIONS 8

#define API_HOSTNAME "192.168.8.191"
#define API_PORT 5000
#define API_PATH "/test"

#define REPORT_SIZE 1024
#define COMM_BUFFER_SIZE 4096

#define XSTR(s) STR(s)
#define STR(s) #s

#define HTTP_HEADER_SIZE (sizeof(HTTP_HEADER) - 1)
#define TCP_DATA_SIZE (HTTP_HEADER_SIZE + CONTENT_LENGTH)

#define SNIFF_CHANNELS {1, 6, 11};
#define SNIFF_TIME 10000
// in ms

#define BLINK_POWER_ON 200, 5
#define BLINK_ACTIVITY 20, 1

#endif
