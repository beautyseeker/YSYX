MEMORY {
  MROM (rx) : ORIGIN = 0x20000000, LENGTH = 4K
  SRAM (rw) : ORIGIN = 0x0f000000, LENGTH = 8K
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
  _erodata = .;

  /* RAM地址空间、存放静态数据段、未初始化数据段、堆区、栈区 */
  .data : { 
    _data_start = .;
    *(.data*)
    *(.sdata*)
  } > SRAM AT> MROM  /* .data段地址在SRAM,但初值在MROM */

  _edata = .;

  .bss : {
    _bss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  } > SRAM  /* .bss段地址在SRAM,由于这个段本身就是待程序初始化，无需LMA获得初值 */
  _ebss = .;

  _stack_top = ALIGN(0x1000); /* 栈地址需4Kb页对齐 */
  . = _stack_top + 0x400;      /* 栈大小1Kb */
  _stack_pointer = .;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
