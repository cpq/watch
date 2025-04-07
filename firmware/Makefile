CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion -Wformat-truncation -fno-common
CFLAGS += -g3 -O0 -ffunction-sections -fdata-sections -Wno-shadow
CFLAGS += -Iarch/stm32 -Iinclude -Icmsis_core/CMSIS/Core/Include -Icmsis_l4/Include
CFLAGS += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS ?= -Tarch/stm32/link.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map
SOURCES = main.c watch.c arch/stm32/syscalls.c
SOURCES += cmsis_l4/Source/Templates/gcc/startup_stm32l432xx.s # ST startup file. Compiler-dependent!

build: firmware.bin

firmware.elf: cmsis_core cmsis_l4  Makefile arch/stm32/hal.h $(SOURCES) 
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(CFLAGS_EXTRA) $(LDFLAGS) -o $@

firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary $< $@

flash: firmware.bin
	STM32_Programmer_CLI -c port=/dev/cu.usbserial-0001 -w $< 0x8000000

monitor:
	esputil -p /dev/cu.usbserial-0001 monitor

cmsis_core:
	git clone -q -c advice.detachedHead=false  --depth 1 -b 5.9.0 https://github.com/ARM-software/CMSIS_5 $@

cmsis_l4:
	git clone -q -c advice.detachedHead=false  --depth 1 -b v1.7.2 https://github.com/STMicroelectronics/cmsis_device_l4 $@

test:
	$(CC) -W -Wall -Wextra -Iarch/unix arch/unix/unit_test.c -o firmware.test && ./firmware.test

clean:
	rm -rf firmware.* cmsis_*
