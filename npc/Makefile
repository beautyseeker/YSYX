# =============================================================================
# NPC 构建脚本（精简版）
# 用法: make -f Makefile.npc <target>
#
# 常用:
#   make -f Makefile.npc sim IMAGE=xxx.hex BATCH=1   # 编译并跑 NPC 仿真
#   make -f Makefile.npc wave                         # gtkwave 看波形
#   make -f Makefile.npc board MODULE=ALU             # NVBoard 上板
#   make -f Makefile.npc clean
# =============================================================================

.DEFAULT_GOAL := help

# ---------- 可配置 ----------
# 以本 Makefile 所在目录为工程根，避免空的 NPC_HOME 环境变量干扰
NPC_ROOT    := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

MODULE   ?= ysyxSoCFull
# Verilator --top-module
TOP      ?= ysyxSoCFull
IMAGE    ?=
BATCH    ?=
CYCLES   ?= 10000

BUILD_DIR ?= $(NPC_ROOT)/build
OBJ_DIR   ?= $(BUILD_DIR)/obj
WAVE_DIR  ?= $(BUILD_DIR)/wave
VCD_FILE  ?= $(BUILD_DIR)/wave/$(TOP).vcd

VSRC_DIR    := $(NPC_ROOT)/vsrc/miniRV
VSRC_DIR    += $(NPC_ROOT)/../ysyxSoC/perip $(NPC_ROOT)/../ysyxSoC/build/
SIM_DIR     := $(NPC_ROOT)/sim/miniRV
CONSTR_DIR  := $(NPC_ROOT)/constr
CSRC_DIR    := $(NPC_ROOT)/csrc

SIM_BIN     := $(BUILD_DIR)/sim_$(MODULE)
BOARD_BIN   := $(BUILD_DIR)/board_$(MODULE)

VERILATOR   ?= verilator
GTKWAVE     ?= gtkwave

# ---------- 公共 RTL ----------
# 排除旧仿真壳及其本地外设（SoC 已提供存储器/UART）
PKG_SV := $(wildcard $(NPC_ROOT)/include/defs_pkg.sv)
VSRCS_RAW := $(PKG_SV) $(shell find $(VSRC_DIR) -name '*.sv' -o -name '*.v' 2>/dev/null | sort)
VSRCS_EXCLUDE := TopMiniRV.sv RAM.sv UART.sv Xbar.sv SimpleBus_if.sv
VSRCS := $(filter-out $(addprefix %/,$(VSRCS_EXCLUDE)),$(VSRCS_RAW))

# ---------- Verilator 公共选项 ----------
# Verilog `include 必须用 Verilator 的 -I，不能只写在 -CFLAGS 里
VERILATOR_FLAGS = -MMD --build -cc -sv \
	-I$(NPC_ROOT)/include \
	-I$(NPC_ROOT)/../ysyxSoC/perip/uart16550/rtl \
	-I$(NPC_ROOT)/../ysyxSoC/perip/spi/rtl \
	-O3 --x-assign fast --x-initial fast \
	--timescale 1ns/1ns \
	--no-timing \
	--autoflush \
	--top-module $(TOP) \
	--Mdir $(OBJ_DIR)/$(MODULE)

# 波形：make TRACE=1 开启 --trace（注意磁盘；默认关闭以免 VCD 撑爆）
VERILATOR_FLAGS += $(if $(filter 1,$(TRACE)),--trace,)

# 仿真程序运行所依赖的头文件目录
INC_PATH  := $(NPC_ROOT)/sim/miniRV/include
INC_PATH  += $(NEMU_HOME)/include/generated
INC_PATH  += $(NVBOARD_HOME)/usr/include
INCFLAGS  := $(addprefix -I,$(INC_PATH))

CXXFLAGS_COMMON := $(INCFLAGS) \
	-I$(OBJ_DIR)/$(MODULE) \
	-DTOP_NAME=V$(TOP) \
	-DTOP_HEADER=V$(TOP).h \
	-DTOP_ROOT_HEADER=V$(TOP)___024root.h \
	$(if $(filter 1,$(TRACE)),-DCONFIG_WAVE,)  \
	-g

