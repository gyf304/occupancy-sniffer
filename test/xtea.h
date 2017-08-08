#ifndef _XTEA_H
#define _XTEA_H

#ifndef __XTENSA__
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef __XTENSA__
#include "user_interface.h"
#endif

#define XTEA_BLOCK_SIZE 8
#define XTEA_KEY_SIZE 16

#define XTEA_SUCCESS 0
#define XTEA_BAD_PADDING 1
#define XTEA_ERROR 2

typedef struct xtea_ctr_info {
  const uint8_t* key;
  const uint8_t* iv;
} xtea_ctr_info_t;

/*
 *  Some comments on using cbc mac:
 *  1. Never use the same iv twice
 *  2. mac_key needs to be secret, same as key itself.
 *  3. mac_key should be different from the key used for encryption.
 *  4. mac_iv should be static, allowing customizable mac_iv can open window 
 *     to attacks.
 */

typedef struct xtea_cbc_mac_info {
  const uint8_t* key; // 128 bit mac key;
  const uint8_t* iv;  // 128 bit mac iv,  set to NULL for 0;
} xtea_cbc_mac_info_t;

uint32_t xtea_ctr(xtea_ctr_info_t* info, 
                  uint8_t* out, uint8_t* data, uint32_t len);

uint32_t xtea_cbc_mac(xtea_cbc_mac_info_t* info, 
                      uint8_t* out, uint8_t* data, uint32_t len);

#endif
