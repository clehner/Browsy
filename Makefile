TOOLCHAIN = /opt/Retro68-build/toolchain
ARCH    = m68k-unknown-elf

BIN     = Browsy
CC      = $(TOOLCHAIN)/bin/$(ARCH)-gcc
SRC     = $(wildcard src/*.c)
INC     = $(wildcard src/*.h)
OBJ     = $(SRC:.c=.o)
DEP     = $(SRC:.c=.d)
CFLAGS  = --std=gnu99 -O3 -DNDEBUG
CFLAGS += -Wno-multichar -Wno-attributes -Wno-deprecated -Werror
CFLAGS += -MMD -I$(TOOLCHAIN)/$(ARCH)/include
LDFLAGS =


ifndef V
	QUIET_CC   = @echo ' CC   ' $<;
	QUIET_LINK = @echo ' LINK ' $@;
endif

all: $(BIN).bin

-include $(DEP)

$(BIN).i: $(OBJ)
	$(QUIET_LINK)$(CC) -o $@ $^ $(LDFLAGS)

%.bin: %.i
	$(TOOLCHAIN)/bin/MakeAPPL -c $< -o $*

%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

wc:
	@wc -l $(SRC) $(INC) | sort -n

clean:
	rm -f $(BIN) $(OBJ) $(DEP)

.PHONY: clean wc

