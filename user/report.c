#include "os_type.h"
#include "user_interface.h"
#include "mem.h"

#include "bloom.h"
#include "report.h"
#include "config.h"


struct report_header {
  bloom_t  bloom;
  uint32_t capacity;
  uint32_t size;
};

struct meta_info {
  uint8_t     mac[6];
  uint16_t    count;
};

static uint8_t* report_buf;
static uint32_t report_buf_size;

bool ICACHE_FLASH_ATTR
report_create(report_t* report_ptr, uint32_t capacity) {
  if (capacity < sizeof(struct meta_info)) return false;
  report_t report = os_malloc(capacity + sizeof(struct report_header));
  os_memset(report, 0, capacity + sizeof(struct report_header));
  if (!bloom_create(&report->bloom, BLOOM_SIZE, BLOOM_HASH_VIARATIONS)) {
    os_free(report);
    return false;
  };
  report->capacity = capacity;
  struct meta_info* mi = (struct meta_info*)(((uint8_t*)report) + sizeof(struct report_header));
  wifi_get_macaddr(0, &mi->mac[0]);
  mi->count = 0;
  return true;
}

bool ICACHE_FLASH_ATTR
report_add_device(report_t report, wifi_device_t* device) {
  struct meta_info* mi = (struct meta_info*)(((uint8_t*)report) + sizeof(struct report_header));
  wifi_device_t* devices = (wifi_device_t*) (((uint8_t*)mi)+sizeof(struct meta_info));
  if ( (mi->count+1) * sizeof(wifi_device_t) + sizeof(struct meta_info) >= report->capacity ) 
    return false; // we full
  if (bloom_is_in(report->bloom, &device->mac[0], 6)) return false; // already in
  #ifdef DEBUG
    os_printf("%02X:%02X:%02X:%02X:%02X:%02X,", 
              device->mac[0], device->mac[1], 
              device->mac[2], device->mac[3], 
              device->mac[4], device->mac[5]);
    os_printf("%d,", device->rssi);
    os_printf("%d\n", device->channel);
  #endif
  mi->count++;
  os_memcpy(&devices[mi->count], device, sizeof(wifi_device_t));
  bloom_add(report->bloom, &device->mac[0], 6);
  return true;
}

void ICACHE_FLASH_ATTR
report_destroy(report_t report)
{
  bloom_destroy(report->bloom);
  os_free(report);
}
