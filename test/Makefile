EE_OBJS_DIR = obj/
EE_BIN_DIR = bin/

EE_ELF = $(EE_BIN_DIR)test.elf
EE_BIN = $(EE_BIN_DIR)test.bin
EE_OBJS = functions.o main.o

EE_LIBS = -luyantsc
#EE_LIBS = -luyapal
HEADER = '1s/^/gametitle=Ratchet and Clank: Up Your Arsenal [45FE0CC4]\n\n/'

ADDRESS = 000b0000

# Define EE_DEFS first
EE_DEFS = -DUYA_NTSC
EE_DEFS += -DDROIDS
# EE_DEFS += -DBOTS
# EE_DEFS += -DCTF
# EE_DEFS += -DSIEGE
# EE_DEFS += -DHUD
# EE_DEFS += -DSCAVENGERHUNT
# EE_DEFS += -DSECRET
# EE_DEFS += -DKOTH
# EE_DEFS += -DJUMPPAD
# EE_DEFS += -DDOMINATION

# Add conditional object files BEFORE the directory prefix transformation
ifneq (,$(findstring -DDROIDS,$(EE_DEFS)))
    EE_OBJS += droids.o
endif
ifneq (,$(findstring -DBOTS,$(EE_DEFS)))
    EE_OBJS += bots.o trainingmode.o
endif
ifneq (,$(findstring -DCTF,$(EE_DEFS)))
    EE_OBJS += ctf.o
endif
ifneq (,$(findstring -DSIEGE,$(EE_DEFS)))
    EE_OBJS += siege.o
endif
ifneq (,$(findstring -DKOTH,$(EE_DEFS)))
    EE_OBJS += koth.o
endif
ifneq (,$(findstring -DSECRET,$(EE_DEFS)))
    EE_OBJS += secret.o
endif
ifneq (,$(findstring -DHUD,$(EE_DEFS)))
    EE_OBJS += hud.o
endif
ifneq (,$(findstring -DSCAVENGERHUNT,$(EE_DEFS)))
    EE_OBJS += scavengerhunt.o
endif
ifneq (,$(findstring -DJUMPPAD,$(EE_DEFS)))
    EE_OBJS += jumppad.o
endif
ifneq (,$(findstring -DDOMINATION,$(EE_DEFS)))
    EE_OBJS += domination.o
endif

# Debug: Print what we have before transformation
$(info Before transform: EE_OBJS = $(EE_OBJS))

# NOW add the obj/ prefix to ALL objects
EE_OBJS := $(EE_OBJS:%=$(EE_OBJS_DIR)%)

# Debug: Print what we have after transformation
$(info After transform: EE_OBJS = $(EE_OBJS))

# Set other variables
EE_INCS := -I../../include -I$(PS2SDK)/ports/include
EE_LDFLAGS = -fno-builtin -nostdlib -nostartfiles -L. -L../../lib -L$(PS2SDK)/ports/lib
EE_CFLAGS := -D_PROGRAM_ENTRY=0x$(ADDRESS) -DDEBUG

# Add debug flag if debug target
ifeq ($(filter debug,$(MAKECMDGOALS)),debug)
    EE_CFLAGS := $(EE_CFLAGS) -DDEBUG
endif

all: linker $(EE_OBJS_DIR) $(EE_BIN_DIR) $(EE_BIN) code
	
debug: all

$(EE_OBJS_DIR):
	mkdir -p $(EE_OBJS_DIR)

$(EE_BIN_DIR):
	mkdir -p $(EE_BIN_DIR)

$(EE_OBJS_DIR)%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_DEFS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_DEFS) $(EE_INCS) -c $< -o $@

linker:
	$(EE_CC) -E -x c $(EE_CFLAGS) $(EE_DEFS) ../linkfile.template | grep -v '^#' > linkfile

code:
	bin2code $(EE_BIN) $(ADDRESS) $(EE_BIN).raw raw-writeonce
	bin2code $(EE_BIN) $(ADDRESS) $(EE_BIN).pnach pnach-writeonce
	cat template/footer.raw >> $(EE_BIN).raw
	cat template/footer.pnach >> $(EE_BIN).pnach
	sed -i $(HEADER) $(EE_BIN).pnach

clean:
	rm -rf linkfile
	rm -f -r $(EE_OBJS_DIR)
	rm -f -r $(EE_BIN_DIR)

include $(CURDIR)/../Makefile.pref
include $(CURDIR)/../Makefile.eeglobal