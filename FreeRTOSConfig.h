/*
 * FreeRTOSConfig.h  —  STM32F446RE (Cortex-M4F) 專用範本
 * ---------------------------------------------------------
 * 用途：放進你的 bare-metal 專案根目錄(或 inc/)，讓 Makefile 的 include path 找得到。
 * 每個設定都有註解說明「這是什麼、為什麼這樣設」，方便你面試被問時能解釋。
 *
 * 前提假設：
 *   - 系統時脈 = 16 MHz (預設 HSI，沒改 PLL 的話就是這個)
 *   - Cortex-M4F，優先級 4 bits (STM32 全系列都是 4 bits)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
/* Tick 計數器的位元寬度。新版 FreeRTOS 用這個取代舊的 configUSE_16_BIT_TICKS。
 * 設 32 = 用 32-bit tick counter（32-bit MCU 的標準選擇，範圍大不易溢位）。 */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS

/*-----------------------------------------------------------
 * 1. 排程器基本行為
 *----------------------------------------------------------*/

/* 1 = preemptive(搶佔式)。高優先級 task 就緒時立刻搶佔低優先級的。
 * 這是你履歷/面試要講的「preemptive RTOS」，設 1。 */
#define configUSE_PREEMPTION                    1

/* 讓 FreeRTOS 用硬體最佳化的方式挑下一個要跑的 task。
 * Cortex-M 有 CLZ 指令可以加速，設 0 用通用版即可(新手先求穩)。 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* 同優先級的 task 之間，是否在每個 tick 輪流(time slicing)。
 * 設 1 = 同優先級 round-robin(你 W2 OS 複習學的)。 */
#define configUSE_TIME_SLICING                  1


/*-----------------------------------------------------------
 * 2. 時脈與 tick
 *----------------------------------------------------------*/

/* CPU 時脈，單位 Hz。務必和你實際的系統時脈一致，否則 delay 時間全錯。
 * 預設 HSI = 16 MHz。如果你之後開了 PLL 拉到更高頻，記得改這裡。 */
#define configCPU_CLOCK_HZ                      ( 16000000UL )

/* 每秒幾個 tick。1000 = 每 1ms 一個 tick(最常見)。
 * 這決定 vTaskDelay() 的時間解析度。 */
#define configTICK_RATE_HZ                      ( 1000 )


/*-----------------------------------------------------------
 * 3. 記憶體與 task 資源
 *----------------------------------------------------------*/

/* 最高優先級數。task 優先級範圍是 0 ~ (configMAX_PRIORITIES - 1)。
 * 數字越大越優先。5 個對你的專案夠用。 */
#define configMAX_PRIORITIES                    ( 5 )

/* 每個 task 最小 stack 大小(單位是 word，不是 byte！CM4 上 1 word = 4 bytes)。
 * 128 words = 512 bytes。ISR→Queue task 這種簡單 task 夠用。 */
#define configMINIMAL_STACK_SIZE                ( 128 )

/* FreeRTOS 的 heap 總大小(byte)。task/queue 都從這裡配。
 * F446RE 有 128KB RAM，給 FreeRTOS 10KB 很寬裕。不夠再往上加。 */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) )

/* task 名字最大長度(字元數)，純除錯用途。 */
#define configMAX_TASK_NAME_LEN                 ( 16 )

/* 1 = 用動態配置(xTaskCreate 這種，從 heap 配)。新手用這個最單純。 */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
/* 1 = 也允許靜態配置(xTaskCreateStatic)。先設 0 不用。 */
#define configSUPPORT_STATIC_ALLOCATION         0


/*-----------------------------------------------------------
 * 4. Hook 函式(先全關，之後要 debug 再開)
 *----------------------------------------------------------*/

/* Idle task 的 hook。0 = 不用。 */
#define configUSE_IDLE_HOOK                     0
/* 每個 tick 的 hook。0 = 不用。 */
#define configUSE_TICK_HOOK                     0

