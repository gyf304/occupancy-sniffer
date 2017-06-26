#include "discover.h"
#include "led.h"
#include "config.h"
#include "osapi.h"
#include "user_interface.h"


static uint8_t state = DISCOVER_STATE_OFF;
static uint8_t sniff_channel = 1;
static discover_device_cb_t* discover_cb = NULL;

struct cached_device {
  uint32_t t;
  uint32_t hit;
  wifi_device_t device;
};

// this is for dedup: a device can send out multiple pr frames at once
#define EVICT_TIME_US 100000
#define DISCOVER_RECENT_DEVICES 10
struct cached_device recent_devices[DISCOVER_RECENT_DEVICES];

static void ICACHE_FLASH_ATTR
sniff_cb(uint8_t* _buf, uint16_t len) 
{
  if (len < sizeof(frame_buffer_t)) return; // drop invalid frames.
  frame_buffer_t* buf = (frame_buffer_t*) _buf;
  if (! (  (buf->frame.type == 0b00 && buf->frame.subtype == 0x04)
       // || (buf->frame.type == 0b10 && buf->frame.to_ds && !buf->frame.from_ds)
        )
     ) return;
  // only interested in probe and sta->ap frames
  wifi_device_t device;
  memset(&device, 0, sizeof(wifi_device_t));
  // zero out current record. 
  device.rssi = buf->rx.rssi;
  device.channel = sniff_channel;
  os_memcpy(&device.mac[0], &buf->frame.addr2[0], 6);

  // os_printf("RAW: %02X:%02X:%02X:%02X:%02X:%02X,", 
  //           device.mac[0], device.mac[1], 
  //           device.mac[2], device.mac[3], 
  //           device.mac[4], device.mac[5]);
  // os_printf("%d,", device.rssi);
  // os_printf("%d\n", device.channel);

  uint32_t cur_time = system_get_time();
  // in any case, we need to evict ones that are too old.
  for (uint8_t i = 0; i < DISCOVER_RECENT_DEVICES; i++)
  {
    if (recent_devices[i].hit && cur_time - recent_devices[i].t > EVICT_TIME_US) 
    {
      if (discover_cb) (*discover_cb)(&recent_devices[i].device);
      recent_devices[i].hit = 0;
    }
  }

  bool evict = true;
  // find if the discovered device is recent, cache if recent
  for (uint8_t i = 0; i < DISCOVER_RECENT_DEVICES; i++)
  {
    // if blank spot found
    if (recent_devices[i].hit == 0)
    {
      recent_devices[i].hit = 1;
      recent_devices[i].t = cur_time;
      os_memcpy(&recent_devices[i].device, &device, sizeof(wifi_device_t));
      evict = false;
      break;
    }
    // otherwise if there's already such a device
    if (recent_devices[i].hit && wifi_device_eq(&recent_devices[i].device, &device))
    {
      // update rssi to an averaged value
      recent_devices[i].device.rssi = 
        ((int32_t)recent_devices[i].device.rssi * (int32_t)recent_devices[i].hit + (int32_t)device.rssi) / 
        ((int32_t)recent_devices[i].hit+1);
      recent_devices[i].hit++;
      recent_devices[i].t = cur_time;
      evict = false;
      break;
    }
  }
  // if we didn't cache anything before, we need to evict something, and then cache
  if (evict) {
    uint8_t evict_index = 0;
    uint32_t min_t = (uint32_t)-1;
    for (uint8_t i = 0; i < DISCOVER_RECENT_DEVICES; i++)
    {
      if (min_t > recent_devices[i].t)
      {
        min_t = recent_devices[i].t;
        evict_index = i;
      }
    }
    // now call cb for the evicted one
    if (discover_cb) (*discover_cb)(&recent_devices[evict_index].device);
    // now store our current one in there
    os_memcpy(&recent_devices[evict_index].device, &device, sizeof(wifi_device_t));
    recent_devices[evict_index].hit = 1;
    recent_devices[evict_index].t = cur_time;
  }
}

bool ICACHE_FLASH_ATTR
discover_init(discover_device_cb_t* cb) {
  discover_cb = cb;
  state = DISCOVER_STATE_STANDBY;
  os_memset(&recent_devices[0], 0, sizeof(recent_devices));
  return true;
}

bool ICACHE_FLASH_ATTR
discover_start(uint8_t channel) 
{
  if (state != DISCOVER_STATE_STANDBY) return false;
  // setup sniffing
  wifi_set_opmode(STATION_MODE);
  sniff_channel = channel;
  wifi_set_channel(channel);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniff_cb);
  // Set up promiscuous callback
  wifi_promiscuous_enable(1);
  state = DISCOVER_STATE_ACTIVE;
  return true;
}

bool ICACHE_FLASH_ATTR
discover_stop() {
  // stop sniffing around
  if (state != DISCOVER_STATE_ACTIVE) return false;
  wifi_promiscuous_enable(0);
  state = DISCOVER_STATE_STANDBY;
  // and evict all cache
  for (uint8_t i = 0; i < DISCOVER_RECENT_DEVICES; i++)
  {
    if (recent_devices[i].hit)
    {
      if (discover_cb) (*discover_cb)(&recent_devices[i].device);
    }
  }
  os_memset(&recent_devices[0], 0, sizeof(recent_devices));
  return true;
}

