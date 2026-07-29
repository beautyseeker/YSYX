# 裸机小程序：编译为 .elf / .bin（不走 AM / SoC.ld）
# 用法:
#   make -f mrom.mk              # 用本 Makefile 所在目录下第一个 .c
#   make -f mrom.mk SRC=foo.c    # 指定源文件
#   make -f mrom.mk TEXT=0x0f000000  # 可选：改链接起始地址（默认 0）

GCC     ?= riscv64-linux-gnu-gcc
OBJCOPY ?= riscv64-linux-gnu-objcopy
OBJDUMP ?= riscv64-linux-gnu-objdump

# Makefile 所在目录（与当前工作目录无关）
MK_DIR  := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
C_FILES := $(sort $(wildcard $(MK_DIR)*.c))

# SRC 可命令行覆盖；未指定则取目录内字典序第一个 .c
ifeq ($(origin SRC),command line)
  # 用户已指定 SRC=
else
  ifeq ($(C_FILES),)
    $(error 在 $(MK_DIR) 下找不到 .c 文件，请指定源文件: make -f mrom.mk SRC=your.c)
  endif
  SRC := $(firstword $(C_FILES))
endif

# 若 SRC 是相对路径，优先相对 MK_DIR，否则相对当前目录
ifeq ($(wildcard $(SRC)),)
  ifneq ($(wildcard $(MK_DIR)$(SRC)),)
    SRC := $(MK_DIR)$(SRC)
  else
    $(error 找不到源文件 '$(SRC)'，请指定存在的 .c: make -f mrom.mk SRC=your.c)
  endif
endif

OUT      ?= $(basename $(notdir $(SRC)))
TEXT     ?= 0x0
BUILD_DIR ?= $(MK_DIR)
ELF      := $(BUILD_DIR)$(OUT).elf
BIN      := $(BUILD_DIR)$(OUT).bin
TXT      := $(BUILD_DIR)$(OUT).txt

# -O2：避免 -O0 栈帧（sp 未初始化会卡死在非法 store）
CFLAGS := -O2 -march=rv32e_zicsr -mabi=ilp32e \
	-static -fno-builtin -nostdlib -nostartfiles -ffreestanding \
	-Wl,--build-id=none -fno-asynchronous-unwind-tables \
	-Wl,-e,_start,-Ttext=$(TEXT)

.PHONY: all clean info
all: info $(BIN)

info:
	@echo "SRC=$(SRC)"
	@echo "OUT=$(OUT)  TEXT=$(TEXT)"
	@echo "-> $(BIN)"

# 讲义：抽取 .text 到 .bin（裸机载荷通常只有代码）
$(BIN): $(ELF)
	$(OBJCOPY) -j .text -O binary $< $@
	@ls -l $@

$(ELF): $(SRC)
	$(GCC) $(CFLAGS) -o $@ $<
	@$(OBJDUMP) -d $@ > $(TXT)

clean:
	rm -f $(ELF) $(BIN) $(TXT)
