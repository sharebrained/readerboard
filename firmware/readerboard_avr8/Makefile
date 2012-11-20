# Hey Emacs, this is a -*- makefile -*-
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
#  >> Modified for use with the LUFA project. <<
#  >> Drastically stripped down for the ShareBrained Technology Monulator
#     project.
#
# Released to the Public Domain
#
# Additional material for this makefile was written by:
# Peter Fleury
# Tim Henigan
# Colin O'Flynn
# Reiner Patommel
# Markus Pfaff
# Sander Pool
# Frederik Rouleau
# Carlos Lamas
# Dean Camera
# Opendous Inc.
# Denver Gingerich
#
#----------------------------------------------------------------------------

MCU = at90usb162

TARGET = main

SRC =
CPPSRC = main.cpp \
         usb.cpp
ASRC =

CFLAGS = -mmcu=$(MCU)
CFLAGS += -Os
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -fno-exceptions
CFLAGS += -Wall
CFLAGS += -Wundef
#CFLAGS += -fwhole-program
#CFLAGS += -flto

CPPFLAGS = -mmcu=$(MCU)
CPPFLAGS += -Os
CPPFLAGS += -funsigned-char
CPPFLAGS += -funsigned-bitfields
CPPFLAGS += -fpack-struct
CPPFLAGS += -fshort-enums
CPPFLAGS += -fno-exceptions
#CPPFLAGS += -ffinite-math-only
#CPPFLAGS += -fno-signed-zeros
#CPPFLAGS += -funsafe-math-optimizations
#CPPFLAGS += -ffast-math
CPPFLAGS += -Wall
CPPFLAGS += -Wundef
#CPPFLAGS += -fwhole-program
#CPPFLAGS += -flto

LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
#LDFLAGS += -flto
LDFLAGS += -lm

CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude
AVRDUDE_PROGRAMMER = avrispmkii
AVRDUDE_PORT = usb
AVRDUDE_FLAGS = -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -p $(MCU)

OBJ = $(SRC:%.c=%.o) $(CPPSRC:%.cpp=%.o) $(ASRC:%.S=%.o)
LST = $(SRC:%.c=%.lst) $(CPPSRC:%.cpp=%.lst) $(ASRC:%.S=%.lst)

all: build size

build: elf hex eep

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep

ELFSIZE = $(SIZE) $(TARGET).elf

size:
	@if test -f $(TARGET).elf; then echo; $(ELFSIZE); 2>/dev/null; echo; fi

program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex -U eeprom:w:$(TARGET).eep

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock $< $@

%.eep: %.elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O ihex $< $@ || exit 0

%.elf: $(OBJ)
	$(CC) $(CFLAGS) $^ --output $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

clean:
	rm -f $(TARGET).hex
	rm -f $(TARGET).eep
	rm -f $(TARGET).elf
	rm -f $(TARGET).map
	rm -f $(SRC:%.c=%.o) $(CPPSRC:%.cpp=%.o) $(ASRC:%.S=%.o)
