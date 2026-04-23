MODULE = example
TOPNAME = top_$(MODULE)
NXDC_FILES = $(CONSTR_DIR)/$(MODULE).nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc -sv -I$(abspath $(INCLUDE_DIR)) \
				-O3 --x-assign fast --x-initial fast --trace -D

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
WAVE_DIR = $(BUILD_DIR)/waveforms

BOARD_BIN = $(BUILD_DIR)/board_$(TOPNAME)
SIM_BIN = $(BUILD_DIR)/sim_$(MODULE)

NEW ?=
CATEGORY_DIR ?= miniRV
INCLUDE_DIR ?= $(if $(CATEGORY_DIR),include/$(CATEGORY_DIR),include)
SIM_DIR ?= $(if $(CATEGORY_DIR),sim/$(CATEGORY_DIR),sim)
VSRCS_DIR ?= $(if $(CATEGORY_DIR),vsrc/$(CATEGORY_DIR),vsrc)
CONSTR_DIR ?= $(if $(CATEGORY_DIR),constr/$(CATEGORY_DIR),constr)

new:
	if [ -z "$(NEW)" ]; then \
		echo "Please provide a name for the new module using 'make new NEW=your_module'"; \
		exit 1; \
	fi
	@mkdir -p $(CONSTR_DIR) $(SIM_DIR) $(VSRCS_DIR) $(INCLUDE_DIR)
	@touch $(CONSTR_DIR)/$(NEW).nxdc
	@touch $(SIM_DIR)/sim_$(NEW).cpp
	@touch $(VSRCS_DIR)/$(NEW).sv
	@echo "Created $(CONSTR_DIR)/$(NEW).nxdc, \
	$(SIM_DIR)/sim_$(NEW).cpp, $(VSRCS_DIR)/$(NEW).sv"	
.PHONY: new


default: $(BOARD_BIN)

$(shell mkdir -p $(BUILD_DIR)/waveforms)
# OBJ_DIR为空则创建
$(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR))

# constraint file
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/$(TOPNAME)_auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
# VSRCS = $(shell find $(abspath ./vsrc) -name "$(TOPNAME).v" -o -name "$(TOPNAME).sv")
VSRCS = $(abspath ./include/defs_pkg.sv) $(shell find $(abspath $(VSRCS_DIR)) -name "*.v" -o -name "*.sv")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
override CSRCS += $(SRC_AUTO_BIND)
SDB_SRCS = $(shell find $(abspath ./sim/miniRV) -name "*.c" -or -name "*.cpp")


CPP_TB := $(if $(wildcard $(SIM_DIR)/sim_$(MODULE).cpp),\
              $(SIM_DIR)/sim_$(MODULE).cpp,\
              $(firstword $(wildcard ./sim/**/sim_$(MODULE).cpp)))


# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk

INC_PATH += $(NPC_HOME)/sim/miniRV/include $(NEMU_HOME)/include/generated/
# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))

# 修改如下：
# 1. -DTOP_NAME=V$(TOPNAME)  -> 定义类名为 VALU (注意没有转义引号，它是代码中的类型)
# 2. -DTOP_HEADER="\"V$(TOPNAME).h\"" -> 定义引用路径为 "VALU.h"
CYCLES ?= 10000
CXXFLAGS += $(INCFLAGS) -DTOP_NAME=V$(TOPNAME) -DTOP_HEADER="\"V$(TOPNAME).h\""
CXXFLAGS += -g -DSIM_CYCLES=$(CYCLES)
LDFLAGS += -lcapstone -lreadline -lhistory -ldl

$(BOARD_BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BOARD_BIN))


$(SIM_BIN): $(VSRCS) $(CPP_TB)
	$(VERILATOR) $(VERILATOR_CFLAGS) --top-module $(TOPNAME) \
	--Mdir $(OBJ_DIR)/sim_trace --exe $(CPP_TB) $(SDB_SRCS) \
	$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
	-o $(abspath $(SIM_BIN)) $^


VCD_FILE ?= $(WAVE_DIR)/$(TOPNAME).vcd

DIFF_ARGS = --diff=$(NEMU_HOME)/build/riscv32-nemu-interpreter-so
BATCH ?= 
SDB_ARGS = $(IMAGE) $(DIFF_ARGS) $(if $(BATCH),--batch,)

# endif
sim: $(SIM_BIN)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	$(abspath $(SIM_BIN)) $(SDB_ARGS)
# 	@echo "Simulation finished. Waveform saved to: $(VCD_FILE)"


show:$(SIM_BIN)
	VCD_FILE=$(VCD_FILE) $^
	@echo "Launching GTKWave to view waveform: $(VCD_FILE)"
	gtkwave $(VCD_FILE)

board: $(BOARD_BIN)
	$(BOARD_BIN)


clean:
	rm -rf $(BUILD_DIR)


all: default
	@echo "Write this Makefile by your self."


include ../Makefile


# rules for Yosys+iEDA STA
PROJ_PATH = ../yosys-sta
SHELL := /bin/bash


RPT_DIR ?= ./report
DESIGN ?= ${MODULE}
SDC_FILE ?= $(PROJ_PATH)/scripts/default.sdc
RTL_FILES ?= $(shell find ./vsrc -name "*.v" -o -name "*.sv")
export CLK_FREQ_MHZ ?= 100
export CLK_PORT_NAME ?= clk
PDK = icsprout55

RESULT_DIR = $(RPT_DIR)/$(DESIGN)-$(CLK_FREQ_MHZ)MHz
SCRIPT_DIR = $(PROJ_PATH)/scripts
NETLIST_SYN_V   = $(RESULT_DIR)/$(DESIGN).netlist.v
TIMING_RPT = $(RESULT_DIR)/$(DESIGN).rpt

init:
	bash -c "$$(wget -O - https://ysyx.oscc.cc/slides/resources/scripts/init-yosys-sta.sh)"
	mkdir -p pdk
	cd pdk && git clone -b ysyx --depth 1 git@github.com:openecos-projects/icsprout55-pdk.git icsprout55

syn: $(NETLIST_SYN_V)
$(NETLIST_SYN_V): $(RTL_FILES) $(SCRIPT_DIR)/yosys.tcl
	mkdir -p $(@D)
	echo tcl $(SCRIPT_DIR)/yosys.tcl $(DESIGN) $(PDK) \"$(RTL_FILES)\" $@ | yosys -g -l $(@D)/yosys.log -s -

sta: $(TIMING_RPT)
$(TIMING_RPT): $(SCRIPT_DIR)/sta.tcl $(SDC_FILE) $(NETLIST_SYN_V)
	set -o pipefail && $(PROJ_PATH)/bin/iEDA -script $^ $(DESIGN) $(PDK) 2>&1 | tee $(RESULT_DIR)/sta.log

clean_sta:
	-rm -rf $(TIMING_RPT)

clean_syn:
	-rm -rf $(NETLIST_SYN_V)

clean_all:
	-rm -rf $(RPT_DIR)


.PHONY: default all clean \
run sim show init syn sta \
clean_sta clean_syn clean_all \
new