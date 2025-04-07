// Copyright (c) 2025 Sergey Lyubka
// SPDX-License-Identifier: MIT

#include "hal.h"

#define UART_DEBUG USART1    // Debug output UART channel
#define BTN_PIN PIN('A', 2)  // Button pin
#define TIMEOUT_MS 2500      // How long LEDs stay on after button press
#define NEXT_PRESS_MS 500    // Time within next button press is expected
#define LOG_PERIOD_MS 1000   // For periodic debug messages

#define SECONDS_IN_DAY (24 * 60 * 60)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Watch states
static enum watch_state {
  STATE_SLEEP,        // Sleeping, drawing minimum energy possible
  STATE_SHOW_TIME,    // Showing current time - after single button press
  STATE_SET_HOURS,    // Setting hours - after a triple button press
  STATE_SET_MINUTES,  // Setting minutes - after a quad button press
} s_state = STATE_SLEEP;

// Timestamp is millis when the LEDs should be turned off and we sleep again
static uint64_t s_timeout;

// Time in a day in milliseconds at device boot
static uint64_t s_time_in_millis_at_boot;

// Milliseconds since boot
volatile uint64_t g_ticks;

// LEDs geometry on the PCB:                    Example:
// ----------------------------------
// blue    blue    blue    blue     8           - -     - o
// green   green   green   green    4           - -     o -
// orange  orange  orange  orange   2           - o     - -
// red     red     red     red      1           o o     - o
// ----------------------------------
// HOUR    HOUR    MINUTE  MINUTE               1 3  :  4 9
static const uint16_t s_leds[] = {
    PIN('A', 3),  PIN('A', 4),  PIN('A', 5),  PIN('A', 6),  // red
    PIN('A', 11), PIN('A', 12), PIN('A', 15), PIN('B', 7),  // orange
    PIN('A', 7),  PIN('A', 8),  PIN('B', 0),  PIN('B', 1),  // green
    PIN('B', 3),  PIN('B', 4),  PIN('B', 5),  PIN('B', 6),  // blue
};

uint32_t SystemCoreClock;  // Required by CMSIS. Holds system core cock value
void SystemInit(void) {    // Called automatically by startup code
  stack_fill();
}

void SysTick_Handler(void) {  // SyStick IRQ handler, triggered every 1ms
  g_ticks++;
}

static void set_state(int new_state) {
  s_state = new_state;
  printf("%s -> %d, tick %lu\n", __func__, s_state, (unsigned long) g_ticks);
}

// Set LEDs to a given state. There are 16 LEDs in 4 columns.
// Each column is: red = 1, orange = 2, green = 4, blue = 8
static void set_leds(uint16_t mask) {
  for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
    gpio_write(s_leds[i], mask & BIT(i));
  }
  // printf("%s -> %#04hx\n", __func__, mask);
}

static inline uint16_t time_to_led_mask(unsigned hours, unsigned minutes) {
  uint8_t a[] = {hours / 10, hours % 10, minutes / 10, minutes % 10};
  uint16_t mask = 0;
  for (int i = 0; i < 4; i++) {    // columns
    for (int j = 0; j < 4; j++) {  // rows
      uint16_t x = (a[i] & (1 << j)) ? 1 : 0;
      mask |= x << (j * 4 + i);
    }
  }
  return mask;
}

static inline unsigned hours(uint64_t milliseconds) {
  return ((milliseconds / 1000) % SECONDS_IN_DAY) / 3600;
}

static inline unsigned minutes(uint64_t milliseconds) {
  return ((milliseconds / 1000) % 3600) / 60;
}

// Button press handler
// When button is pressed, we set the time in the future until which the
// press should be considered "active". The LED task will pick it and
// turn on LEDs
static volatile uint64_t s_next_press_timeout;
static volatile int s_press_count;

