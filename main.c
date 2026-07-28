/* main.c ??Nucleo-F446RE：LD2 (PA5) 顯示 B1 (PC13) ?��??�?? *
 * 三步驟�?
 *   1. ??GPIOA / GPIOC ?��???(RCC)
 *   2. ??GPIO ?��??��? gpio_init / gpio_write / gpio_read
 *   3. 迴�?：�? PC13，偵測�?下�?�?��?��? PA5
 */
#include "practice.h"

/* 簡單?��?�?delay（�?精�?，�??��??��?之�??�用 timer ?�代�?*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile("nop");
    }
}

/* mode: 0=input, 1=output, 2=alternate function, 3=analog（�???MODER 定義�?*/
void gpio_init(GPIO_TypeDef *port, int pin, int mode) {
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |=  ((uint32_t)mode << (pin * 2));
}


/* val ??0 ???��?；val ??0 ???��? */
void gpio_write(GPIO_TypeDef *port, int pin, int val) {
    if (val) {
        port->BSRR = (1U << pin);
    } else {
        port->BSRR = (1U << (pin + 16));
    }
}

/* ?�傳乾淨??0 ??1 */
int gpio_read(GPIO_TypeDef *port, int pin) {
    return (port->IDR & (1U << pin)) != 0;
}

/* 輪詢�?TXE=1（發?�暫存器空�?）�?�?DR */
void uart_write_char(char c) {
    while (!(USART2->SR & (1U << 7)));
    USART2->DR = c;
}


void uart_write_str(const char *s) {
    while (*s) {
        uart_write_char(*s);
        s++;
    }
}




#define RX_BUF_SIZE 32

volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_head = 0;
volatile int rx_tail = 0;

/* ISR：�??�到??byte 塞進緩衝�?（�??�者�? */
void USART2_IRQHandler(void) {
    uint8_t byte = USART2->DR;   /* 讀 DR ?��??��?�?RXNE */
    int next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) {
        rx_buf[rx_head] = byte;
        rx_head = next_head;
    }
}

/* ?��?資�??��??��?（head 追�? tail �?��空�?�?*/
int rx_available(void) {
    return rx_head != rx_tail;
}

/* 從緩衝�??��???byte（�?費者�?main() ?��? */
uint8_t rx_pop(void) {
    uint8_t byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return byte;
}
uint8_t spi1_transfer(uint8_t data)
{
    while (!(SPI1->SR & (1U << 1))) {
        /* wait TXE = 1 */
    }

    SPI1->DR = data;  /* start transfer */

    while (!(SPI1->SR & (1U << 0))) {
        /* wait RXNE = 1 */
    }

    return (uint8_t)SPI1->DR;  /* read received byte */
}

void spi1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;  /* enable SPI1 clock */

    gpio_init(GPIOA, 5, 2);
    gpio_init(GPIOA, 6, 2);
    gpio_init(GPIOA, 7, 2);

    GPIOA->AFRL &= ~(15U << (5 * 4));  /* clear PA5 AF */
    GPIOA->AFRL &= ~(15U << (6 * 4));  /* clear PA6 AF */
    GPIOA->AFRL &= ~(15U << (7 * 4));  /* clear PA7 AF */

    GPIOA->AFRL |= (5U << (5 * 4));    /* PA5 AF5: SPI1_SCK */
    GPIOA->AFRL |= (5U << (6 * 4));    /* PA6 AF5: SPI1_MISO */
    GPIOA->AFRL |= (5U << (7 * 4));    /* PA7 AF5: SPI1_MOSI */

    SPI1->CR1 |= (1U << 2);     /* MSTR: master */
    SPI1->CR1 |= (7U << 3);     /* BR: fPCLK/256 */
    SPI1->CR1 |= (1U << 8);     /* SSI: internal NSS high */
    SPI1->CR1 |= (1U << 9);     /* SSM: software NSS */
    SPI1->CR1 |= (1U << 6);     /* SPE: enable SPI */
}
void uart_write_hex8(uint8_t x)
{
    const char hex[] = "0123456789ABCDEF";

    uart_write_str("0x");
    uart_write_char(hex[(x >> 4) & 0xF]);
    uart_write_char(hex[x & 0xF]);
}


int main(void) {
    /* --- Step 1: ?��???---
     * ?��??�設?��??��??�電）�??��??��?定�??�在 RCC ?��??��?*/
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    


    /* --- Step 2: 設�? pin 模�? --- */
    
    gpio_init(GPIOA, 2, 2);
    gpio_init(GPIOA, 3, 2);
    gpio_init(GPIOC, 13, 0);

    GPIOA->AFRL &= ~(15U << (2 * 4));
    GPIOA->AFRL |= (7U << (2 * 4));
    GPIOA->AFRL &= ~(15U << (3 * 4));   /* �?pin3 ?��?�?*/
    GPIOA->AFRL |= (7U << (3 * 4));

    
    spi1_init();

    USART2->BRR |= (8U << 4) | 11U;

    USART2->CR1 |= (1U << 13);   /* UE */
    USART2->CR1 |= (1U << 3);    /* TE */

    USART2->CR1 |= (1U << 2);    /* RE */
    USART2->CR1 |= (1U << 5);    /* RXNEIE */

    NVIC[1] |= (1U << 6);



    uart_write_str("Hello\r\n");

    //int last_button = 1;   /* ?��??�設?��??�」�?B1 ?��??��??��??��?讀??1�?*/
    //int led_on = 0;
    //uint8_t r = spi1_transfer(0xAB);
    //uart_write_str("SPI="); 
    //uart_write_hex8(r);
    //uart_write_str("\r\n");

    /* --- Step 3: ?��??��? LED + UART echo --- */
    while (1) {
        //int button_now = gpio_read(GPIOC, 13);   /* PC13 低電位表示�?下去 */

        //if (button_now == 0 && last_button == 1) {
        //    led_on = !led_on;   /* ?�測?�「放?��??��??��??�緣，�??��? */
        //}

       // gpio_write(GPIOA, 5, led_on);   /* PA5 ??�� LD2，�?高亮?��??��??��? */
        //delay(1000000);
        //last_button = button_now;   /* 記�??�次?��??��?給�?一?�迴?�用 */


        while (rx_available()) {
            uint8_t cmd = rx_pop();

            if (cmd == 'S') {
                uint8_t r = spi1_transfer(0xAB);
                uart_write_char((char)r);
            } else {
                uart_write_char((char)cmd);
            }
        }
    }


}
