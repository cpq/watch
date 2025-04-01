# Binary watch

This is my hobby project - a binary watch. It is FOSS software and hardware.
The watch is based on STM32 L432 microcontroller. It sleeps until the
button is pressed - then it shows the current time via set of LEDs.


## Firmware

The firmware code is written using make and GCC. It is bare metal, and uses
CMSIS headers only.


## Hardware

The hardware was designed using EasyEDA, with the help of EEVBlog community.
Thank you all who helped me with hardware design review - you can see
the details in [this thread](https://www.eevblog.com/forum/microcontrollers/suggest-a-microcontroller-for-a-diy-pcb-watch/).

The PCB and assembly was carried out by JLCPCB.
