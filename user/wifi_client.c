#include "wifi_client.h"

static os_timer_t wifi_client_timeout_timer;
static struct station_config conf;
static uint32_t state = WIFI_CLIENT_STATE_OFF;
static wifi_client_cb_t* wifi_client_cb = NULL;
static uint16_t timeout = 0;

static void ICACHE_FLASH_ATTR
wifi_client_timeout_cb(void* arg)
{
  switch (state) {
    case WIFI_CLIENT_STATE_CONNECTING:
      (*wifi_client_cb)(WIFI_CLIENT_CONNECT_TIMEOUT);
      break;
    case WIFI_CLIENT_STATE_DISCONNECTING:
      (*wifi_client_cb)(WIFI_CLIENT_DISCONNECT_TIMEOUT);
    default:
      break;
  }
}

static void ICACHE_FLASH_ATTR 
wifi_cb(System_Event_t *evt)
{
  switch (evt->event) {
    case EVENT_STAMODE_GOT_IP:
      os_timer_disarm(&wifi_client_timeout_timer);
      if (wifi_client_cb) (*wifi_client_cb)(WIFI_CLIENT_CONNECT_SUCCESS);
      state = WIFI_CLIENT_STATE_CONNECTED;
      break;

    case EVENT_STAMODE_DISCONNECTED:
      os_timer_disarm(&wifi_client_timeout_timer);
      if (wifi_client_cb) {
        switch (state) {
          case WIFI_CLIENT_STATE_CONNECTING:
            (*wifi_client_cb)(WIFI_CLIENT_CONNECT_ERROR);
            break;
          case WIFI_CLIENT_STATE_DISCONNECTING:
            (*wifi_client_cb)(WIFI_CLIENT_DISCONNECT_SUCCESS);
          case WIFI_CLIENT_STATE_DISCONNECTED:
            break;
          default:
            (*wifi_client_cb)(WIFI_CLIENT_DISCONNECTED);
            break;
        }
      };
      state = WIFI_CLIENT_STATE_DISCONNECTED;
      break;

    case EVENT_STAMODE_DHCP_TIMEOUT:
      os_timer_disarm(&wifi_client_timeout_timer);
      if (wifi_client_cb) (*wifi_client_cb)(WIFI_CLIENT_CONNECT_DHCP_TIMEOUT);
      state = WIFI_CLIENT_STATE_CONNECTED;
      break;

    default:
      break;
  }
}

bool ICACHE_FLASH_ATTR
wifi_client_init(const wifi_client_info_t* info, wifi_client_cb_t* cb) {
  if (state != WIFI_CLIENT_STATE_DISCONNECTED &&
      state != WIFI_CLIENT_STATE_OFF) return false;
  // set wifi info
  os_memset(&conf, 0, sizeof(struct station_config));
  conf.bssid_set = 0;
  os_memcpy(&conf.ssid, info->ssid, os_strlen(info->ssid));
  os_memcpy(&conf.password, info->password, os_strlen(info->password));

  timeout = info->timeout;

  wifi_client_cb = cb;

  state = WIFI_CLIENT_STATE_DISCONNECTED;
  
  return true;
}

bool ICACHE_FLASH_ATTR
wifi_client_connect() 
{
  // do not allow reconnections
  if (state != WIFI_CLIENT_STATE_DISCONNECTED) return false;
  
  // disable promiscuous mode, if previously enabled.
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);

  // setup callbacks.
  wifi_station_set_config_current(&conf);
  wifi_set_event_handler_cb(wifi_cb);

  // timer
  os_timer_disarm(&wifi_client_timeout_timer);
  os_timer_setfn(&wifi_client_timeout_timer, 
                 (os_timer_func_t *)wifi_client_timeout_cb, NULL);
  os_timer_arm(&wifi_client_timeout_timer, timeout, 0);
  
  state = WIFI_CLIENT_STATE_CONNECTING;
  
  return wifi_station_connect();
}

bool ICACHE_FLASH_ATTR
wifi_client_disconnect() {
  if (state != WIFI_CLIENT_STATE_CONNECTED) return false;
  state = WIFI_CLIENT_STATE_DISCONNECTING;
  return wifi_station_disconnect();
}
