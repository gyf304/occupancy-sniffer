#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "bloom.h"

struct bloom_header {
  uint32_t size;
  uint8_t  hash_viarations;
};

bool ICACHE_FLASH_ATTR
bloom_create(bloom_t* bloom_ptr, uint32_t size, uint8_t hash_viarations) {
  bloom_t b = os_malloc(size + sizeof(struct bloom_header));
  os_memset(b, 0, size + sizeof(struct bloom_header));
  b->size = size;
  b->hash_viarations = hash_viarations;
  *bloom_ptr = b;
  return true;
}

inline static void set(uint8_t* buf, uint32_t i) {
  const uint32_t byte_offset = i/8;
  const uint8_t bit_offset = i%8;
  buf[byte_offset] |= 1 << bit_offset;
}

inline static bool get(uint8_t* buf, uint32_t i) {
  const uint32_t byte_offset = i/8;
  const uint8_t bit_offset = i%8;
  return buf[byte_offset] & (1 << bit_offset);
}

const static uint32_t rnv_primes[16] = {486187739, 92821, 22375207, 18544973, 29226163, 30696949, 30709619, 17818487, 15762389, 31501619, 31623667, 32452843, 15486241, 15489053, 29684339, 31074107};
inline static uint32_t hash (uint8_t v, uint8_t* x, uint8_t elem_size) {
  const uint32_t prime1 = v % 16;
  const uint32_t prime2 = (v+1) % 16;
  const uint32_t prime3 = (v+2) % 16;
  uint32_t result = prime2 * prime3;
  uint8_t i;
  for (i = 0; i < elem_size; i++) {
    result = result ^ x[i];
    result = result * prime1;
  }
  return result;
}

bool ICACHE_FLASH_ATTR
bloom_add(bloom_t b, uint8_t* x, uint32_t elem_size) {
  uint8_t i;
  uint8_t* buf = ((uint8_t*)b) + sizeof(struct bloom_header);
  for (i = 0; i < b->hash_viarations; i++) {
    set(buf, hash(i, x, elem_size) % (b->size * 8));
  }
  return true;
}

bool ICACHE_FLASH_ATTR
bloom_is_in(bloom_t b, uint8_t* x, uint32_t elem_size) {
  uint8_t i;
  uint8_t* buf = ((uint8_t*)b) + sizeof(struct bloom_header);
  for (i = 0; i < b->hash_viarations; i++) {
    if (!get(buf, hash(i, x, elem_size) % (b->size * 8))) return false;
  }
  return true;
}

bool ICACHE_FLASH_ATTR
bloom_destroy(bloom_t b)
{
  os_free(b);
  return true;
}