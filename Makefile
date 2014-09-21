TOOLCHAIN = /opt/Retro68-build/toolchain
ARCH    = m68k-unknown-elf

BIN     = Browsy
CC      = $(TOOLCHAIN)/bin/$(ARCH)-gcc
LD      = $(TOOLCHAIN)/bin/$(ARCH)-g++
MAKE_APPL = $(TOOLCHAIN)/bin/MakeAPPL
SRC     = $(wildcard src/*.c)
INC     = $(wildcard src/*.h)
OBJ     = $(SRC:.c=.o)
DEP     = $(SRC:.c=.d)
CFLAGS  = -MMD
CFLAGS += -O3 -DNDEBUG
CFLAGS += -Wno-multichar -Wno-attributes -Wno-unused-function
LDFLAGS = -lretrocrt -Wl,-elf2flt -Wl,-q -Wl,-Map=linkmap.txt -Wl,-undefined=consolewrite

ifndef V
	QUIET_CC   = @echo ' CC   ' $<;
	QUIET_LINK = @echo ' LINK ' $@;
	QUIET_APPL = @echo ' APPL ' $@;
endif

all: $(BIN).bin

-include $(DEP)

$(BIN): $(OBJ)
	$(QUIET_LINK)$(LD) -o $@ $^ $(LDFLAGS)

%.dsk %.bin: %
	$(QUIET_APPL)$(MAKE_APPL) -c $< -o $*

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

wc:
	@wc -l $(SRC) $(INC) | sort -n

clean:
	rm -f $(BIN) $(BIN).dsk $(BIN).bin $(OBJ) $(DEP) linkmap.txt

.PHONY: clean wc
