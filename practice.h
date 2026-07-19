#ifndef PRACTICE_H
#define PRACTICE_H
#include <stdint.h>

#define PERIPH_BASE       0x40000000UL
#define AHB1PERIPH_BASE   (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE   (PERIPH_BASE + 0x00000000UL)

#define GPIOA_BASE        (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOC_BASE        (AHB1PERIPH_BASE + 0x0800UL)
#define RCC_BASE          (AHB1PERIPH_BASE + 0x3800UL)
#define USART2_BASE       (APB1PERIPH_BASE + 0x4400UL)
#define NVIC_BASE         (0xE000E100UL)
#define NVIC        ((uint32_t *) NVIC_BASE)

typedef struct {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t LCKR;   /* 0x1C，占位用，這次不會操作它 */
  volatile uint32_t AFRL;   /* 0x20，這次真正要用的 */

} GPIO_TypeDef;

typedef struct {
  volatile uint32_t reserved0[12];   /* 0x00~0x2C */
  volatile uint32_t AHB1ENR;          /* 0x30 */
  volatile uint32_t reserved1[3];     /* 0x34~0x3C，跳過 AHB2ENR/AHB3ENR/Reserved */
  volatile uint32_t APB1ENR;          /* 0x40 */
} RCC_TypeDef;

typedef struct {
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t BRR;
  volatile uint32_t CR1;
} USART_TypeDef;

#define GPIOA   ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOC   ((GPIO_TypeDef *) GPIOC_BASE)
#define RCC     ((RCC_TypeDef  *) RCC_BASE)
#define USART2  ((USART_TypeDef *) USART2_BASE)

#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)
#define RCC_APB1ENR_USART2EN  (1U << 17)

#endif
