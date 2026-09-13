MEMORY {
  MROM (rx) : ORIGIN = 0x20000000, LENGTH = 4K
  SRAM (rw) : ORIGIN = 0x0f000000, LENGTH = 8K
  SPI  (rw) : ORIGIN = 0x10001000, LENGTH = 8K
  FLASH(rx) : ORIGIN = 0x30000000, LENGTH = 256M
  PSRAM(rw) : ORIGIN = 0x80000000, LENGTH = 4M
}

ENTRY(_start)
SECTIONS {
  /* FLASH地址空间，存放启动代码段、只读数据段 */
  .text : {
    KEEP(*(entry))
    *(.text*)
  } > FLASH

  /*
   * mem-test 的代码 + 其只读数据（字符串、跳转表等）
   * VMA=SRAM，LMA=Flash；开机 sram_text_init() 整段拷入再执行
   */
  .sram.text : {
    . = ALIGN(4);
    _sram_text_start = .;
    *(.sram.text)
    /* printf 字符串等仍叫 .rodata*，跟函数 attribute 无关，按文件收进来 */
    *mem-test*.o(.rodata*)
    *mem-test*.o(.srodata*)
    . = ALIGN(4);
    _sram_text_end = .;
  } > SRAM AT> FLASH
  _sram_text_lma = LOADADDR(.sram.text);

  .rodata : { 
    _rodata_start = .;
    *(EXCLUDE_FILE(*mem-test*.o) .rodata*)
    *(EXCLUDE_FILE(*mem-test*.o) .srodata*)
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
  _stack_pointer = ORIGIN(SRAM) + LENGTH(SRAM);
  /* 栈区4k空间，超过则栈溢出 */
  _stack_top = _stack_pointer - 0x1000;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
