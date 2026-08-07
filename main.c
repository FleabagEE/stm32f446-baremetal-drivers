/* main.c ??Nucleo-F446RE：LD2 (PA5) 顯示 B1 (PC13) ?��??�?? *
 * 三步驟�?
 *   1. ??GPIOA / GPIOC ?��???(RCC)
 *   2. ??GPIO ?��??��? gpio_init / gpio_write / gpio_read
 *   3. 迴�?：�? PC13，偵測�?下�?�?��?��? PA5
 */

#include "practice.h"
#include "FreeRTOS.h"      // ?�這�?
#include "task.h"          // ?�這�?（TaskHandle_t?�xTaskCreate ?�在?��?
#include "queue.h"         // ?�這�?（�?�?ISR?�Queue ?�用?��??��??��?


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

void vApplicationMallocFailedHook(void) {
    for (;;);
}
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

QueueHandle_t uart_rx_queue;

volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_head = 0;
volatile int rx_tail = 0;

/* ISR：�??�到??byte 塞進緩衝�?（�??�者�? */
void USART2_IRQHandler(void) {
    uint8_t byte = USART2->DR;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(uart_rx_queue, &byte, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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


void uart_task(void *params)
{
    (void)params;

    uint8_t byte;

    for (;;) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY) == pdPASS) {
            if (byte == 'S') {
                uint8_t r = spi1_transfer(0xAB);
                uart_write_char((char)r);
            } else {
                uart_write_char((char)byte);
            }
        }
    }
}


int main(void) {
    /* --- Step 1: enable peripheral clocks --- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* --- Step 2: GPIO alternate function setup --- */
    gpio_init(GPIOA, 2, 2);     /* PA2: USART2_TX */
    gpio_init(GPIOA, 3, 2);     /* PA3: USART2_RX */
    gpio_init(GPIOC, 13, 0);    /* PC13: button input, kept for later */

    GPIOA->AFRL &= ~(15U << (2 * 4));
    GPIOA->AFRL |=  (7U  << (2 * 4));
    GPIOA->AFRL &= ~(15U << (3 * 4));
    GPIOA->AFRL |=  (7U  << (3 * 4));

    spi1_init();

    /* --- Step 3: USART2 setup --- */
    USART2->BRR |= (8U << 4) | 11U;
    USART2->CR1 |= (1U << 13);   /* UE */
    USART2->CR1 |= (1U << 3);    /* TE */
    USART2->CR1 |= (1U << 2);    /* RE */

    /* Create RTOS objects before enabling the interrupt that uses them. */
    uart_rx_queue = xQueueCreate(32, sizeof(uint8_t));
    if (uart_rx_queue == NULL) {
        uart_write_str("queue create fail\r\n");
        for (;;);
    }

    BaseType_t ok_uart = xTaskCreate(uart_task, "uart", 128, NULL, 2, NULL);
    if (ok_uart != pdPASS) {
        uart_write_str("uart task create fail\r\n");
        for (;;);
    }

    USART2->CR1 |= (1U << 5);     /* RXNEIE */
    NVIC_IPR[38] = (6U << 4);     /* USART2 IRQ38 priority = 6 */
    NVIC[1] |= (1U << 6);         /* enable USART2 IRQ38 */

    uart_write_str("Hello\r\n");

    vTaskStartScheduler();

    uart_write_str("scheduler returned\r\n");
    for (;;);
}
