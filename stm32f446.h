/* stm32f446.h — 暫存器位址定義（只放點燈需要的）
 *
 * 位址全部對照 RM0390 (STM32F446 Reference Manual)。
 * 這裡示範「怎麼把 datasheet 的位址變成 C 能用的東西」——
 * 這是 register-level driver 的核心手法。
 */
#ifndef STM32F446_H
#define STM32F446_H

#include <stdint.h>

/* ---- Base addresses (RM0390 §2.3 Memory map) ----
 * 週邊掛在不同 bus 上，base 位址不同：
 *   RCC   在 AHB1  → 0x40023800
 *   GPIOA 在 AHB1  → 0x40020000
 */
#define PERIPH_BASE       0x40000000UL
#define AHB1PERIPH_BASE   (PERIPH_BASE + 0x00020000UL)   /* 0x40020000 */

#define GPIOA_BASE        (AHB1PERIPH_BASE + 0x0000UL)   /* 0x40020000 */
#define RCC_BASE          (AHB1PERIPH_BASE + 0x3800UL)   /* 0x40023800 */

/* ---- 用 struct 疊出 register block ----
 * 把連續的暫存器用 struct 描述，欄位順序 = 記憶體位址順序。
 * volatile 必須加：告訴編譯器這些值硬體會改，不准做快取優化。
 */

/* GPIO register block (RM0390 §8.4) */
typedef struct {
  volatile uint32_t MODER;    /* 0x00: 模式 (input/output/AF/analog) */
  volatile uint32_t OTYPER;   /* 0x04: 輸出型態 (push-pull/open-drain) */
  volatile uint32_t OSPEEDR;  /* 0x08: 輸出速度 */
  volatile uint32_t PUPDR;    /* 0x0C: 上/下拉 */
  volatile uint32_t IDR;      /* 0x10: 輸入資料 (讀 pin 狀態) */
  volatile uint32_t ODR;      /* 0x14: 輸出資料 (寫 pin 高低) */
  volatile uint32_t BSRR;     /* 0x18: bit set/reset (原子操作，推薦用這個) */
} GPIO_TypeDef;

/* RCC register block — 這裡只列到我們要用的 AHB1ENR (RM0390 §6.3) */
typedef struct {
  volatile uint32_t reserved0[12];  /* 0x00~0x2C：跳過不用的 */
  volatile uint32_t AHB1ENR;        /* 0x30: AHB1 週邊時脈開關 */
} RCC_TypeDef;

/* ---- 把 base 位址 cast 成 struct 指標 ----
 * 之後就能用 GPIOA->MODER 這種寫法，等同直接操作那個位址。
 */
#define GPIOA  ((GPIO_TypeDef *) GPIOA_BASE)
#define RCC    ((RCC_TypeDef  *) RCC_BASE)

/* ---- 常用 bit 定義 ---- */
#define RCC_AHB1ENR_GPIOAEN  (1U << 0)   /* AHB1ENR bit0 = GPIOA 時脈致能 */

#endif /* STM32F446_H */
