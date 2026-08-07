# Debug Log: UART RX Data Overrun

## Symptom

Python test framework (`test_uart.py`, `pyserial`) sent a multi-byte string to the board and compared the echoed response:

```
PASS: single byte     (sent b'A', got b'A')
FAIL: multi byte, sent b'Hello', got b'o'
```

Sending 5 bytes (`b'Hello'`) only echoed back 1 byte — and it was the *last* byte sent, not the first.

## Investigation

The RX path (Week 3, Day 4-5) used interrupt-driven receive with a single shared variable:

```c
volatile uint8_t rx_byte;
volatile int rx_flag = 0;

void USART2_IRQHandler(void) {
    rx_byte = USART2->DR;   /* reading DR auto-clears RXNE */
    rx_flag = 1;
}
```

The main loop only checked `rx_flag` once per iteration, and each iteration also called `delay(1000000)` (blocking busy-wait, used for LED/button debounce):

```c
while (1) {
    /* ...button/LED logic... */
    delay(1000000);
    /* ...check rx_flag... */
}
```

## Root Cause

`rx_byte` is a single variable, not a buffer — every new interrupt overwrites whatever was there before it's read.

At 115200 baud, one byte (start bit + 8 data bits + stop bit) takes roughly:
```
10 bits / 115200 bps ≈ 87 microseconds
```

`delay(1000000)` (a busy-wait `nop` loop) takes several *milliseconds* — orders of magnitude longer than 87 µs. So while `main()` is stuck in `delay()`, all 5 bytes of "Hello" arrive and trigger the ISR back-to-back, each one silently overwriting the last:

```
ISR('H') → rx_byte='H'
ISR('e') → rx_byte='e'   (H lost, never read)
ISR('l') → rx_byte='l'   (e lost)
ISR('l') → rx_byte='l'
ISR('o') → rx_byte='o'   (only this one survives)
```

By the time `main()` finally checks `rx_flag`, only the last byte (`'o'`) remains. This is a classic **buffer overrun** caused by a single-slot "buffer" with no backpressure or storage for unread data.

## Fix

Replaced the single variable with a 32-byte ring (circular) buffer, using `head`/`tail` indices:

```c
#define RX_BUF_SIZE 32
volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_head = 0;   /* ISR writes here */
volatile int rx_tail = 0;   /* main() reads here */

void USART2_IRQHandler(void) {
    uint8_t byte = USART2->DR;
    int next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) {      /* only write if not full */
        rx_buf[rx_head] = byte;
        rx_head = next_head;
    }
}

int rx_available(void) { return rx_head != rx_tail; }

uint8_t rx_pop(void) {
    uint8_t byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return byte;
}
```

Main loop now drains the buffer completely on every iteration instead of checking a single flag:

```c
while (rx_available()) {
    uart_write_char(rx_pop());
}
```

`head == tail` means empty; `next_head == tail` means full (one slot is sacrificed to distinguish full from empty without an extra counter). The ISR now *drops* a byte only if the buffer is genuinely full — 32 bytes is enough headroom for the 87 µs/byte vs. millisecond-scale main loop in this project.

## Verification

Re-ran the same Python test after flashing the fix:

```
PASS: single byte
PASS: multi byte
```

Both cases pass; no bytes dropped.

## Takeaway

`rx_byte`/`rx_flag` is what a naive "shared variable between ISR and main loop" pattern looks like, and it silently breaks the moment data arrives faster than the consumer checks it — which is *exactly* the failure mode that motivates using a proper queue between an ISR and a task. This ring buffer is a manual, single-producer/single-consumer preview of what a FreeRTOS queue formalizes (bounded buffer + safe hand-off between an interrupt context and task context) — planned for Week 4.

---

# Debug Log: FreeRTOS Port Boots Into UsageFault Before main()

## Problem

After integrating the FreeRTOS kernel skeleton, the board flashed successfully but appeared dead at runtime:

- UART echo tests all failed: Python sent data but received `b''`.
- SPI loopback test failed because the UART command path did not respond.
- B1 / PA5 LED behavior also appeared inactive.

## Initial Misread

The first likely suspects were the UART driver, USART2 interrupt setup, FreeRTOS vector ownership, or a half-finished scheduler setup. That was misleading because the symptom looked like a peripheral/runtime problem.

## Real Root Cause

The CPU never reached `main()`.

The hand-written `startup.s` `.data` copy loop incremented only the offset register `r3`, but its termination check compared the unchanged destination pointer `r0` against `_edata`:

```asm
copy_data:
  cmp r0, r1
  bge zero_bss
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4
  b copy_data
```

`r0` stayed at `_sdata`, so the loop condition never changed. The offset kept growing until the code wrote past valid SRAM and triggered a UsageFault. Since this happened in `Reset_Handler`, `main()` never ran.

## How It Was Found

GDB backtrace made the failure concrete:

```text
Program received signal SIGINT, Interrupt.
UsageFault_Handler () at startup.s:78

bt:
#0 UsageFault_Handler
#1 <signal handler called>
#2 copy_data () at startup.s:51
#3 <signal handler called>
```

The register dump was the giveaway:

```text
r0 = 0x20000000   // _sdata
r1 = 0x20000008   // _edata
r2 = 0x08005718   // _sidata
r3 = 0x00020004   // offset had grown far past the 8-byte .data region
```

The `.data` region was only 8 bytes, but the offset had grown to `0x20004`, proving the copy loop did not terminate.

## Fix

The immediate fix was to advance the source and destination pointers themselves, so the comparison against `_edata` can terminate correctly:

```asm
copy_data:
  cmp r0, r1
  bge zero_bss
  ldr r4, [r2]
  str r4, [r0]
  adds r0, r0, #4
  adds r2, r2, #4
  b copy_data
```

After the lesson was captured, the project was moved back to an official STM32F446xx GCC startup template so the boot foundation is no longer hand-rolled.

## Related Issues

- ST-LINK firmware was too old for `ST-LINK_gdbserver`:
  - Before: `V2J33M25`
  - After upgrade: `V2J47M34`
  - Fixed with `STLinkUpgrade.jar`.

- FreeRTOS uses `memcpy` / `memset` internally. In the bare-metal `-nostdlib` style, those symbols were missing, so a minimal `mem.c` was added.

## Takeaway

When peripherals appear dead after a startup or linker change, first prove that the CPU reaches `main()`. GDB backtrace is the fastest way to distinguish:

```text
peripheral bug vs. startup fault vs. exception handler trap
```

This case was not a UART bug and not a FreeRTOS scheduler bug. It was a boot-time `.data` initialization bug.


## FreeRTOS UART Queue Echo Test：Task 輸出干擾測試

### 問題

FreeRTOS UART queue echo 測試時，Python 測試失敗：

- single byte echo：送 `A`，卻收到 `T`
- multi byte echo：送 `Hello`，卻收到 `ask B`
- SPI loopback：送 `S`，期待 `0xAB`，卻收到其他字元

### 誤判方向

一開始可能以為：

- UART ISR 壞了
- Queue 沒收到資料
- SPI loopback 壞了
- FreeRTOS scheduler 有問題

但其實 scheduler 是正常的。

### 真正原因

之前用來驗證 scheduler 的 `task_A` / `task_B` 還在背景印：

```text
Task A
Task B