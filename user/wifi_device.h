#ifndef _WIFI_DEVICE_H
#define _WIFI_DEVICE_H
#include "os_type.h"

typedef struct wifi_device {
  uint8_t mac[6];
  int8_t rssi;
  int8_t channel;
} wifi_device_t;

bool wifi_device_eq(wifi_device_t* d1, wifi_device_t* d2);

#endif