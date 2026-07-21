/* main.c — Nucleo-F446RE：LD2 (PA5) 顯示 B1 (PC13) 按鈕狀態
 *
 * 三步驟：
 *   1. 開 GPIOA / GPIOC 的時脈 (RCC)
 *   2. 把 GPIO 操作包成 gpio_init / gpio_write / gpio_read
 *   3. 迴圈：讀 PC13，偵測按下邊緣就切換 PA5
 */
#include "practice.h"

/* 簡單的忙等 delay（不精準，點燈夠用；之後會用 timer 取代） */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile("nop");
    }
}

/* mode: 0=input, 1=output, 2=alternate function, 3=analog（對照 MODER 定義） */
void gpio_init(GPIO_TypeDef *port, int pin, int mode) {
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |=  ((uint32_t)mode << (pin * 2));
}


/* val 非 0 → 拉高；val 是 0 → 拉低 */
void gpio_write(GPIO_TypeDef *port, int pin, int val) {
    if (val) {
        port->BSRR = (1U << pin);
    } else {
        port->BSRR = (1U << (pin + 16));
    }
}

/* 回傳乾淨的 0 或 1 */
int gpio_read(GPIO_TypeDef *port, int pin) {
    return (port->IDR & (1U << pin)) != 0;
}

/* 輪詢等 TXE=1（發送暫存器空了）才寫 DR */
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

/* ISR：把收到的 byte 塞進緩衝區（生產者） */
void USART2_IRQHandler(void) {
    uint8_t byte = USART2->DR;   /* 讀 DR 同時自動清 RXNE */
    int next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) {
        rx_buf[rx_head] = byte;
        rx_head = next_head;
    }
}

/* 還有資料可讀嗎？（head 追上 tail 代表空的） */
int rx_available(void) {
    return rx_head != rx_tail;
}

/* 從緩衝區取一個 byte（消費者，main() 用） */
uint8_t rx_pop(void) {
    uint8_t byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return byte;
}

int main(void) {
    /* --- Step 1: 開時脈 ---
     * 週邊預設是關的（省電），用之前一定要先在 RCC 開時脈。 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;


    /* --- Step 2: 設定 pin 模式 --- */
    gpio_init(GPIOA, 5, 1);    /* PA5 → output */
    gpio_init(GPIOC, 13, 0);   /* PC13 → input */
    gpio_init(GPIOA, 2, 2);
    gpio_init(GPIOA, 3, 2);

    GPIOA->AFRL &= ~(15U << (2 * 4));
    GPIOA->AFRL |= (7U << (2 * 4));
    GPIOA->AFRL &= ~(15U << (3 * 4));   /* 清 pin3 的欄位 */
    GPIOA->AFRL |= (7U << (3 * 4));

    USART2->BRR |= (8U << 4) | 11U;

    USART2->CR1 |= (1U << 13);   /* UE */
    USART2->CR1 |= (1U << 3);    /* TE */

    USART2->CR1 |= (1U << 2);    /* RE */
    USART2->CR1 |= (1U << 5);    /* RXNEIE */

    NVIC[1] |= (1U << 6);


    uart_write_str("Hello\r\n");

    int last_button = 1;   /* 初始假設「沒按」（B1 有外部上拉，放開讀到 1） */
    int led_on = 0;

    /* --- Step 3: 按鈕切換 LED + UART echo --- */
    while (1) {
        int button_now = gpio_read(GPIOC, 13);   /* PC13 低電位表示按下去 */

        if (button_now == 0 && last_button == 1) {
            led_on = !led_on;   /* 偵測到「放開→按下」的邊緣，才切換 */
        }

        gpio_write(GPIOA, 5, led_on);
        delay(1000000);
        last_button = button_now;   /* 記錄這次的狀態，給下一圈迴圈用 */

        while (rx_available()) {
            uart_write_char(rx_pop());
        }
    }
}
