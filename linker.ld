/* linker.ld — STM32F446RE 記憶體地圖
 *
 * 這個檔告訴 linker：把哪個 section 放到哪個位址。
 * startup.s 會照這裡定義的 symbol（_sdata, _edata, _sbss...）去搬資料。
 */

/* 1. 定義實體記憶體區塊 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K   /* F446RE 有 512K Flash */
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K   /* F446RE 有 128K SRAM  */
}

/* 2. stack 頂端 = SRAM 最高位址（stack 從高往低長） */
_estack = ORIGIN(RAM) + LENGTH(RAM);   /* 0x20020000 */

SECTIONS
{
  /* .isr_vector = 向量表，必須放在 Flash 最前面（0x08000000） */
  .isr_vector :
  {
    KEEP(*(.isr_vector))   /* KEEP = 就算沒被引用也別優化掉 */
  } > FLASH

  /* .text = 程式碼 + 唯讀資料，放 Flash */
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  /* _sidata = .data 初始值在 Flash 裡的存放位址（給 startup 搬移用） */
  _sidata = LOADADDR(.data);

  /* .data = 有初始值的全域變數
   *   執行時住在 SRAM (VMA)，但初始值存在 Flash (LMA)
   *   > RAM AT> FLASH 就是「跑在 RAM、但存在 Flash」的意思 */
  .data :
  {
    _sdata = .;        /* .data 在 SRAM 的起點 */
    *(.data*)
    _edata = .;        /* .data 在 SRAM 的終點 */
  } > RAM AT> FLASH

  /* .bss = 沒初始值的全域變數，開機清零即可，不佔 Flash */
  .bss :
  {
    _sbss = .;         /* .bss 起點 */
    *(.bss*)
    *(COMMON)
    _ebss = .;         /* .bss 終點 */
  } > RAM
}
