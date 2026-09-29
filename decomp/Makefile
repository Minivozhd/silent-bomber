# Silent Bomber (SLUS_009.02) matching build
#
#   make split    - (re)generate asm/ + linkers/ from configs/USA/main.yaml
#   make          - assemble, link and verify sha1 against the retail EXE
#   make clean    - remove build/
#
# Requires: mipsel-linux-gnu binutils (brew install mipsel-linux-gnu-binutils
# on macOS, binutils-mipsel-linux-gnu on Debian/Ubuntu) and a python with
# splat64 (pip install splat64). Override the python with
# `make PYTHON=/path/to/venv/bin/python3`.

CROSS   ?= mipsel-linux-gnu-
AS      := $(CROSS)as
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy

# Prefer an in-repo venv, fall back to whatever python3 is on PATH.
ifneq ($(wildcard ../venv/bin/python3),)
PYTHON  ?= ../venv/bin/python3
else
PYTHON  ?= python3
endif

VERSION     := USA
CONFIG      := configs/$(VERSION)/main.yaml
BUILD       := build/$(VERSION)
LD_SCRIPT   := linkers/$(VERSION)/main.ld
UNDEF_SYMS  := linkers/$(VERSION)/undefined_syms_auto.main.txt
UNDEF_FUNCS := linkers/$(VERSION)/undefined_funcs_auto.main.txt
ELF         := $(BUILD)/main.elf
OUT         := $(BUILD)/out/SLUS_009.02
SHA1        := daf6c9b6771c724928aa8d158f973b181d7b8115

ASFLAGS := -EL -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude
LDFLAGS := -EL -nostdlib --no-check-sections -Map $(BUILD)/main.map

ASM_S := $(shell find asm/$(VERSION)/main -name '*.s' 2>/dev/null)
OBJS  := $(patsubst %.s,$(BUILD)/%.s.o,$(ASM_S))

.PHONY: all check split clean tools

all: check

# --- split -----------------------------------------------------------------

split:
	$(PYTHON) -m splat split $(CONFIG)

$(LD_SCRIPT) $(UNDEF_SYMS) $(UNDEF_FUNCS):
	$(PYTHON) -m splat split $(CONFIG)

# --- assemble ---------------------------------------------------------------

$(BUILD)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# --- link + extract ----------------------------------------------------------

$(ELF): $(OBJS) $(LD_SCRIPT) $(UNDEF_SYMS) $(UNDEF_FUNCS)
	$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -T $(UNDEF_SYMS) -T $(UNDEF_FUNCS) -o $@

$(OUT): $(ELF)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $< $@

# --- verify ------------------------------------------------------------------

check: $(OUT)
	@echo "$(SHA1)  $(OUT)" | sha1sum -c - && echo "OK: $(OUT) matches retail SLUS_009.02"

clean:
	rm -rf build
