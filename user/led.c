#include "led.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
static os_timer_t led_timer;

static uint16_t led_tick = 0;
static uint32_t led_period = 0;

void ICACHE_FLASH_ATTR 
led_init()
{
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  gpio_output_set(BIT2, 0, BIT2, 0);
}

void ICACHE_FLASH_ATTR 
led_on()
{
  gpio_output_set(0, BIT2, BIT2, 0);
}

void ICACHE_FLASH_ATTR 
led_off()
{
  gpio_output_set(BIT2, 0, BIT2, 0);
}

static void ICACHE_FLASH_ATTR 
blink_cb(void* arg)
{
  led_tick--;
  if (led_tick % 2) {
    led_on();
  } else {
    led_off();
  }
  if (led_tick == 0) os_timer_disarm(&led_timer);
}

void ICACHE_FLASH_ATTR 
led_blink(uint32_t period, uint8_t repeat)
{
  if (repeat == 0) return;
  if (led_tick > 0) return;
  led_tick = repeat * 2 - 1;
  os_timer_disarm(&led_timer);
  os_timer_setfn(&led_timer, (os_timer_func_t *)blink_cb, NULL);
  os_timer_arm(&led_timer, period, 1);
  led_on();
}

void ICACHE_FLASH_ATTR 
led_set(bool in)
{
  if (in) {led_on(); } else {led_off();}
}
