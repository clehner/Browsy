BIN     = Browsy
TOOLCHAIN = /opt/Retro68-build/toolchain
ARCH    = m68k-unknown-elf
CC      = $(TOOLCHAIN)/bin/$(ARCH)-gcc
LD      = $(TOOLCHAIN)/bin/$(ARCH)-g++
AS      = $(TOOLCHAIN)/bin/$(ARCH)-as
MAKE_APPL = $(TOOLCHAIN)/bin/MakeAPPL
FROM_HEX= xxd -r -ps
CSRC    = $(wildcard src/*.c)
SSRC    = $(wildcard src/*.s)
INC     = $(wildcard src/*.h)
SRC     = $(CSRC) $(SSRC)
OBJ     = $(SRC:.c=.o)
DEP     = $(CSRC:.c=.d)
CFLAGS  = -MMD
CFLAGS += -O3 -DNDEBUG -std=c11
CFLAGS += -Wno-multichar -Wno-attributes -Werror
LDFLAGS = -lretrocrt -Wl,-elf2flt -Wl,-q -Wl,-Map=linkmap.txt -Wl,-undefined=consolewrite
SFLAGS  =

RSRC_HEX=$(wildcard rsrc/*/*.hex)
RSRC_TXT=$(wildcard rsrc/*/*.txt)
RSRC_DAT=$(RSRC_HEX:.hex=.dat) $(RSRC_TXT:.txt=.dat)

MINI_VMAC_DIR=~/Mac/Emulation/Mini\ vMac
MINI_VMAC=$(MINI_VMAC_DIR)/Mini\ vMac
MINI_VMAC_LAUNCHER_DISK=$(MINI_VMAC_DIR)/launcher-sys.dsk

ifndef V
	QUIET_CC   = @echo ' CC   ' $<;
	QUIET_LINK = @echo ' LINK ' $@;
	QUIET_APPL = @echo ' APPL ' $@;
	QUIET_RSRC = @echo ' RSRC ' $@;
	QUIET_RUN = @echo ' RUN  ' $<;
endif

all: $(BIN).bin

-include $(DEP)

$(BIN).68k: $(OBJ)
	$(QUIET_LINK)$(LD) -o $@ $^ $(LDFLAGS)

%.dsk %.bin: %.68k rsrc-args
	$(QUIET_APPL)$(MAKE_APPL) -c $< -o $* $(shell cat rsrc-args)

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(QUIET_AS)$(AS) $(SFLAGS) -o $@ $<

rsrc: $(RSRC_DAT) rsrc-args

rsrc/%.dat: rsrc/%.hex
	$(QUIET_RSRC)$(FROM_HEX) $< > $@

rsrc/TEXT/%.dat: rsrc/TEXT/%.txt
	$(QUIET_RSRC)tr '\n' '\r' < $< > $@

rsrc-args: $(RSRC_DAT)
	@cd rsrc && for code in $$(ls); do \
		echo -n "-t $$code "; \
		cd "$$code" && for file in *.dat; do \
			echo -n "-r $${file%.dat} rsrc/$$code/$$file "; \
		done; \
		cd ..; \
	done > ../$@

wc:
	@wc -l $(SRC) $(INC) | sort -n

run: $(BIN).dsk
	$(QUIET_RUN)$(MINI_VMAC) $(MINI_VMAC_LAUNCHER_DISK) $(DISK) $(BIN).dsk

clean:
	rm -f $(BIN) $(BIN).dsk $(BIN).bin $(BIN).68k $(BIN).68k.gdb \
		$(OBJ) $(DEP) $(RSRC_DAT) rsrc-args linkmap.txt

.PHONY: clean wc run