/* stack 溢位檢查。2 = 完整檢查(會在 task 切換時檢查 stack 有沒有爆)。
 * 開發階段強烈建議設 2，抓 stack overflow 超有用(你 W7 要學的主題)。
 * 設 2 需要你自己提供 vApplicationStackOverflowHook() 函式。 */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* heap 配置失敗的 hook。1 = 開。configTOTAL_HEAP_SIZE 不夠時會呼叫，
 * 需要你提供 vApplicationMallocFailedHook()。開著能及早發現 heap 不夠。 */
#define configUSE_MALLOC_FAILED_HOOK            1


/*-----------------------------------------------------------
 * 5. 功能開關(用到才開，省 code size)
 *----------------------------------------------------------*/

#define configUSE_MUTEXES                       1  /* W6 priority inversion 要用 */
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configQUEUE_REGISTRY_SIZE               8  /* 除錯時可註冊 queue 名字 */


/*-----------------------------------------------------------
 * 6. 中斷優先級(Cortex-M 最容易出錯的地方！仔細看註解)
 *----------------------------------------------------------*/

/* STM32(所有系列)的中斷優先級是 4 bits。這是硬體決定的，別改。 */
#define configPRIO_BITS                         4

/* 最低優先級的數值。4 bits → 0~15，最低是 15。
 * 注意 Cortex-M 的「數字越大 = 優先級越低」(和 task 相反！) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* 「最高的、仍然可以呼叫 FreeRTOS API」的中斷優先級數值。
 * 比這個「數字更小(=優先級更高)」的中斷，不可以呼叫任何 FreeRTOS API，
 * 也不會被 FreeRTOS 的臨界區(critical section)遮蔽。
 * 設 5 是常見安全值。 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* 下面兩個是把上面的「優先級數值」左移到 CM4 硬體暫存器要求的位置。
 * (CM4 的優先級放在 byte 的高 4 bits，所以要左移 8-configPRIO_BITS = 4 位)
 * 這兩行照抄即可，不用改。 */
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/*
 * ★★★ 你的 UART 中斷優先級一定要 >= 5(數字上) ★★★
 * 因為你的 USART2_IRQHandler 裡會呼叫 xQueueSendFromISR(),
 * 那是 FreeRTOS API。只有優先級數值 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY(=5)
 * 的中斷才能安全呼叫 FromISR 的 API。
 * 所以在你的程式裡設 NVIC 時：
 *     NVIC_SetPriority(USART2_IRQn, 6);   // 6 >= 5，安全
 * 如果設成比 5 小的數字(更高優先級)，一呼叫 FromISR API 就會觸發 assert 或 hard fault。
 * 這是新手最常踩的坑，先記住。
 */


/*-----------------------------------------------------------
 * 7. 中斷向量對應(解決 startup.s 向量表衝突)
 *----------------------------------------------------------*/
/*
 * 你的 startup.s 向量表裡用的名字是 SVC_Handler / PendSV_Handler / SysTick_Handler,
 * 但 FreeRTOS 內部的 handler 叫 vPortSVCHandler / xPortPendSVHandler / xPortSysTickHandler。
 * 下面三行把 FreeRTOS 的名字對應到你向量表的名字，這樣不用改 startup.s。
 *
 * ★ 如果你的 startup.s 用的是別的名字，改這裡對應，或改 startup.s 二選一。
 * ★ 不做這步 = 排程不會動 = 跑起來卡死/hard fault。
 */
#define vPortSVCHandler         SVC_Handler
#define xPortPendSVHandler      PendSV_Handler
#define xPortSysTickHandler     SysTick_Handler


/*-----------------------------------------------------------
 * 8. assert(開發階段強烈建議開)
 *----------------------------------------------------------*/
/*
 * configASSERT 會在 FreeRTOS 內部偵測到「你用錯了」時觸發。
 * 例如上面說的「在太高優先級的 ISR 裡呼叫 API」就會被這裡抓到。
 * 開發時開著，能把很多隱晦的當機變成明確的停點。
 * 這裡用無窮迴圈，你可以在 debugger 裡看到它停在哪、backtrace 找原因。
 */
#define configASSERT( x )   if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }


/*-----------------------------------------------------------
 * 9. 要包含哪些 API 函式(用到的設 1)
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1

#endif /* FREERTOS_CONFIG_H */
