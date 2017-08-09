#ifndef _ENDIAN_H
#define _ENDIAN_H
#include "os_type.h"

static inline uint32_t htobe32(uint32_t num)
{
  return  ((num>>24)&0xff)     | // move byte 3 to byte 0
          ((num<<8) &0xff0000) | // move byte 1 to byte 2
          ((num>>8) &0xff00)   | // move byte 2 to byte 1
          ((num<<24)&0xff000000); // byte 0 to byte 3
}

static inline uint16_t htobe16(uint32_t num)
{
  return  ((num<<8) &0xff00) | // move byte 1 to byte 2
          ((num>>8) &0x00ff)   ; // move byte 2 to byte 1
}


#endif