MEMORY {
  MROM (rx) : ORIGIN = 0x20000000, LENGTH = 4K
  SRAM (rw) : ORIGIN = 0x0f000000, LENGTH = 8K
  SPI  (rw) : ORIGIN = 0x10001000, LENGTH = 8K
  FLASH(rx)  : ORIGIN = 0x30000000, LENGTH = 256M
}

ENTRY(_start)
SECTIONS {
  /* ROM地址空间，存放代码段、只读数据段 */
  .text : {
    KEEP(*(entry))  /* KEEP强制保留.entry节防止被链接器优化 */
    *(.text*)
  } > MROM   /* .text段地址在MROM,初值固化在内部无需LMA */

  .rodata : { 
    _rodata_start = .;
    *(.rodata*) 
  } > MROM
  _etext = .;

  /* RAM地址空间、存放静态数据段、未初始化数据段、堆区、栈区 */
  .data : { 
    _data_start = .;
    *(.data*)
    *(.sdata*)
  } > SRAM AT> MROM  /* .data段地址在SRAM,但初值在MROM */

  _edata = .;
  _data_LMA_start = LOADADDR(.data);

  .bss : {
    _bss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  } > SRAM  /* .bss段地址在SRAM,由于这个段本身就是待程序初始化，无需LMA获得初值 */
  _ebss = .;
  _heap_start = ORIGIN(SRAM);

  /* 栈指针初始地址为SRAM末尾，低地址方向压栈 */
  _stack_pointer = ORIGIN(SRAM) + LENGTH(SRAM);
  /* 栈区1k空间，超过则栈溢出 */
  _stack_top = _stack_pointer - 0x400;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
