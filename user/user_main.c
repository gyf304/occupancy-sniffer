#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "endian.h"
#include "led.h"
#include "config.h"
#include "discover.h"
#include "wifi_client.h"
#include "comm.h"
#include "debug.h"

static os_timer_t discover_timer;
/*
  see discover.c for sniffing related stuffs
*/

static uint8_t sniffer_mac[6];

static const wifi_client_info_t wifi_info =
{ WIFI_SSID, WIFI_PASSWORD, 5000 };

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
  COMM_BUFFER_SIZE,
  COMM_BUFFER_SIZE,
  .hxdt=&report_hxdt_info
};

static comm_state_t report_comm_state;

static void (*soft_reset)(void) = 0;

void after_init();
void discover_setup();
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
  dbg_printf("[INFO] DEBUG MODE\n");
  os_printf("[INFO] Compile Date & Time: %s %s\n", __DATE__, __TIME__);
  wifi_get_macaddr(0, &sniffer_mac[0]);
  os_printf("[INFO] Sniffer MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
              sniffer_mac[0], sniffer_mac[1], 
              sniffer_mac[2], sniffer_mac[3], 
              sniffer_mac[4], sniffer_mac[5]);
  led_blink(BLINK_POWER_ON);
  discover_setup();
}

void ICACHE_FLASH_ATTR
discover_setup() {
  // initialize discover;
  dbg_printf("[INFO] Discover Setup\n");
  discover_init(&device_cb);
  // setup timer for SNIFF_TIME
  os_timer_disarm(&discover_timer);
  os_timer_setfn(&discover_timer, (os_timer_func_t *)discover_done_cb, NULL);
  os_timer_arm(&discover_timer, SNIFF_TIME * 1000, 0);
  // initialize communication
  comm_create(&report_comm_state, report_comm_info, report_cb);
  // write string for discover mode, ver 1
  comm_write(report_comm_state, "disc1\0\0\0", 8);
  // write mac addr
  comm_write(report_comm_state, &sniffer_mac[0], 6);
  // write discovery time frame
  uint16_t time = htobe16((uint16_t)SNIFF_TIME);
  comm_write(report_comm_state, &time, 2);
  // start discovery
  discover_start(SNIFF_CHANNEL);
}

void ICACHE_FLASH_ATTR
device_cb(wifi_device_t* device) {
  dbg_printf("[INFO] Device: %02X:%02X:%02X:%02X:%02X:%02X,", 
            device->mac[0], device->mac[1], 
            device->mac[2], device->mac[3], 
            device->mac[4], device->mac[5]);
  dbg_printf(" RSSI %d,", device->rssi);
  dbg_printf(" CHN %d\n", device->channel);
  led_blink(BLINK_ACTIVITY);
  comm_write(report_comm_state, device, sizeof(wifi_device_t));
}

void ICACHE_FLASH_ATTR
discover_done_cb(void* info)
{
  discover_stop();
  led_blink(BLINK_REPORT);
  dbg_printf("[INFO] Discover Done\n");
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
  dbg_printf("[INFO] Report done, error code %d\n", err);
  comm_destroy(report_comm_state);
  wifi_client_disconnect();
}

void ICACHE_FLASH_ATTR
wifi_client_disconnected_cb()
{
  discover_setup();
}

// Helpers
void ICACHE_FLASH_ATTR
wifi_client_cb(uint32_t status)
{
  switch (status) {
    case WIFI_CLIENT_CONNECT_SUCCESS:
      dbg_printf("[INFO] WIFI CONNECTED\n");
      wifi_client_connected_cb(); 
      break;
    case WIFI_CLIENT_DISCONNECT_SUCCESS:
      dbg_printf("[INFO] WIFI DISCONNECTED\n");
      wifi_client_disconnected_cb();
      break;
    default:
      dbg_printf("[ERROR] Unexpected wifi callback: %d\n", status);
      soft_reset(); // when in doubt, restart.
      break;
  }
}
