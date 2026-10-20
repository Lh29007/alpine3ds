include $(DEVKITARM)/3ds_rules

TARGET   := alpine3ds
BUILD    := build
SOURCES  := source

ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

LIBDIRS  := $(PORTLIBS) $(CTRULIB)
INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
LDFLAGS  := -specs=3dsx.specs -g $(ARCH) $(LIBPATHS)
LIBS     := -lcitro2d -lcitro3d -lctru -lm

SRC      := $(wildcard $(SOURCES)/*.c)
OBJ      := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(SRC))

.PHONY: all clean

all: $(TARGET).3dsx

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx
