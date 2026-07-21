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
