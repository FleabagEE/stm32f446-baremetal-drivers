# W4 Day 6 - SPI1 Loopback Bring-up

## Goal

Verify register-level SPI1 transfer with MOSI-MISO loopback.

Expected result:

```text
spi1_transfer(0xAB) returns 0xAB

Hardware
Board: Nucleo-F446RE
SPI1 pins:
PA5 = SPI1_SCK  AF5
PA6 = SPI1_MISO AF5
PA7 = SPI1_MOSI AF5
Loopback wiring:
PA7(MOSI) <-> PA6(MISO)
NSS:
Software NSS
SSM = 1
SSI = 1
Register Setup Summary
RCC:
GPIOAEN = 1
SPI1EN  = 1
GPIO:
PA5/PA6/PA7 MODER = 10, alternate function
PA5/PA6/PA7 AFRL  = AF5
SPI1_CR1:
MSTR = 1
BR   = 111, fPCLK/256
SSM  = 1
SSI  = 1
SPE  = 1, enabled last
SPI transfer flow:
wait TXE
write SPI1->DR
wait RXNE
read SPI1->DR
Result
UART output:
Hello
SPI=0xAB
Loopback test passed.

Notes
Important pitfall:
SSM=1 but SSI=0 can make master see NSS low,
causing mode fault and SPI not working.
PA5 is used as SPI1_SCK during this test, so LD2 LED control must be disabled.