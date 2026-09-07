# Copyright (c) 2026 David Lucius Severus
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# micron on bare metal
#
#   make                     build the i386 image and run the object gate
#   make run                 boot it and grade the result
#   make ARCH=amd64 run      ... and the same for every other target
#   make all-images          build + gate all five, which is what a sweep wants
#
# ARCH is one of i386 amd64 arm32 arm64 stm32

ARCH  ?= i386
CPU   ?=
POOL  ?=
DUCK  ?= $(firstword $(wildcard ../../bin/duck) $(shell command -v duck 2>/dev/null))
OUT   ?= bin

MICRON := ../..
START  := $(MICRON)/start

IMAGE  := $(OUT)/metal_image_$(ARCH)
BOARDO := $(OUT)/board_$(ARCH).o

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# per-target: the duck arch, the board file, the demo, the pool, and how to boot it

ifeq ($(ARCH),i386)
  # --isa base, not duck's -march=native default: a board image must not carry a claim about the
  # machine that BUILT it. The qemu side compensates with -cpu max; a real board cannot.
  DUCK_ARCH   := --i386 --isa base
  BOARD       := board_pc.cpp
  DEMO        := metal_demo.cpp
  POOL_DEF    ?= 8388608
  QEMU        := qemu-system-i386
  QEMU_CPU    ?= max
  QEMU_ARGS   := -cpu $(QEMU_CPU) -kernel $(IMAGE) \
                 -device isa-debug-exit,iobase=0xf4,iosize=0x04
  # isa-debug-exit reports (value << 1) | 1, and board_pc.cpp writes 0 for micron's sentinel
  PASS_CODES  := 1 3
else ifeq ($(ARCH),amd64)
  DUCK_ARCH   := --x86 --isa base
  BOARD       := board_pc.cpp
  DEMO        := metal_demo.cpp
  POOL_DEF    ?= 8388608
  QEMU        := qemu-system-x86_64
  QEMU_CPU    ?= max
  QEMU_ARGS   := -cpu $(QEMU_CPU) -kernel $(IMAGE) \
                 -device isa-debug-exit,iobase=0xf4,iosize=0x04
  PASS_CODES  := 1 3
else ifeq ($(ARCH),arm32)
  DUCK_ARCH   := --arm
  BOARD       := board_virt.cpp
  DEMO        := metal_demo.cpp
  POOL_DEF    ?= 8388608
  QEMU        := qemu-system-arm
  QEMU_CPU    ?= cortex-a15
  QEMU_ARGS   := -M virt -cpu $(QEMU_CPU) -kernel $(IMAGE) -semihosting-config enable=on,target=native
  PASS_CODES  := 1
else ifeq ($(ARCH),arm64)
  DUCK_ARCH   := --arm64
  BOARD       := board_virt.cpp
  DEMO        := metal_demo.cpp
  POOL_DEF    ?= 8388608
  QEMU        := qemu-system-aarch64
  QEMU_CPU    ?= cortex-a57
  QEMU_ARGS   := -M virt -cpu $(QEMU_CPU) -kernel $(IMAGE) -semihosting-config enable=on,target=native
  PASS_CODES  := 1
else ifeq ($(ARCH),stm32)
  MCU         ?= cortex-m4
  DUCK_ARCH   := --cortex-m $(MCU) --minsize
  BOARD       := board_stm32.cpp
  DEMO        := metal_stm32_demo.cpp
  # 48 KiB out of the part's 192 KiB, leaving the 8 KiB stack and the image's own data
  POOL_DEF    ?= 49152
  QEMU        := qemu-system-arm
  # netduinoplus2 is an STM32F405 (M4, 192K SRAM); netduino2 is an F205 (M3, 128K)
  MACHINE     ?= netduinoplus2
  QEMU_ARGS   := -M $(MACHINE) -kernel $(IMAGE) -semihosting-config enable=on,target=native
  PASS_CODES  := 1
else
  $(error ARCH must be one of i386 amd64 arm32 arm64 stm32)
endif

POOL := $(if $(POOL),$(POOL),$(POOL_DEF))

GATE_ARCH := $(if $(filter stm32,$(ARCH)),cortexm,$(if $(filter lm,$(METAL_ENTRY)),$(ARCH)-lm,$(ARCH)))

METAL_ENTRY_FLAG := $(if $(METAL_ENTRY),--metal-entry $(METAL_ENTRY),)

DUCKFLAGS := --metal $(METAL_ENTRY_FLAG) --start $(START) -i $(MICRON) -i $(MICRON)/src \
             --def MICRON_BB_PORT_POOL=$(POOL) -o $(OUT)

.PHONY: all image check run clean help all-images

all: image check

image:
	@test -n "$(DUCK)" || { echo "no duck on PATH and none at ../../bin/duck -- see BAREBONES.md"; exit 1; }
	@mkdir -p $(OUT)
	$(DUCK) compile $(BOARD) $(DUCK_ARCH) $(DUCKFLAGS) --raw-obj
	@mv -f $(OUT)/$(basename $(BOARD)).o $(BOARDO)
	$(DUCK) build $(DEMO) $(BOARDO) $(notdir $(IMAGE)) $(DUCK_ARCH) $(DUCKFLAGS)

check: image
	@sh ../../scripts/check_metal_image.sh $(IMAGE) $(GATE_ARCH)

run: image
	@command -v $(QEMU) >/dev/null || { \
	   echo "$(QEMU) is not installed -- dnf install $(QEMU)"; exit 1; }
	@echo "booting $(IMAGE) on $(QEMU) ..."
	@set +e; \
	 out=$$(timeout 120 $(QEMU) $(QEMU_ARGS) -nographic -no-reboot -serial mon:stdio -display none </dev/null 2>&1); \
	 rc=$$?; \
	 printf '%s\n' "$$out"; \
	 echo "--- qemu exit $$rc ---"; \
	 echo "$$out" | grep -q 'ALL CHECKS PASSED' || { echo "FAIL: the image never printed ALL CHECKS PASSED"; exit 1; }; \
	 for c in $(PASS_CODES); do [ "$$rc" = "$$c" ] && { echo "PASS ($(ARCH), exit $$rc)"; exit 0; }; done; \
	 echo "FAIL: exit $$rc is not one of: $(PASS_CODES)"; exit 1

all-images:
	@for a in i386 amd64 arm32 arm64 stm32; do \
	   echo "=== $$a ==="; \
	   $(MAKE) --no-print-directory ARCH=$$a all || exit 1; \
	 done

clean:
	rm -rf $(OUT)

help:
	@echo "make [ARCH=i386|amd64|arm32|arm64|stm32] [POOL=<bytes>] [METAL_ENTRY=pvh|lm]"
	@echo "  all         build + check the image (default)"
	@echo "  image       build only"
	@echo "  check       the 0-syscall / 0-vector / 0-TLS / 0-undefined / boot-header gate"
	@echo "  run         boot it under qemu and grade the SERIAL OUTPUT, not just the exit code"
	@echo "  all-images  build + gate all five targets"
	@echo ""
	@echo "  ARCH=stm32 also takes MCU=cortex-m4|cortex-m3 and MACHINE=netduinoplus2|netduino2"
	@echo "  ARCH=amd64 also takes METAL_ENTRY=lm, for a board already in long mode (does not boot"
	@echo "             under qemu by design -- there is no loader for that shape here)"
