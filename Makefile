BIN     = Browsy
TOOLCHAIN = /opt/Retro68-build/toolchain
ARCH    = m68k-unknown-elf
CC      = $(TOOLCHAIN)/bin/$(ARCH)-gcc
LD      = $(TOOLCHAIN)/bin/$(ARCH)-g++
AS      = $(TOOLCHAIN)/bin/$(ARCH)-as
AR      = $(TOOLCHAIN)/bin/$(ARCH)-ar
MAKE_APPL = $(TOOLCHAIN)/bin/MakeAPPL
FROM_HEX= xxd -r -ps
CSRC    = $(wildcard src/*.c src/**/*.c)
INC     = $(wildcard src/*.h src/**/*.h)
OBJ     = $(CSRC:.c=.o)
CDEP    = $(CSRC:.c=.d)
SHAREDIR= Shared
DEP_DIR = dep
LIB_DIR = lib
DEPS    = http_parser cstreams
LIBS    = $(DEPS:%=$(LIB_DIR)/lib%.a)
LIBS_L  = $(DEPS:%=-l%)
CFLAGS  = -MMD
CFLAGS += -O3 -DNDEBUG -std=c11
CFLAGS += -Wno-multichar -Wno-attributes -Werror
CFLAGS += -Isrc -Idep/http-parser -Idep/c-streams/src
LDFLAGS = -L$(LIB_DIR) -lretrocrt $(LIBS_L) -Wl,-elf2flt -Wl,-q -Wl,-Map=linkmap.txt -Wl,-undefined=consolewrite -Wl,-gc-sections
SFLAGS  =

RSRC_HEX=$(wildcard rsrc/*/*.hex)
RSRC_TXT=$(wildcard rsrc/*/*.txt)
RSRC_DAT=$(RSRC_HEX:.hex=.dat) $(RSRC_TXT:.txt=.dat)

MINI_VMAC_DIR=~/Mac/Emulation/Mini\ vMac
MINI_VMAC=$(MINI_VMAC_DIR)/Mini\ vMac
MINI_VMAC_LAUNCHER_DISK=$(MINI_VMAC_DIR)/launcher-sys.dsk

ifndef V
	QUIET_CC   = @echo ' CC   ' $<;
	QUIET_AS   = @echo ' AS   ' $<;
	QUIET_LINK = @echo ' LINK ' $@;
	QUIET_APPL = @echo ' APPL ' $*;
	QUIET_RSRC = @echo ' RSRC ' $@;
	QUIET_RUN  = @echo ' RUN  ' $<;
endif

# Main

all: $(BIN).bin

-include $(CDEP)

$(BIN).68k: $(OBJ) $(LIBS)
	$(QUIET_LINK)$(LD) -o $@ $^ $(LDFLAGS)

%.dsk %.bin %.APPL: %.68k rsrc-args
	$(QUIET_APPL)$(MAKE_APPL) -c $< -o $* -C WWW6 $(shell cat rsrc-args)

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(QUIET_AS)$(AS) $(SFLAGS) -o $@ $<

# Resources

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

# Dependencies

deps: $(LIBS) $(LIB_DIR)

$(LIB_DIR)/libhttp_parser.a: $(DEP_DIR)/http-parser/libhttp_parser.a $(LIB_DIR)
	cp $< $@

$(LIB_DIR)/libcstreams.a: $(DEP_DIR)/c-streams/libcstreams.a $(LIB_DIR)
	cp $< $@

$(DEP_DIR)/http-parser/libhttp_parser.a: $(DEP_DIR)/http-parser
	cd $< && make package CC=$(CC) AR=$(AR)

$(DEP_DIR)/c-streams/libcstreams.a: $(DEP_DIR)/c-streams
	cd $< && make libcstreams.a

$(DEP_DIR)/http-parser: $(DEP_DIR)
	test -e $@ || git clone https://github.com/joyent/http-parser $@

$(DEP_DIR)/c-streams: $(DEP_DIR)
	@#git clone https://github.com/clehner/c-streams $@
	test -e $@ || git clone ../c-streams $@

$(DEP_DIR) $(LIB_DIR):
	mkdir -p $@

# Running

run: $(BIN).dsk
	$(QUIET_RUN)$(MINI_VMAC) $(MINI_VMAC_LAUNCHER_DISK) $(DISK) $(BIN).dsk

share: $(SHAREDIR)/$(BIN)

$(SHAREDIR)/$(BIN): $(BIN).APPL
	cp $(BIN).APPL $(SHAREDIR)/$(BIN)
	@cp .rsrc/$(BIN).APPL $(SHAREDIR)/.rsrc/$(BIN)
	@cp .finf/$(BIN).APPL $(SHAREDIR)/.finf/$(BIN)

run-basiliskii: share
	ps aux | grep -v grep | grep BasiliskII -s || BasiliskII &

# Misc

wc:
	@wc -l $(CSRC) $(INC) | sort -n

clean:
	rm -f $(BIN) $(BIN).dsk $(BIN).bin $(BIN).68k $(BIN).68k.gdb \
		$(OBJ) $(CDEP) rsrc/*/*.dat rsrc-args linkmap.txt $(LIBS)

.PHONY: clean wc run
