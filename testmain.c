#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "user/config.h"

#define xstr(s) str(s)
#define str(s) #s

int main() {
  printf("SIZE: %lu\n", sizeof("\r\nContent-Length:       \r\n\r\n"));
  return 0;
}
