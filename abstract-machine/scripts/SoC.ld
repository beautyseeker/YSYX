ENTRY(_start)
SECTIONS {
  /* CPU 复位 PC = 0x20000000，必须把 _start（entry）放在最前 */
  . = 0x20000000;
  .text : {
    KEEP(*(entry))
    *(.text*)
  }
  .rodata : { *(.rodata*) }
  /* 可写数据/栈在 SRAM：0x0f000000，大小 8KB（与 ysyxSoC AXI4RAM 一致） */
  . = 0x0f000000;
  .data : { *(.data*) }
  .bss : {
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0xf00000;
  _stack_pointer = .;
  _end = .;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
