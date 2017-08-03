#ifndef _ENDIAN_H
#define _ENDIAN_H
#include "os_type.h"

static inline void endian_convert(void* out, const void* in, uint8_t size)
{
  uint8_t k = size / 2;
  for (uint8_t i = 0; i < k; i++) {
    ((char*)out)[i] = ((char*)in)[size-i-1];
  }
}

#endif