#include "wifi_device.h"

bool ICACHE_FLASH_ATTR
wifi_device_eq(wifi_device_t* d1, wifi_device_t* d2)
{
  for (uint8_t i = 0; i < 6; i++) {
    if (d1->mac[i] != d2->mac[i]) return false;
  }
  return true;
}