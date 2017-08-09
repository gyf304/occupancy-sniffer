#include "xtea.h"
#ifndef __XTENSA__
#include <string.h>
#endif
#ifdef __XTENSA__
#include "mem.h"
#define memcpy(x) os_memcpy(x)
#endif

inline static void fast_endian_32(uint32_t* dst, uint32_t* src) {
  const uint32_t num = *src;
  *dst =  ((num>>24)&0xff)     | // move byte 3 to byte 0
          ((num<<8) &0xff0000) | // move byte 1 to byte 2
          ((num>>8) &0xff00)   | // move byte 2 to byte 1
          ((num<<24)&0xff000000); // byte 0 to byte 3
}

inline static void endian_32(void* dst, void* src) {
  uint32_t tmp;
  uint8_t* _tmp = (uint8_t*)(&tmp);
  uint8_t* _src = (uint8_t*)src;
  uint8_t* _dst = (uint8_t*)dst;
  _tmp[0] = _src[3]; 
  _tmp[1] = _src[2]; 
  _tmp[2] = _src[1]; 
  _tmp[3] = _src[0];
  _dst[0] = _tmp[0];
  _dst[1] = _tmp[1];
  _dst[2] = _tmp[2];
  _dst[3] = _tmp[3];
}

inline static void xor(void* dst, void* a, void* b, uint32_t len) {
  uint8_t* _dst = dst;
  uint8_t* _a = a;
  uint8_t* _b = b;
  for (uint32_t i = 0; i < len; i++) {
    _dst[i] = _a[i] ^ _b[i];
  }
}

static void encipher(unsigned int num_rounds, 
                     uint32_t o[2], uint32_t v[2], 
                     uint32_t const key[4]) {
  unsigned int i;
  uint32_t v0=v[0], v1=v[1], sum=0, delta=0x9E3779B9;
  for (i=0; i < num_rounds; i++) {
    v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    sum += delta;
    v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
  }
  o[0]=v0; o[1]=v1;
}

// verified with https://www.3amsystems.com/Crypto-Toolbox#xtea,ctr,Encrypt
uint32_t xtea_ctr(xtea_ctr_info_t* info, 
                  uint8_t* out, uint8_t* data, uint32_t len) {
  if (len % 8) return XTEA_BAD_PADDING;
  const uint32_t block_len = len / 8;
  uint32_t key_le[4]; // little endian version of key
  uint32_t ctr_le[2];
  uint32_t o[2];
  for (uint8_t i = 0; i < 4; i++) {
    endian_32(&key_le[i], &((uint32_t*)(info->key))[i]);
  }
  for (uint8_t i = 0; i < 2; i++) {
    endian_32(&ctr_le[i], &((uint32_t*)(info->iv))[i]);
  } // init v...;
  for (uint32_t i = 0; i < block_len; i++) {
    encipher(32, o, ctr_le, key_le);
    endian_32(&o[0], &o[0]);
    endian_32(&o[1], &o[1]);
    xor(&(((uint32_t*)out)[i*2]), &(o[0]), &(((uint32_t*)data)[i*2]), 8);
    // manual 64bit counter add
    ctr_le[1]++;
    if (ctr_le[1] == 0) ctr_le[0]++;
  }
  return XTEA_SUCCESS;
}

// verified with https://www.3amsystems.com/Crypto-Toolbox#xtea,cbc,Encrypt
// outputs a 64 bits / 8 bytes mac.
uint32_t xtea_cbc_mac(xtea_cbc_mac_info_t* info, 
                      uint8_t* out, uint8_t* data, uint32_t len) {
  if (len % 8) return XTEA_BAD_PADDING;
  const uint32_t block_len = len / 8;
  uint32_t key_le[4]; // little endian version of key
  uint32_t data_le[2];  // little endian input
  uint32_t state_le[2];
  for (uint8_t i = 0; i < 4; i++) {
    endian_32(&key_le[i], &((uint32_t*)(info->key))[i]);
  }
  if (info->iv == NULL) {
    state_le[0] = 0; state_le[1] = 0;
  } else {
    endian_32(&state_le[0], &((uint32_t*)(info->iv))[0]);
    endian_32(&state_le[1], &((uint32_t*)(info->iv))[1]);
  }
  // init complete...
  for (uint32_t i = 0; i < block_len; i++) {
    endian_32(&data_le[0], &((uint32_t*)data)[i*2]);
    endian_32(&data_le[1], &((uint32_t*)data)[i*2+1]);
    state_le[0] ^= data_le[0];
    state_le[1] ^= data_le[1];
    encipher(32, state_le, state_le, key_le); // in place encipher
  }
  endian_32(&((uint32_t*)out)[0], &state_le[0]);
  endian_32(&((uint32_t*)out)[1], &state_le[1]);
  return XTEA_SUCCESS;
}
