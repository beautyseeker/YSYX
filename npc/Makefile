MODULE = example
TOPNAME = top_$(MODULE)
NXDC_FILES = constr/$(MODULE).nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc -sv \
				-O3 --x-assign fast --x-initial fast --noassert --trace -D

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
WAVE_DIR = $(BUILD_DIR)/waveforms
BIN = $(BUILD_DIR)/$(TOPNAME)
SIM_BIN = $(BUILD_DIR)/sim_$(MODULE)
CPP_TB = sim/sim_$(MODULE).cpp

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR)/waveforms)

# constraint file
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/$(TOPNAME)_auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
# VSRCS = $(shell find $(abspath ./vsrc) -name "$(TOPNAME).v" -o -name "$(TOPNAME).sv")
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v" -o -name "*.sv")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
override CSRCS += $(SRC_AUTO_BIND)


# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))

# 修改如下：
# 1. -DTOP_NAME=V$(TOPNAME)  -> 定义类名为 VALU (注意没有转义引号，它是代码中的类型)
# 2. -DTOP_HEADER="\"V$(TOPNAME).h\"" -> 定义引用路径为 "VALU.h"
CXXFLAGS += $(INCFLAGS) -DTOP_NAME=V$(TOPNAME) -DTOP_HEADER="\"V$(TOPNAME).h\""
CXXFLAGS += -g

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))


$(SIM_BIN): $(VSRCS) $(CPP_TB)
	@rm -rf $(OBJ_DIR)/sim_trace
	$(VERILATOR) $(VERILATOR_CFLAGS) --top-module $(TOPNAME) --trace \
	--Mdir $(OBJ_DIR)/sim_trace --exe $(CPP_TB) \
	$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
	-o $(abspath $(SIM_BIN)) $^

run: $(BIN)
	@$^

clean:
	rm -rf $(BUILD_DIR)


all: default
	@echo "Write this Makefile by your self."

VCD_FILE ?= $(WAVE_DIR)/$(TOPNAME).vcd
sim: $(SIM_BIN)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	@$^
# 	$(VERILATOR) $(VERILATOR_CFLAGS) --top-module $(TOPNAME) --trace \
# 	--Mdir $(OBJ_DIR)/sim_trace --exe $(CPP_TB) \
# 	$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
# 	-o $(BUILD_DIR)/sim_$(MODULE)

show:$(VCD_FILE)
	gtkwave $(VCD_FILE)

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