# ysyx 提交追踪（保留，勿删 sim 里的 git_commit）
-include $(NPC_ROOT)/../Makefile

# =============================================================================
# NVBoard 上板相关配置
# =============================================================================

include $(NVBOARD_HOME)/scripts/nvboard.mk

NXDC        := $(CONSTR_DIR)/$(MODULE).nxdc
AUTO_BIND   := $(BUILD_DIR)/$(TOP)_auto_bind.cpp
BOARD_MAIN  := $(wildcard $(CSRC_DIR)/$(MODULE).cpp)
BOARD_SRCS  := $(BOARD_MAIN) $(AUTO_BIND)

# CXXFLAGS / LDFLAGS 由 nvboard.mk 注入 SDL2 等
$(AUTO_BIND): $(NXDC)
	@mkdir -p $(dir $@)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $< $@

# =============================================================================
# 仿真程序构建列表
# =============================================================================
SIM_MAIN   := $(SIM_DIR)/sim_$(TOP).cpp
SIM_SRCS := $(shell find $(SIM_DIR)/src -name '*.cpp' -o -name '*.c' 2>/dev/null | sort)
SIM_SRCS := $(filter-out %/npc_sim_main.cpp,$(SIM_SRCS))
# nvboard.mk 注入的 LDFLAGS（SDL2 等）也要链上；NVBOARD_ARCHIVE 提供 nvboard_* 实现
LDFLAGS_SIM := -lcapstone -lreadline -lhistory -ldl $(LDFLAGS)

$(SIM_BIN): $(VSRCS) $(SIM_MAIN) $(SIM_SRCS) $(AUTO_BIND) $(NVBOARD_ARCHIVE)
	@mkdir -p $(WAVE_DIR) $(OBJ_DIR)/$(MODULE) $(dir $@)
	$(VERILATOR) $(VERILATOR_FLAGS) \
		--exe $(SIM_MAIN) $(SIM_SRCS) $(AUTO_BIND) $(NVBOARD_ARCHIVE) \
		$(addprefix -CFLAGS ,$(CXXFLAGS_COMMON)) \
		$(addprefix -LDFLAGS ,$(LDFLAGS_SIM)) \
		-o $(abspath $@) \
		$(VSRCS)
	@echo "[OK] sim binary -> $@"

.PHONY: sim run

# =============================================================================
# 仿真程序启动参数
# =============================================================================
DIFF_SO  ?=
SIM_ARGS  = $(IMAGE)
SIM_ARGS += $(if $(DIFF_SO),$(if $(wildcard $(DIFF_SO)),--diff=$(DIFF_SO),),)
SIM_ARGS += $(if $(BATCH),--batch,)

sim run: $(SIM_BIN)
	$(call git_commit,"sim RTL")
	@test -n "$(IMAGE)" || (echo "ERROR: set IMAGE=path/to/image.hex"; exit 1)
	VCD_FILE=$(VCD_FILE) $(abspath $(SIM_BIN)) $(SIM_ARGS)

# =============================================================================
# 波形
# =============================================================================
.PHONY: wave show
wave show:
	@test -f "$(VCD_FILE)" || (echo "ERROR: no wave file: $(VCD_FILE)"; exit 1)
	$(GTKWAVE) $(VCD_FILE)

# =============================================================================
# 杂项
# =============================================================================
.PHONY: clean help
clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "NPC Makefile.npc targets:"
	@echo "  sim/run   IMAGE=<hex> [BATCH=1] [MODULE=TopMiniRV]"
	@echo "  wave      [VCD_FILE=path]     # gtkwave"
	@echo "  board     MODULE=<name> CATEGORY=digital"
	@echo "  clean"
	@echo ""
	@echo "Examples:"
	@echo "  make -f Makefile.npc sim IMAGE=../am-kernels/tests/cpu-tests/build/dummy-riscv32e-npc.hex BATCH=1"
	@echo "  make -f Makefile.npc wave"
	@echo "  make -f Makefile.npc board MODULE=prioty8_3_encoder CATEGORY=digital"
