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

static comm_state_t report_comm_state;
static const comm_info_t report_comm_info = 
  {COMM_PROTOCOL_HXDT, 
    {.hxdt = 
      {
        API_HOSTNAME, 
        API_PATH, 
        API_PORT, 
        (uint8_t*)ENCRYPT_KEY, 
        (uint8_t*)AUTH_KEY, 
        (uint8_t[])AUTH_IV
      }
    }
  };

void ICACHE_FLASH_ATTR
device_cb(wifi_device_t* device) {
  os_printf("CB: %02X:%02X:%02X:%02X:%02X:%02X,", 
            device->mac[0], device->mac[1], 
            device->mac[2], device->mac[3], 
            device->mac[4], device->mac[5]);
  os_printf("%d,", device->rssi);
  os_printf("%d\n", device->channel);
  //report_add_device(report, device);
}

void ICACHE_FLASH_ATTR
discover_done_cb(void* info)
{
  discover_stop();
  #ifdef DEBUG
    os_printf("Discover Done\n");
  #endif

  uint8_t ctr_key [16] = ENCRYPT_KEY;
  uint8_t ctr_iv  [16];
  uint8_t auth_key[16] = AUTH_KEY;
  uint8_t auth_iv [16] = AUTH_IV;

  os_get_random(&ctr_iv[0], 16);
  xtea_ctr_info_t ctr_info;
  xtea_cbc_mac_info_t mac_info;
  ctr_info.key = ctr_key;
  ctr_info.iv  = ctr_iv;

  mac_info.key = auth_key;
  mac_info.iv  = auth_iv;

  uint8_t mac[8];

  //xtea_ctr(&ctr_info, &report_buf[0], &report_buf[0], REPORT_SIZE);
  //xtea_cbc_mac(&mac_info, &mac[0], &report_buf[0], REPORT_SIZE);
  // os_printf("h\n");
  // for (uint32_t i = 0; i < TCP_DATA_SIZE; i++) {
  //   os_printf("%02x", tcp_buffer[i]);
  // }
  // os_printf("\n");
}

void ICACHE_FLASH_ATTR
mode_discover() {
  discover_init(&device_cb);
  os_timer_disarm(&discover_timer);
  os_timer_setfn(&discover_timer, (os_timer_func_t *)discover_done_cb, NULL);
  os_timer_arm(&discover_timer, SNIFF_TIME, 0);
  discover_start(1);
}

void ICACHE_FLASH_ATTR
wifi_client_cb(uint32_t status)
{
  switch (status) {
    case WIFI_CLIENT_CONNECT_SUCCESS:
      #ifdef DEBUG
      os_printf("[OK] WIFI CONNECTED\n");
      #endif
      break;
    case WIFI_CLIENT_DISCONNECT_SUCCESS:
      #ifdef DEBUG
      os_printf("[OK] WIFI DISCONNECTED\n");
      #endif
      break;
    default:
      #ifdef DEBUG
      os_printf("[ERROR] Unexpected wifi callback: %d\n", status);
      #endif
      system_restart(); // when in doubt, restart.
      break;
  }
}

void ICACHE_FLASH_ATTR
stack_test(uint32_t a, uint32_t b, uint32_t c, uint64_t d)
{
  os_printf("a, b, c, d, %p, %p, %p, %p\n", &a, &b, &c, &d);
}


void ICACHE_FLASH_ATTR
after_init()
{
  #ifdef DEBUG
    os_printf("DEBUG MODE\n");
  #endif
  led_blink(BLINK_POWER_ON);
  comm_create(&report_comm_state, &report_comm_info, COMM_BUFFER_SIZE);
  //comm_writef(report_comm_state, "i", 32);
  //comm_destroy(report_comm_state);
  stack_test(0,0,0,0);
  mode_discover();
}

//Init function 
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
  wifi_client_info_t cinfo;
  cinfo.ssid = (uint8_t*)SSID;
  cinfo.ssid_len = sizeof(SSID) - 1;
  cinfo.password = (uint8_t*)PASSWORD;
  cinfo.password_len = sizeof(PASSWORD) - 1;
  cinfo.timeout = 5000;
  wifi_client_init(&cinfo, &wifi_client_cb); 

  //Execute some other stuff.
  system_init_done_cb(after_init);
}
