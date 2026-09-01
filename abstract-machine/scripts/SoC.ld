MEMORY {
  MROM (rx) : ORIGIN = 0x20000000, LENGTH = 4K
  SRAM (rw) : ORIGIN = 0x0f000000, LENGTH = 8K
  SPI  (rw) : ORIGIN = 0x10001000, LENGTH = 8K
  FLASH(rx) : ORIGIN = 0x30000000, LENGTH = 256M
  PSRAM(rw) : ORIGIN = 0x80000000, LENGTH = 4M
}

ENTRY(_start)
SECTIONS {
  /* ROM地址空间，存放代码段、只读数据段 */
  .text : {
    KEEP(*(entry))  /* KEEP强制保留.entry节防止被链接器优化 */
    *(.text*)
  } > FLASH   /* .text段地址在FLASH,初值固化在内部无需LMA */

  .rodata : { 
    _rodata_start = .;
    *(.rodata*) 
  } > FLASH
  _etext = .;

  /* RAM地址空间、存放静态数据段、未初始化数据段、堆区、栈区 */
  .data : { 
    _data_start = .;
    *(.data*)
    *(.sdata*)
  } > PSRAM AT> FLASH  /* .data段地址在SRAM,但初值在FLASH */

  _edata = .;
  _data_LMA_start = LOADADDR(.data);

  .bss : {
    _bss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  } > PSRAM  /* .bss段地址在SRAM,由于这个段本身就是待程序初始化，无需LMA获得初值 */
  _ebss = .;
  _heap_start = ORIGIN(PSRAM);

  /* 栈指针初始地址为SRAM末尾，低地址方向压栈 */
  _stack_pointer = ORIGIN(PSRAM) + LENGTH(PSRAM);
  /* 栈区1k空间，超过则栈溢出 */
  _stack_top = _stack_pointer - 0x1000;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
