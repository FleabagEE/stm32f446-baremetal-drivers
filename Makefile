# Makefile — STM32F446RE bare-metal blink
#
# 用法：
#   make          → 編譯，產生 blink.elf 和 blink.bin
#   make flash    → 用 OpenOCD 燒進板子
#   make clean    → 清掉產生的檔案

# ---- 工具鏈 ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

# ---- CPU 旗標 ----
# Cortex-M4 + 硬體浮點；-mthumb 因為 Cortex-M 只跑 Thumb
CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ---- 編譯旗標 ----
# -nostdlib：不連標準庫（bare-metal，自己控制一切）
# -ffreestanding：告訴編譯器沒有 OS
# -T linker.ld：用我們自己的 linker script
CFLAGS  = $(CPU) -Wall -g -O0 -ffreestanding -nostdlib
LDFLAGS = -T linker.ld -nostdlib

SRCS = main.c startup.s
TARGET = blink

# ---- 編譯 ----
all: $(TARGET).bin

$(TARGET).elf: $(SRCS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# ---- 燒錄 ----
# 舊版 OpenOCD (0.10) 用 stlink-v2-1.cfg；新版用 stlink.cfg
# 你目前是 0.10，所以用 stlink-v2-1.cfg
flash: $(TARGET).elf
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg \
	  -c "program $(TARGET).elf verify reset exit"

clean:
	rm -f *.elf *.bin *.o

.PHONY: all flash clean
