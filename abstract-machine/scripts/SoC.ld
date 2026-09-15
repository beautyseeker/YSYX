MEMORY {
  SRAM (rw) : ORIGIN = 0x0f000000, LENGTH = 8K
  SPI  (rw) : ORIGIN = 0x10001000, LENGTH = 8K
  MROM (rx) : ORIGIN = 0x20000000, LENGTH = 4K
  FLASH(rx) : ORIGIN = 0x30000000, LENGTH = 256M
  PSRAM(rw) : ORIGIN = 0x80000000, LENGTH = 4M
}

ENTRY(_start)
SECTIONS {
  /*
   * Flash 极小引导：_start + _flash_boot（只把 SRAM loader 拷进 SRAM）
   */
  .boot : {
    KEEP(*(entry))
    *start*.o(.text*)
    *(.text.boot)
    *(.text.boot.*)
  } > FLASH

  /* SRAM 搬运器：在片上取指，把 .text/.data 从 Flash 搬到 PSRAM */
  .sram.loader : {
    . = ALIGN(4);
    _sram_loader_start = .;
    *(.sram.loader)
    *(.sram.loader.*)
    . = ALIGN(4);
    _sram_loader_end = .;
  } > SRAM AT> FLASH
  _sram_loader_lma = LOADADDR(.sram.loader);

  /* 主程序：VMA=PSRAM，LMA=Flash；由 _sram_loader 拷入后执行 */
  .text : {
    . = ALIGN(4);
    _text_start = .;
    *(EXCLUDE_FILE(*start*.o) .text*)
    . = ALIGN(4);
    _etext = .;
  } > PSRAM AT> FLASH
  _text_lma = LOADADDR(.text);

  .rodata : {
    _rodata_start = .;
    *(EXCLUDE_FILE(*mem-test*.o) .rodata*)
    *(EXCLUDE_FILE(*mem-test*.o) .srodata*)
  } > FLASH

  /* mem-test：VMA=SRAM，开机再拷入 */
  .sram.text : {
    . = ALIGN(4);
    _sram_text_start = .;
    *(.sram.text)
    *mem-test*.o(.rodata*)
    *mem-test*.o(.srodata*)
    . = ALIGN(4);
    _sram_text_end = .;
  } > SRAM AT> FLASH
  _sram_text_lma = LOADADDR(.sram.text);

  .data : {
    . = ALIGN(4);
    _data_start = .;

    /* RT-Thread：须进 PSRAM。勿用 extra.ld INSERT（会贴在 .sram.text 后误进 8K SRAM） */
    . = ALIGN(8);
    __fsymtab_start = .;
    KEEP(*(FSymTab))
    __fsymtab_end = .;
    . = ALIGN(8);
    __vsymtab_start = .;
    KEEP(*(VSymTab))
    __vsymtab_end = .;
    . = ALIGN(8);
    __rt_init_start = .;
    KEEP(*(SORT(.rti_fn*)))
    __rt_init_end = .;
    . = ALIGN(8);
    __rt_utest_tc_tab_start = .;
    KEEP(*(UtestTcTab))
    __rt_utest_tc_tab_end = .;
    . = ALIGN(8);
    __am_apps_data_start = .;
    *(__am_apps.data*)
    *(__am_apps.sdata*)
    __am_apps_data_end = .;
    . = ALIGN(8);

    *(.data*)
    *(.sdata*)
  } > PSRAM AT> FLASH

  _edata = .;
  _data_LMA_start = LOADADDR(.data);

  .bss : {
    . = ALIGN(4);
    _bss = .;
    . = ALIGN(8);
    __am_apps_bss_start = .;
    *(__am_apps.bss*)
    *(__am_apps.sbss*)
    *(__am_apps.scommon*)
    __am_apps_bss_end = .;
    . = ALIGN(8);
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  } > PSRAM  /* .bss段地址在SRAM,由于这个段本身就是待程序初始化，无需LMA获得初值 */
  _ebss = .;

  /* mem-test 要求 lo/hi 8 字节对齐 */
  . = ALIGN(8);
  _heap_start = .;

  /* 栈指针初始地址为SRAM末尾，低地址方向压栈 */
  _stack_pointer = ORIGIN(SRAM) + LENGTH(SRAM);
  /* 栈区4k空间，超过则栈溢出 */
  _stack_top = _stack_pointer - 0x1000;

  /DISCARD/ : { *(.note*) *(.eh_frame*) *(.comment) }
}
