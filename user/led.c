#include "led.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
static os_timer_t led_timer;

static bool led_state = 0;
static uint8_t led_repeat = 0;
static uint32_t led_period = 0;

void ICACHE_FLASH_ATTR led_init() {
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  gpio_output_set(BIT2, 0, BIT2, 0);
}

void ICACHE_FLASH_ATTR led_on() {
  gpio_output_set(0, BIT2, BIT2, 0);
}

void ICACHE_FLASH_ATTR led_off() {
  gpio_output_set(BIT2, 0, BIT2, 0);
}

static void ICACHE_FLASH_ATTR blink_cb(void* arg)
{
  if (led_repeat == 0) {
    os_timer_disarm(&led_timer);
    led_off();
    return;
  }
  if (led_state) {
    led_off();
  } else {
    led_on();
    led_repeat--;
  }
  led_state = !led_state;
}

void ICACHE_FLASH_ATTR led_blink(uint32_t period, uint8_t repeat) {
  led_state = 0;
  led_repeat = repeat;
  led_period = period;
  os_timer_disarm(&led_timer);
  os_timer_setfn(&led_timer, (os_timer_func_t *)blink_cb, NULL);
  os_timer_arm(&led_timer, led_period, 1);
}

void ICACHE_FLASH_ATTR led_set(bool in) {
  if (in) {led_on(); } else {led_off();}
}