#include "discover.h"
#include "led.h"
#include "bloom.h"
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

bloom_t device_bloom;

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

  if (!bloom_is_in(device_bloom, &device.mac[0], 6)) 
  {
    if (discover_cb) (*discover_cb)(&device);
    bloom_add(device_bloom, &device.mac[0], 6);
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
  if (!bloom_create(&device_bloom, BLOOM_SIZE, BLOOM_HASH_VIARATIONS)) return false;
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
  os_memset(&recent_devices[0], 0, sizeof(recent_devices));
  if (!bloom_destroy(device_bloom)) return false;
  return true;
}

