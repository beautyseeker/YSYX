.section entry, "ax"
.globl _start
.type _start, @function

_start:
  mv s0, zero
  /* SRAM 与 MROM 相距过远，不能用 PC 相对的 la；改用绝对地址 lui+addi */
  lui sp, %hi(_stack_pointer)
  addi sp, sp, %lo(_stack_pointer)
  /* Flash 小 stub → 拷 SRAM loader → 再由 loader 搬 .text/.data 到 PSRAM */
  call _flash_boot
1:
  j 1b

.size _start, . - _start
