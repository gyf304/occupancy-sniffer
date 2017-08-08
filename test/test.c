#include <stdio.h>
#include "xtea.h"

int main(){
  xtea_ctr_info_t info = {
    "testtesttesttest",
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  };
  uint8_t data[128];
  for (int i = 0; i < sizeof(data); i++) data[i] = 0;
  xtea_ctr(&info, (void*)&data[0], (void*)&data[0], 128);
  for (int i = 0; i < sizeof(data); i++) printf("%02X", data[i]);
  printf("\n");
  return 0;
}