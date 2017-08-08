#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "led.h"
#include "config.h"
#include "discover.h"
#include "wifi_client.h"
#include "xtea.h"
#include "comm.h"

static os_timer_t discover_timer;
/*
  see sniff.c for sniffing related stuffs
*/

static const wifi_client_info_t wifi_info =
{ SSID, PASSWORD, 5000 };

static const hxdt_info_t report_hxdt_info = 
{
  API_HOSTNAME,
  API_PATH,
  API_PORT,
  ENCRYPT_KEY,
  AUTH_KEY,
  AUTH_IV
};

static const comm_info_t report_comm_info = 
{
  COMM_PROTOCOL_HXDT,
  .hxdt=&report_hxdt_info
};

static comm_state_t report_comm_state;


void after_init();
void start_discover();
void device_cb(wifi_device_t* device);
void report_cb(uint8_t err, uint8_t* buf, uint32_t len);
void discover_done_cb(void* info);
void wifi_client_cb(uint32_t status);
void wifi_client_connected_cb();
void wifi_client_disconnected_cb();

void ICACHE_FLASH_ATTR
user_init()
{
  // Initialize the GPIO subsystem.
  gpio_init();
  led_init();
  // Set baud rate for uart
  uart_div_modify(0, UART_CLK_FREQ/115200);

  wifi_station_set_auto_connect(0); // prevent wifi auto connection
  wifi_station_set_reconnect_policy(0); // do not reconnect

  // initialize wifi subsystem.
  wifi_client_init(&wifi_info, &wifi_client_cb); 

  //Execute some other stuff.
  system_init_done_cb(after_init);
}

void ICACHE_FLASH_ATTR
after_init()
{
  #ifdef DEBUG
    os_printf("DEBUG MODE\n");
  #endif
  led_blink(BLINK_POWER_ON);
  start_discover();
}

void ICACHE_FLASH_ATTR
start_discover() {
  // initialize discover;
  discover_init(&device_cb);
  // setup timer for SNIFF_TIME
  os_timer_disarm(&discover_timer);
  os_timer_setfn(&discover_timer, (os_timer_func_t *)discover_done_cb, NULL);
  os_timer_arm(&discover_timer, SNIFF_TIME, 0);
  // initialize communication
  comm_create(&report_comm_state, report_comm_info, COMM_BUFFER_SIZE, report_cb);
  // write string discover
  comm_write(report_comm_state, "DISCOVER", 8);
  comm_write(report_comm_state, "DISCOVER", 8);
  // write mac addr
  uint8_t mac[6];
  wifi_get_macaddr(0, &mac[0]);
  comm_write(report_comm_state, &mac[0], 6);
  // start discovery
  discover_start(1);
}

void ICACHE_FLASH_ATTR
device_cb(wifi_device_t* device) {
  os_printf("CB: %02X:%02X:%02X:%02X:%02X:%02X,", 
            device->mac[0], device->mac[1], 
            device->mac[2], device->mac[3], 
            device->mac[4], device->mac[5]);
  os_printf("%d,", device->rssi);
  os_printf("%d\n", device->channel);
  //report_add_device(report, device);
  comm_write(report_comm_state, device, sizeof(wifi_device_t));
}

void ICACHE_FLASH_ATTR
discover_done_cb(void* info)
{
  discover_stop();
  #ifdef DEBUG
    os_printf("Discover Done\n");
  #endif
  // connect to wifi
  wifi_client_connect();
}

void ICACHE_FLASH_ATTR
wifi_client_connected_cb()
{
  comm_send(report_comm_state); // fire
}

void ICACHE_FLASH_ATTR
report_cb(uint8_t err, uint8_t* buf, uint32_t len)
{
  os_printf("report_done\n");
  comm_destroy(report_comm_state);
  wifi_client_disconnect();
}

void ICACHE_FLASH_ATTR
wifi_client_disconnected_cb()
{
  start_discover();
}

// Helpers
void ICACHE_FLASH_ATTR
wifi_client_cb(uint32_t status)
{
  switch (status) {
    case WIFI_CLIENT_CONNECT_SUCCESS:
      #ifdef DEBUG
      os_printf("[OK] WIFI CONNECTED\n");
      #endif
      wifi_client_connected_cb(); 
      break;
    case WIFI_CLIENT_DISCONNECT_SUCCESS:
      #ifdef DEBUG
      os_printf("[OK] WIFI DISCONNECTED\n");
      #endif
      wifi_client_disconnected_cb();
      break;
    default:
      #ifdef DEBUG
      os_printf("[ERROR] Unexpected wifi callback: %d\n", status);
      #endif
      system_restart(); // when in doubt, restart.
      break;
  }
}

