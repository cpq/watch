// Copyright (c) 2025 Sergey Lyubka
// SPDX-License-Identifier: MIT

#include "hal.h"

#define UART_DEBUG USART1          // Debug output UART channel
#define BTN_PIN PIN('A', 2)        // Button pin
#define LED_SHOW_DURATION_MS 2500  // How long LEDs stay on after button press
#define NEXT_PRESS_MS 500          // Time within next button press is expected

#define SECONDS_IN_DAY (24 * 60 * 60)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Watch states
static enum watch_state {
  STATE_SLEEP,      // Sleeping, drawing minimum energy possible
  STATE_SHOW_TIME,  // Showing current time - after single button press
  STATE_SET_HOURS,  // Setting hours - after a triple button press
  STATE_SET_MINS,   // Setting minutes - after a quad button press
} s_state = STATE_SLEEP;

// Timestamp is millis when the LEDs should be turned off and we sleep again
static uint64_t s_leds_turn_off_time;

// Time in a day in milliseconds at device boot
static uint64_t s_time_in_millis_at_boot;

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

static volatile uint64_t s_ticks;  // Milliseconds since boot
void SysTick_Handler(void) {       // SyStick IRQ handler, triggered every 1ms
  s_ticks++;
}

static void delay_ms(uint64_t milliseconds) {
  uint64_t expire = s_ticks + milliseconds;
  while (s_ticks < expire) spin(1);
}

// Set LEDs to a given state. There are 16 LEDs in 4 columns.
// Each column is: red = 1, orange = 2, green = 4, blue = 8
static void set_leds(uint16_t mask) {
  for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
    gpio_write(s_leds[i], mask & (1U << i));
  }
}

// Button press handler
// When button is pressed, we set the time in the future until which the
// press should be considered "active". The LED task will pick it and
// turn on LEDs
static volatile uint64_t s_next_press_timeout;
static volatile size_t s_press_count;

void EXTI2_IRQHandler(void) {
  uint8_t n = (uint8_t) (PINNO(BTN_PIN));
  EXTI->PR1 = BIT(n);  // Clear interrupt
  s_next_press_timeout = s_ticks + NEXT_PRESS_MS;
  s_press_count++;
}

static void blink_all(int num_times) {
  printf("blinking...\n");
  for (int i = 0; i < num_times; i++) {
    set_leds(0xffff);
    delay_ms(250);
    set_leds(0);
    delay_ms(250);
  }
  printf("blinking done\n");
}

static void btn_task(void) {
  uint32_t mh = 3600 * 1000;  // Milliseconds in 1 hour
  uint32_t mm = 60 * 1000;    // Milliseconds in 1 minute

  if (s_next_press_timeout > s_ticks) return;

  if (s_press_count > 0) {
    printf("tick: %5lu, heap used: %ld, stack used: %ld\n",
           (unsigned long) s_ticks, ram_used(), stack_used());
    printf("presses: %lu, state: %d\n", (unsigned long) s_press_count, s_state);
    s_leds_turn_off_time = s_ticks + LED_SHOW_DURATION_MS;
  }

  if (s_state == STATE_SLEEP && s_press_count == 1) {
    s_state = STATE_SHOW_TIME;
  } else if (s_state == STATE_SLEEP && s_press_count == 3) {
    blink_all(1);
    s_state = STATE_SET_HOURS;
  } else if (s_state == STATE_SLEEP && s_press_count == 4) {
    blink_all(2);
    s_state = STATE_SET_MINS;
  } else if (s_state == STATE_SET_HOURS) {
    s_time_in_millis_at_boot =
        s_press_count * mh + (s_time_in_millis_at_boot % mh);
    printf("Setting hours. Offset: %ld\n", (long) s_time_in_millis_at_boot);
    s_state = STATE_SLEEP;
  } else if (s_state == STATE_SET_MINS) {
    s_time_in_millis_at_boot =
        (s_time_in_millis_at_boot / mh) * mh + s_press_count * mm;
    printf("Setting minutes. Offset: %ld\n", (long) s_time_in_millis_at_boot);
    s_state = STATE_SLEEP;
  }
  s_press_count = 0;
}

static void led_task(void) {
  static bool on = false;
  if (on == false && s_state == STATE_SHOW_TIME &&
      s_leds_turn_off_time > s_ticks) {
    set_leds(0xffff);
    on = true;
  }
  if (on == true && s_leds_turn_off_time < s_ticks) {
    set_leds(0);
    s_state = STATE_SLEEP;
    on = false;
  }
}

// retargeting printf() to UART
int _write(int fd, char *ptr, int len) {
  if (fd == 1) uart_write_buf(UART_DEBUG, ptr, (size_t) len);
  return -1;
}

#if 0
static void log_task(void) {  // Print a log every LOG_PERIOD_MS
  static uint64_t timer = 0;
  if (timer_expired(&timer, LOG_PERIOD_MS, s_ticks)) {
    printf("tick: %5lu, heap used: %ld, stack used: %ld\n",
           (unsigned long) s_ticks, ram_used(), stack_used());
  }
}
#endif

int main(void) {
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

  for (;;) {
    // log_task();
    btn_task();
    led_task();
  }

  return 0;
}
