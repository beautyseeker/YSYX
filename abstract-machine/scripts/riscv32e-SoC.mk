include $(AM_HOME)/scripts/isa/riscv.mk
include $(AM_HOME)/scripts/platform/SoC.mk
COMMON_CFLAGS += -march=rv32e_zicsr -mabi=ilp32e  # overwrite
LDFLAGS       += -melf32lriscv                    # overwrite

AM_SRCS += riscv/SoC/libgcc/div.S \
           riscv/SoC/libgcc/muldi3.S \
           riscv/SoC/libgcc/multi3.c \
           riscv/SoC/libgcc/ashldi3.c \
           riscv/SoC/libgcc/unused.c
