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

# --- C pipeline (GCC 2.8.1-psx + ASPSX 2.77, -O2 -G8; cpp -> cc1 -> maspsx) --
# See tools/fingerprint/README.md for how this combination was identified.
# maspsx runs WITHOUT --expand-div (the retail divs are trap-free).
CC1     := tools/compilers/macos/gcc-2.8.1-psx/cc1
MASPSX  := tools/maspsx/maspsx.py
CC1FLAGS := -O2 -G8 -mips1 -w -funsigned-char -fpeephole -ffunction-cse \
            -fpcc-struct-return -fcommon -msoft-float -mgas -fgnu-linker -quiet
CPPFLAGS := -Iinclude -D_LANGUAGE_C
MASPSXFLAGS := --aspsx-version=2.77 --run-assembler --gnu-as-path=$(AS) \
               --dont-force-G0 \
               -EL -O2 -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude -G8

# asm/.../nonmatchings/ is compiled into the .c objects via INCLUDE_ASM,
# never assembled directly.
ASM_S := $(shell find asm/$(VERSION)/main -name '*.s' -not -path '*/nonmatchings/*' 2>/dev/null)
SRC_C := $(shell find src/main -name '*.c' 2>/dev/null)
OBJS  := $(patsubst %.s,$(BUILD)/%.s.o,$(ASM_S)) \
         $(patsubst %.c,$(BUILD)/%.c.o,$(SRC_C))

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

# --- compile C (cpp -> cc1 -> maspsx+as) ---------------------------------------

# cc1 2.x segfaults on comments; always feed it preprocessed source.
$(BUILD)/%.c.i: %.c
	@mkdir -p $(dir $@)
	clang -E -P -xc $(CPPFLAGS) -o $@ $<

$(BUILD)/%.c.s: $(BUILD)/%.c.i
	$(CC1) $(CC1FLAGS) -o $@ $<

$(BUILD)/%.c.o: $(BUILD)/%.c.s
	$(PYTHON) $(MASPSX) $(MASPSXFLAGS) -o $@ $<

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
