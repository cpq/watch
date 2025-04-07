// Copyright (c) 2025 Sergey Lyubka
// SPDX-License-Identifier: MIT

extern void setup(void);
extern void loop(void);

int main(void) {
  setup();
  for (;;) {
    loop();
  }
  return 0;
}