void EXTI2_IRQHandler(void) {
  uint8_t n = (uint8_t) (PINNO(BTN_PIN));
  EXTI->PR1 = BIT(n);  // Clear interrupt
  s_next_press_timeout = g_ticks + NEXT_PRESS_MS;
  s_press_count++;
  printf("%s -> %d %lu\n", __func__, s_press_count, (unsigned long) g_ticks);
}

static void blink_all(int num_times) {
  for (int i = 0; i < num_times; i++) {
    set_leds(0xffff);
    delay_ms(200);
    set_leds(0);
    delay_ms(200);
  }
}

static void led_task(void) {
  static int saved_press_count;

  if (s_state == STATE_SLEEP) {
    if (s_press_count > 0 && g_ticks > s_next_press_timeout) {
      if (s_press_count == 1) {
        uint64_t now = g_ticks + s_time_in_millis_at_boot;
        set_state(STATE_SHOW_TIME);
        s_timeout = g_ticks + TIMEOUT_MS;
        set_leds(time_to_led_mask(hours(now), minutes(now)));
      } else if (s_press_count == 4) {
        blink_all(2);
        set_state(STATE_SET_MINUTES);
        s_timeout = g_ticks + TIMEOUT_MS;
      } else if (s_press_count == 3) {
        blink_all(1);
        set_state(STATE_SET_HOURS);
        s_timeout = g_ticks + TIMEOUT_MS;
      }
      s_press_count = 0;
    }
  } else if (s_state == STATE_SHOW_TIME) {
  } else if (s_state == STATE_SET_MINUTES) {
    // On click, increment and show the current minute and shift the timeout
    if (saved_press_count != s_press_count) {
      s_time_in_millis_at_boot =
          (s_time_in_millis_at_boot / 3600000) * 3600000 +
          s_press_count * 60000;
      set_leds(time_to_led_mask(0, s_press_count));
      s_timeout = g_ticks + TIMEOUT_MS;
      saved_press_count = s_press_count;
      printf("Setting minutes: %d. Offset: %ld, tick: %lu\n", s_press_count,
             (long) s_time_in_millis_at_boot, (unsigned long) g_ticks);
    }
  } else if (s_state == STATE_SET_HOURS) {
    // On click, increment and show the current hour and shift the timeout
    if (saved_press_count != s_press_count) {
      s_time_in_millis_at_boot =
          s_press_count * 3600000 + (s_time_in_millis_at_boot % (3600000));
      set_leds(time_to_led_mask(s_press_count, 0));
      s_timeout = g_ticks + TIMEOUT_MS;
      saved_press_count = s_press_count;
      printf("Setting hours: %d. Offset: %ld, tick %lu\n", s_press_count,
             (long) s_time_in_millis_at_boot, (unsigned long) g_ticks);
    }
  }

  // On timeout in any state, go back to sleep
  if (s_state != STATE_SLEEP && g_ticks > s_timeout) {
    set_leds(0);
    set_state(STATE_SLEEP);
    s_press_count = 0;
    saved_press_count = 0;
  }
}

// retargeting printf() to UART
int _write(int fd, char *ptr, int len) {
  if (fd == 1) uart_write_buf(UART_DEBUG, ptr, (size_t) len);
  return len;
}

static void log_task(void) {  // Print a log every LOG_PERIOD_MS
  static uint64_t timer = 0;
  if (timer_expired(&timer, LOG_PERIOD_MS, g_ticks)) {
    // printf("tick: %5lu, heap used: %ld, stack used: %ld\n",
    //        (unsigned long) g_ticks, ram_used(), stack_used());
  }
}

void setup() {
  clock_init();
  uart_init(UART_DEBUG, 115200);
  printf("CPU %lu MHz. Initialising firmware\n",
         (unsigned long) (SystemCoreClock / 1000000));

  // Initialise LEDs: set output mode, and turn them off
  for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
    gpio_output(s_leds[i]);
    gpio_write(s_leds[i], 0);
  }

  // Initialise user button
  gpio_input(BTN_PIN);
  attach_external_irq(BTN_PIN);
}

void loop(void) {
  log_task();
  led_task();
}
