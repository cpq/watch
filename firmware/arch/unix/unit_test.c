// Copyright (c) 2025 Sergey Lyubka
// SPDX-License-Identifier: MIT

#include "../../watch.c"

static uint16_t get_led_mask(void) {
  uint16_t mask = 0;
  for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
    uint16_t val = gpio_read(s_leds[i]) ? 1 : 0;
    mask |= val << i;
  }
  return mask;
}

int main(void) {
  setup();

  // Check all LEDs are off
  assert(get_led_mask() == 0);

  assert(time_to_led_mask(0, 0) == 0);
  assert(time_to_led_mask(1, 0) == 0x10);
  assert(time_to_led_mask(0, 1) == 0x1000);

  // Simulate single button press
  g_ticks = 3 * 60 * 1000;  // Shift to 3 minutes
  EXTI2_IRQHandler();
  g_ticks += NEXT_PRESS_MS + 1;
  loop();
  assert(get_led_mask() == 0x3000);

  // Turn off after timeout
  g_ticks += LED_SHOW_DURATION_MS + 1;
  loop();
  assert(get_led_mask() == 0);

  // Setting hours. Click 3 times, then wait for timeout
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  g_ticks += NEXT_PRESS_MS + 1;
  loop();
  assert(get_led_mask() == 0);
  assert(s_state == STATE_SET_HOURS);  // Check state
  g_ticks += LED_SHOW_DURATION_MS + 2;
  loop();
  assert(get_led_mask() == 0);
  assert(s_state == STATE_SLEEP);

  // Setting hours. Click 3 times
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  g_ticks += NEXT_PRESS_MS + 1;
  loop();
  assert(get_led_mask() == 0);
  assert(s_state == STATE_SET_HOURS);  // Check state
  EXTI2_IRQHandler();                  // Click once
  loop();
  assert(s_state == STATE_SET_HOURS);
  assert(get_led_mask() == 0x10);  // Check that we show 1 hour
  EXTI2_IRQHandler();              // Click once more
  loop();
  assert(get_led_mask() == 0x20);  // Check that we show 2 hours
  assert(s_state == STATE_SET_HOURS);

  // Wait until timeout - and set an hour
  g_ticks += NEXT_PRESS_MS + LED_SHOW_DURATION_MS + 2;
  loop();
  assert(s_time_in_millis_at_boot = 2 * 3600 * 1000);
  // printf("--> %#04x\n", get_led_mask());
  assert(get_led_mask() == 0);

  // Setting minutes. Click 4 times
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  EXTI2_IRQHandler();
  g_ticks += NEXT_PRESS_MS + 1;
  loop();
  assert(get_led_mask() == 0);
  assert(s_state == STATE_SET_MINUTES);  // Check state
  EXTI2_IRQHandler();                    // Click once
  loop();
  assert(get_led_mask() == 0x1000);  // Check that we show 1 hour
  EXTI2_IRQHandler();                // Click once more
  loop();
  assert(get_led_mask() == 0x2000);  // Check that we show 2 hours

  // Wait until timeout - and set an hour
  g_ticks += NEXT_PRESS_MS + LED_SHOW_DURATION_MS + 2;
  loop();
  assert(get_led_mask() == 0);

  return 0;
}
