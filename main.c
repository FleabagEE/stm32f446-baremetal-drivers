/* main.c ??Nucleo-F446REï¼šLD2 (PA5) é¡¯ç¤º B1 (PC13) ?‰é??€?? *
 * ä¸‰æ­¥é©Ÿï?
 *   1. ??GPIOA / GPIOC ?„æ???(RCC)
 *   2. ??GPIO ?ä??…æ? gpio_init / gpio_write / gpio_read
 *   3. è¿´å?ï¼šè? PC13ï¼Œåµæ¸¬æ?ä¸‹é?ç·?°±?‡æ? PA5
 */
#include "practice.h"

/* ç°¡å–®?„å?ç­?delayï¼ˆä?ç²¾æ?ï¼Œé??ˆå??¨ï?ä¹‹å??ƒç”¨ timer ?–ä»£ï¼?*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile("nop");
    }
}

/* mode: 0=input, 1=output, 2=alternate function, 3=analogï¼ˆå???MODER å®šç¾©ï¼?*/
void gpio_init(GPIO_TypeDef *port, int pin, int mode) {
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |=  ((uint32_t)mode << (pin * 2));
}


/* val ??0 ???‰é?ï¼›val ??0 ???‰ä? */
void gpio_write(GPIO_TypeDef *port, int pin, int val) {
    if (val) {
        port->BSRR = (1U << pin);
    } else {
        port->BSRR = (1U << (pin + 16));
    }
}

/* ?žå‚³ä¹¾æ·¨??0 ??1 */
int gpio_read(GPIO_TypeDef *port, int pin) {
    return (port->IDR & (1U << pin)) != 0;
}

/* è¼ªè©¢ç­?TXE=1ï¼ˆç™¼?æš«å­˜å™¨ç©ºä?ï¼‰æ?å¯?DR */
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

/* ISRï¼šæ??¶åˆ°??byte å¡žé€²ç·©è¡å?ï¼ˆç??¢è€…ï? */
void USART2_IRQHandler(void) {
    uint8_t byte = USART2->DR;   /* è®€ DR ?Œæ??ªå?æ¸?RXNE */
    int next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) {
        rx_buf[rx_head] = byte;
        rx_head = next_head;
    }
}

/* ?„æ?è³‡æ??¯è??Žï?ï¼ˆhead è¿½ä? tail ä»?¡¨ç©ºç?ï¼?*/
int rx_available(void) {
    return rx_head != rx_tail;
}

/* å¾žç·©è¡å??–ä???byteï¼ˆæ?è²»è€…ï?main() ?¨ï? */
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
    /* --- Step 1: ?‹æ???---
     * ?±é??è¨­?¯é??„ï??é›»ï¼‰ï??¨ä??ä?å®šè??ˆåœ¨ RCC ?‹æ??ˆã€?*/
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    


    /* --- Step 2: è¨­å? pin æ¨¡å? --- */
    
    gpio_init(GPIOA, 2, 2);
    gpio_init(GPIOA, 3, 2);
    gpio_init(GPIOC, 13, 0);

    GPIOA->AFRL &= ~(15U << (2 * 4));
    GPIOA->AFRL |= (7U << (2 * 4));
    GPIOA->AFRL &= ~(15U << (3 * 4));   /* æ¸?pin3 ?„æ?ä½?*/
    GPIOA->AFRL |= (7U << (3 * 4));

    
    spi1_init();

    USART2->BRR |= (8U << 4) | 11U;

    USART2->CR1 |= (1U << 13);   /* UE */
    USART2->CR1 |= (1U << 3);    /* TE */

    USART2->CR1 |= (1U << 2);    /* RE */
    USART2->CR1 |= (1U << 5);    /* RXNEIE */

    NVIC[1] |= (1U << 6);



    uart_write_str("Hello\r\n");

    //int last_button = 1;   /* ?å??‡è¨­?Œæ??‰ã€ï?B1 ?‰å??¨ä??‰ï??¾é?è®€??1ï¼?*/
    //int led_on = 0;
    uint8_t r = spi1_transfer(0xAB);
    uart_write_str("SPI="); 
    uart_write_hex8(r);
    uart_write_str("\r\n");

    /* --- Step 3: ?‰é??‡æ? LED + UART echo --- */
    while (1) {
        //int button_now = gpio_read(GPIOC, 13);   /* PC13 ä½Žé›»ä½è¡¨ç¤ºæ?ä¸‹åŽ» */

        //if (button_now == 0 && last_button == 1) {
        //    led_on = !led_on;   /* ?µæ¸¬?°ã€Œæ”¾?‹â??‰ä??ç??Šç·£ï¼Œæ??‡æ? */
        //}

       // gpio_write(GPIOA, 5, led_on);   /* PA5 ??Ž¥ LD2ï¼Œæ?é«˜äº®?ˆï??‰ä??„ç? */
        //delay(1000000);
        //last_button = button_now;   /* è¨˜é??™æ¬¡?„ç??‹ï?çµ¦ä?ä¸€?ˆè¿´?ˆç”¨ */


        while (rx_available()) {
            uart_write_char(rx_pop());
        }
    }


}
