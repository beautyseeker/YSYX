AM_SRCS := riscv/SoC/start.S \
           riscv/SoC/trm.c \
           riscv/SoC/ioe.c \
           riscv/SoC/timer.c \
           riscv/SoC/input.c \
           riscv/SoC/cte.c \
           riscv/SoC/trap.S \

CFLAGS    += -fdata-sections -ffunction-sections -O2
LDSCRIPTS += $(AM_HOME)/scripts/SoC.ld
LDFLAGS   += --gc-sections -e _start

MAINARGS_MAX_LEN = 64
MAINARGS_PLACEHOLDER = the_insert-arg_rule_in_Makefile_will_insert_mainargs_here
CFLAGS += -DMAINARGS_MAX_LEN=$(MAINARGS_MAX_LEN) -DMAINARGS_PLACEHOLDER=$(MAINARGS_PLACEHOLDER)

insert-arg: image
	@python $(AM_HOME)/tools/insert-arg.py $(IMAGE).bin $(MAINARGS_MAX_LEN) $(MAINARGS_PLACEHOLDER) "$(mainargs)"

image: image-dep
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin

run: insert-arg
	$(MAKE) -C $(NPC_HOME) IMAGE=$(IMAGE).bin sim

.PHONY: insert-arg
