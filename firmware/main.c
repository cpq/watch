// Copyright (c) 2025 Sergey Lyubka
// SPDX-License-Identifier: MIT

#include "hal.h"

static const uint16_t s_leds[] = {
    PIN('A', 3),  PIN('A', 4),  PIN('A', 5),  PIN('A', 6),  // red
    PIN('A', 11), PIN('A', 12), PIN('A', 15), PIN('B', 7),  // orange
    PIN('A', 7),  PIN('A', 8),  PIN('B', 0),  PIN('B', 1),  // green
    PIN('B', 3),  PIN('B', 4),  PIN('B', 5),  PIN('B', 6),  // blue
};
#define NUM_LEDS (sizeof(s_leds) / sizeof(s_leds[0]))
#define BTN_PIN PIN('A', 2)
#define BLINK_PERIOD_MS 500  // LED blinking period in millis
#define LOG_PERIOD_MS 1000   // Info log period in millis
#define LED_SHOW_DURATION_MS 2500
#define SECONDS_IN_DAY (24 * 60 * 60)

uint32_t SystemCoreClock;  // Required by CMSIS. Holds system core cock value
void SystemInit(void) {    // Called automatically by startup code
}

static volatile uint64_t s_ticks;  // Milliseconds since boot
void SysTick_Handler(void) {       // SyStick IRQ handler, triggered every 1ms
  s_ticks++;
}

static volatile uint64_t s_turn_off_time;
void EXTI2_IRQHandler(void) {
  uint8_t n = (uint8_t) (PINNO(BTN_PIN));
  EXTI->PR1 = BIT(n);  // Clear interrupt
  s_turn_off_time = s_ticks + LED_SHOW_DURATION_MS;
}

static void led_task(void) {
  static int on = 0;
  if (!on && s_turn_off_time > s_ticks) {
    // gpio_write(s_leds[0], true);
    on = 1;
  }
  if (on && s_turn_off_time < s_ticks) {
    // for (size_t i = 0; i < NUM_LEDS; i++) gpio_write(s_leds[i], false);
    //on = false;
  }

#if 0
  static uint64_t timer = 0;
  if (timer_expired(&timer, BLINK_PERIOD_MS, s_ticks)) {
    for (size_t i = 0; i < NUM_LEDS; i++) {
      // gpio_write(s_leds[i], !gpio_read(s_leds[i]));
    }
  }
#endif
}

static void log_task(void) {  // Print a log every LOG_PERIOD_MS
  static uint64_t timer = 0;
  if (timer_expired(&timer, LOG_PERIOD_MS, s_ticks)) {
    printf("tick: %5lu, CPU %lu MHz\n", (unsigned long) s_ticks,
           (unsigned long) (SystemCoreClock / 1000000));
  }
}

int main(void) {
  clock_init();
  uart_init(UART_DEBUG, 115200);

  // Initialise LEDs: set output mode, and turn them off
  for (size_t i = 0; i < NUM_LEDS; i++) {
    gpio_output(s_leds[i]);
    gpio_write(s_leds[i], 0);
  }

  // Initialise user button
  gpio_input(BTN_PIN);
  attach_external_irq(BTN_PIN);

  for (;;) {
    led_task();
    log_task();
  }

  return 0;
}
