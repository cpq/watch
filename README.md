#Binary watch

This is my hobby project - a binary watch. It is FOSS software and hardware.
The watch is based on STM32 L432KC microcontroller. It sleeps until the
button is pressed - then it shows the current time via set of LEDs:

The current time can be set up using triple button click for hours,
and quadruple button click for minutes.


## Hardware

The hardware was designed using EasyEDA, with the help from EEVBlog community.
Thank you all who helped me with the hardware design review - you can see
the details in [this thread](https://www.eevblog.com/forum/microcontrollers/suggest-a-microcontroller-for-a-diy-pcb-watch/).
EasyEDA sources for both schematic and PCB are in the [hardware](hardware/) folder.

PCB production and assembly was carried out by JLCPCB. This is the board
I've got (I don't know why they soldered male pin headers, but fine, they came
out handy for programming and testing. To be desoldered later):


## Firmware

The firmware code is written using make and GCC. It is bare metal, and uses
CMSIS headers only. It is also buildable for both STM32 and Mac/Linux,
for unit testing. The Mac/Linux build creates a unit test binary which makes
it easy to test the logic. The unit test mocks LEDs, simulates button clicks
by calling EXTI IRQ handler, and verifies that appropriate LEDs are lit.
