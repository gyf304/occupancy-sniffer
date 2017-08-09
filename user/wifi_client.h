#ifndef _WIFI_CLIENT_H
#define _WIFI_CLIENT_H

#include "user_interface.h"
#include "osapi.h"

typedef void (wifi_client_cb_t)(uint32_t status);

typedef struct wifi_client_info {
  char* ssid;
  char* password;
  uint16_t timeout;
} wifi_client_info_t;

enum {
    WIFI_CLIENT_CONNECT_SUCCESS = 0, 
    WIFI_CLIENT_CONNECT_TIMEOUT,       // timeout
    WIFI_CLIENT_CONNECT_ERROR,         // error while connecting
    WIFI_CLIENT_CONNECT_DHCP_TIMEOUT,  // No ip address
    WIFI_CLIENT_DISCONNECTED,          // unexpected disconnection
    WIFI_CLIENT_DISCONNECT_SUCCESS,
    WIFI_CLIENT_DISCONNECT_TIMEOUT,
    WIFI_CLIENT_DISCONNECT_ERROR,
    WIFI_CLIENT_ERROR                  // generic error
};

enum {
    WIFI_CLIENT_STATE_CONNECTED = 0,
    WIFI_CLIENT_STATE_CONNECTING,
    WIFI_CLIENT_STATE_DISCONNECTING,
    WIFI_CLIENT_STATE_DISCONNECTED,
    WIFI_CLIENT_STATE_ERROR,
    WIFI_CLIENT_STATE_OFF
};

bool wifi_client_init(const wifi_client_info_t* info, wifi_client_cb_t* cb);
bool wifi_client_connect();
bool wifi_client_disconnect();

#endif