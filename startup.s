/* startup.s — STM32F446RE 啟動碼（簡化版）
 *
 * 做三件事：
 *   1. 定義向量表（前兩個 entry：MSP 初始值、Reset_Handler）
 *   2. Reset_Handler：搬 .data、清 .bss，然後跳 main
 *   3. 預設的無窮迴圈 handler
 */

.syntax unified          /* 用統一的 Thumb-2 語法 */
.cpu cortex-m4
.thumb                   /* Cortex-M 只跑 Thumb 指令 */

/* ============ 向量表 ============ */
.section .isr_vector, "a"
.word _estack            /* [0x00] MSP 初始值（來自 linker.ld） */
.word Reset_Handler      /* [0x04] reset 後 PC 跳這裡 */
.word Default_Handler    /* [0x08] NMI */
.word Default_Handler    /* [0x0C] HardFault */
.rept 50
  .word Default_Handler
.endr
.word USART2_IRQHandler


/* 其餘 exception / IRQ 這裡先省略，點燈用不到 */

/* ============ Reset_Handler ============ */
.section .text
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:

  /* --- Step 1: 把 .data 從 Flash 搬到 SRAM --- */
  ldr r0, =_sdata        /* r0 = SRAM 目的地起點 */
  ldr r1, =_edata        /* r1 = 目的地終點 */
  ldr r2, =_sidata       /* r2 = Flash 來源起點 */
  movs r3, #0
copy_data:
  cmp r0, r1             /* 搬到終點了嗎？ */
  bge zero_bss           /* 是 → 去清 bss */
  ldr r4, [r2, r3]       /* 從 Flash 讀一個 word */
  str r4, [r0, r3]       /* 寫進 SRAM */
  adds r3, r3, #4        /* offset += 4 */
  b copy_data

  /* --- Step 2: 把 .bss 清零 --- */
zero_bss:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
clear_bss:
  cmp r0, r1
  bge call_main
  str r2, [r0]           /* 寫 0 */
  adds r0, r0, #4
  b clear_bss

  /* --- Step 3: 跳進 main() --- */
call_main:
  bl main
  b .                    /* main 若返回，卡在這裡 */

/* ============ Default_Handler ============ */
.type Default_Handler, %function
.global Default_Handler
Default_Handler:
  b .                    /* 抓到未預期的中斷就停在這，方便 debug */
