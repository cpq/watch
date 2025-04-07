#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIX 1
#define USART1 NULL

#define BIT(x) (1UL << (x))
#define CLRSET(reg, clear, set) ((reg) = ((reg) & ~(clear)) | (set))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8)

// No-op HAL API implementation for a device with GPIO and UART
#define hal_init()
#define clock_init()
#define ram_free() (long) 0
#define ram_used() (long) 0
#define stack_used() (long) 0
#define stack_fill()
#define spin(x) ((void) 0)
#define attach_external_irq(pin)

#define gpio_output(pin)
#define gpio_input(pin)
#define gpio_toggle(pin)

extern volatile uint64_t g_ticks;  // Milliseconds since boot

static inline void delay_ms(uint64_t milliseconds) {
  g_ticks += milliseconds;  // Simulate that wait time has expired
}

// Simulating pins
static bool g_pins[10][15];

static inline bool gpio_read(uint16_t pin) {
  return g_pins[PINBANK(pin)][PINNO(pin)];
}

static inline void gpio_write(uint16_t pin, bool val) {
  g_pins[PINBANK(pin)][PINNO(pin)] = val;
}

#define uart_init(uart, baud)
#define uart_read_ready(uart) 0
#define uart_write_byte(uart, ch)

static inline void uart_write_buf(void *uart, void *buf, size_t len) {
  (void) uart, (void) buf, (void) len;
}

static struct exti {
  volatile uint32_t PR1;
} g_exti;
struct exti *EXTI = &g_exti;

// t: expiration time, prd: period, now: current time. Return true if expired
static inline bool timer_expired(volatile uint64_t *t, uint64_t prd,
                                 uint64_t now) {
  if (now + prd < *t) *t = 0;                    // Time wrapped? Reset timer
  if (*t == 0) *t = now + prd;                   // Firt poll? Set expiration
  if (*t > now) return false;                    // Not expired yet, return
  *t = (now - *t) > prd ? now + prd : *t + prd;  // Next expiration time
  return true;                                   // Expired, return true
}
