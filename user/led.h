#ifndef _LED_H
#define _LED_H
#include "os_type.h"

void led_init();
void led_on();
void led_off();
void led_set(bool in);
void ICACHE_FLASH_ATTR led_blink(uint32_t period, uint8_t repeat);

#endif