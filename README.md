# STM32F446RE Bare-Metal Drivers

No HAL, no CMSIS — register-level bare-metal drivers for the Nucleo-F446RE board, built from the reference manual (RM0390) up.

## What's here

- **GPIO**: output (LD2 / PA5), input with edge-detected button (B1 / PC13)
- **UART (USART2)**: polling TX, interrupt-driven RX with echo — PA2/PA3, routed via ST-Link virtual COM port
- Hand-written `startup.s` (vector table, `Reset_Handler`, `.data`/`.bss` init) and `linker.ld`
- `practice.h`: peripheral register maps (`GPIO_TypeDef`, `RCC_TypeDef`, `USART_TypeDef`, NVIC) built by hand from RM0390 register maps, not copied from ST's CMSIS headers

## Build

```
make        # builds blink.elf / blink.bin
make flash  # flashes via OpenOCD
```

## Files

| File | Purpose |
|---|---|
| `main.c` | Application: GPIO drivers, UART drivers, ISR, main loop |
| `practice.h` | Hand-built register definitions |
| `startup.s` | Vector table, reset handler, data/bss init |
| `linker.ld` | Memory map, section placement |
| `Makefile` | Build + flash rules |
