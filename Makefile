# Makefile - STM32F446RE bare-metal project

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
INCLUDES = -IFreeRTOS/include -IFreeRTOS -I. -IFreeRTOS/portable/GCC/ARM_CM4F
CFLAGS  = $(CPU) $(INCLUDES) -Wall -g -O0 -ffreestanding 
LDFLAGS = -T linker.ld -nostartfiles

SRCS = main.c startup.s mem.c system_stubs.c \
       FreeRTOS/tasks.c \
       FreeRTOS/queue.c \
       FreeRTOS/list.c \
       FreeRTOS/timers.c \
       FreeRTOS/portable/GCC/ARM_CM4F/port.c \
       FreeRTOS/portable/MemMang/heap_4.c

TARGET = blink

all: $(TARGET).bin

$(TARGET).elf: $(SRCS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).elf
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg \
	  -c "program $(TARGET).elf verify reset exit"

clean:
	rm -f *.elf *.bin *.o

.PHONY: all flash clean