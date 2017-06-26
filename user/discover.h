#ifndef _DISCOVER_H
#define _DISCOVER_H

#include "user_interface.h"
#include "frame.h"
#include "wifi_device.h"

enum {
  DISCOVER_STATE_OFF = 0,
  DISCOVER_STATE_STANDBY,
  DISCOVER_STATE_ACTIVE
};

typedef void (discover_device_cb_t)(wifi_device_t*);

bool discover_init(discover_device_cb_t* cb);
bool discover_start(uint8_t channel);
bool discover_stop();

#endif
