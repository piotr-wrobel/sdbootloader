################################################################################################
#                  „Widzimy daleko, bo stoimy na ramionach olbrzyma”                           #
# Ten plik zosta³ zmodyfikowany przez mlodedzrwale, wszystkie komentarze dodane przez osoby    #
# które wczeœniej teorzy³y ten plik s¹ niezmienione na samym koñcu pliku.                      #
################################################################################################


################################################################################################
# Wybierz odpowedni procesor (MCU), dostêpne procesory to:                                     #
# atmega48; atmega8; atmega8a; atmega88; atmega16; atmega162; atmega168; atmega169; Atmega3250 #
# atmega328; atmega32; atmega3290; atmega644                                                   #
# (wybranie procesora spoza listy zakoñczy siê b³êdem)                                         #
# Nastêpnie wybierz czêstotliwoœæ, z jak¹ pracowa³ bêdzie procesor (F_CPU) oraz prêdkoœæ pracy #
# UART (BAUD)                                                                                  #
################################################################################################





MCU = atmega328p
F_CPU = 20000000
BAUD = 57600
















################################################################################################
#######               Definicje PROCESORÓW/ DEFINITONS OF DEVICES                       ########

# Atmega328
ifeq ($(MCU), atmega328)
	SIGN = 0x3A
	BLS_START = 0x7000
endif

# Atmega328P
ifeq ($(MCU), atmega328p)
	SIGN = 0x3A
	BLS_START = 0x7000
endif


ifndef SIGN
$(error WRONG DEVICE)
endif

#######                                                                                 ########
################################################################################################



FORMAT = ihex
TARGET = main
OBJDIR = .
SRC = $(TARGET).c sd_spi.c uart.c fat16.c
OPT = s


# Debugging format.
# Native formats for AVR-GCC's -g are dwarf-2 [default] or stabs.
# AVR Studio 4.10 requires dwarf-2.
# AVR [Extended] COFF format requires stabs, plus an avr-objcopy run.
DEBUG = dwarf-2

# List any extra directories to look for include files here.
# Each directory must be seperated by a space.
# Use forward slashes for directory separators.
# For a directory that has spaces, enclose it in quotes.

EXTRAINCDIRS =
# Compiler flag to set the C Standard level.
# c89 = "ANSI" C
# gnu89 = c89 plus GCC extensions
# c99 = ISO C99 standard (not yet fully implemented)
# gnu99 = c99 plus GCC extensions

CSTANDARD = -std=gnu99
# Dla wszystkich
COMDEFS = -DBLS_START=$(BLS_START)
COMDEFS += -DXTAL=$(F_CPU)
COMDEFS += -DMCU=$(MCU)

# Place -D or -U options here for C sources
CDEFS =

# Place -D or -U options here for ASM sources
ADEFS =

#---------------- Compiler Options C ----------------
# -g*: generate debugging information
# -O*: optimization level
# -f...: tuning, see GCC manual and avr-libc documentation
# -Wall...: warning level
# -Wa,...: tell GCC to pass this to the assembler.
# -adhlns...: create assembler listing
CFLAGS = -g$(DEBUG)
CFLAGS += -DF_CPU=$(F_CPU)UL
CFLAGS += -DSIGN=$(SIGN)
CFLAGS += -DBAUD=$(BAUD)
CFLAGS += $(COMDEFS)
CFLAGS += $(CDEFS)
CFLAGS += -O$(OPT)
# dodane optymalizacje
#CFLAGS += -Wall
#CFLAGS += -ffunction-sections
#CFLAGS += -fdata-sections
#koniec

CFLAGS += -fgnu89-inline
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -fno-tree-scev-cprop
CFLAGS += -fno-inline-small-functions
CFLAGS += -fno-split-wide-types
CFLAGS += -ffreestanding
CFLAGS += -mno-interrupts
#CFLAGS += -pedantic
CFLAGS += -Wextra
CFLAGS += -Wno-sign-compare
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
# W tym projekcie potrzebne jest wylaczenie sprawdzania scrict aliasing
# ze wzgledu na mapowanie struktur FAT16 na bufor
CFLAGS += -fno-strict-aliasing

CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wa,-adhlns=$(<:%.c=$(OBJDIR)/%.lst)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)

#---------------- Compiler Options C++ ----------------
# -g*: generate debugging information
# -O*: optimization level
# -f...: tuning, see GCC manual and avr-libc documentation
# -Wall...: warning level
# -Wa,...: tell GCC to pass this to the assembler.
# -adhlns...: create assembler listing
CPPFLAGS = -g$(DEBUG)
CPPFLAGS += $(CPPDEFS)
CPPFLAGS += -O$(OPT)
CPPFLAGS += -funsigned-char
CPPFLAGS += -funsigned-bitfields
CPPFLAGS += -fpack-struct
CPPFLAGS += -fshort-enums
CPPFLAGS += -fno-exceptions
CPPFLAGS += -Wall
CPPFLAGS += -Wundef
CPPFLAGS += -Wa,-adhlns=$(<:%.cpp=$(OBJDIR)/%.lst)
CPPFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

#---------------- Assembler Options ----------------
# -Wa,...: tell GCC to pass this to the assembler.
# -adhlns: create listing
# -gstabs: have the assembler create line number information; note that
# for use in COFF files, additional information about filenames
# and function names needs to be present in the assembler source
# files -- see avr-libc docs [FIXME: not yet described there]
# -listing-cont-lines: Sets the maximum number of continuation lines of hex
# dump that will be displayed for a given single line of source input.
#ASFLAGS = $(ADEFS) -Wa,-adhlns=$(<:%.S=$(OBJDIR)/%.lst),-gstabs,--listing-cont-lines=100
ASFLAGS = -gstabs
ASFLAGS += -DF_CPU=$(F_CPU)
ASFLAGS += $(COMDEFS)
ASFLAGS += $(ADEFS)
ASFLAGS += -Wa,-adhlns=$(<:%.S=$(OBJDIR)/%.lst)
ASFLAGS += -listing-cont-lines=100

#---------------- External Memory Options ----------------
# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# used for variables (.data/.bss) and heap (malloc()).
#EXTMEMOPTS = -Wl,-Tdata=0x801100,--defsym=__heap_end=0x80ffff
# 64 KB of external RAM, starting after internal RAM (ATmega128!),
# only used for heap (malloc()).
#EXTMEMOPTS = -Wl,--section-start,.data=0x801100,--defsym=__heap_end=0x80ffff
EXTMEMOPTS =
#---------------- Linker Options ----------------
# -Wl,...: tell GCC to pass this to linker.
# -Map: create map file
# --cref: add cross reference to map file
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--section-start=.text=$(BLS_START)
LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))
#---------------- Programming Options (avrdude) ----------------
# Programming hardware
# Type: avrdude -c ?
# to get a full listing.
#

AVRDUDE_PROGRAMMER = avrispmkII
AVRDUDE_PORT = usb
#AVRDUDE_PROGRAMMER = stk200
#AVRDUDE_PORT = lpt1
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM=
#Disable autoerase for flash memory
AVRDUDE_DISABLE_AUTOERASE =
#AVRDUDE_DISABLE_AUTOERASE = -D

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
AVRDUDE_FLAGS += $(AVRDUDE_DISABLE_AUTOERASE)

#---------------- Debugging Options ----------------
# For simulavr only - target MCU frequency.
DEBUG_MFREQ = $(F_CPU)
# Set the DEBUG_UI to either gdb or insight.
# DEBUG_UI = gdb
DEBUG_UI = insight
# Set the debugging back-end to either avarice, simulavr.
DEBUG_BACKEND = avarice
#DEBUG_BACKEND = simulavr
# GDB Init Filename.
GDBINIT_FILE = __avr_gdbinit
# When using avarice settings for the JTAG
JTAG_DEV = /dev/com1
# Debugging port used to communicate between GDB / avarice / simulavr.
DEBUG_PORT = 4242
# Debugging host used to communicate between GDB / avarice / simulavr, normally
# just set to localhost unless doing some sort of crazy debugging when
# avarice is running on a different computer.
DEBUG_HOST = localhost
#============================================================================
# Define programs and commands.

SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AR = avr-ar rcs
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
REMOVEDIR = rm -rf
COPY = cp
WINSHELL = cmd
# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = -------- end --------
MSG_SIZE_BEFORE = Size before:
MSG_SIZE_AFTER = Size after:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_CPP = Compiling C++:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:
MSG_CREATING_LIBRARY = Creating library:
# Define all object files.
OBJ = $(SRC:%.c=$(OBJDIR)/%.o) $(CPPSRC:%.cpp=$(OBJDIR)/%.o) $(ASRC:%.S=$(OBJDIR)/%.o)
# Define all listing files.
LST = $(SRC:%.c=$(OBJDIR)/%.lst) $(CPPSRC:%.cpp=$(OBJDIR)/%.lst) $(ASRC:%.S=$(OBJDIR)/%.lst)
# Compiler flags to generate dependency files.
# GENDEPFLAGS = -MMD -MP -MF .dep/$(@F).d
# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_CPPFLAGS = -mmcu=$(MCU) -I. -x c++ $(CPPFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)
# Default target.
all: begin gccversion  build size end
# Change the build target to build a HEX file or a library.
build: elf hex eep lss sym
#build: lib
elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym
LIBNAME=lib$(TARGET).a
lib: $(LIBNAME)
# Eye candy.
# AVR Studio 3.x does not check make's exit code but relies on
# the following magic strings to be generated by the compile job.
begin:
	@echo
	@echo $(MSG_BEGIN)
end:
	@echo $(MSG_END)
	@echo
# Display size of file.
HEXSIZE = $(SIZE) --target=$(FORMAT) $(TARGET).hex
ELFSIZE = $(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

size:
	$(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

# Display compiler version information.
gccversion :
	@$(CC) --version
# Program the device.

# Generate avr-gdb config/init file which does the following:
# define the reset signal, load the target file, connect to target, and set
# a breakpoint at main().
gdb-config:
	@$(REMOVE) $(GDBINIT_FILE)
	@echo define reset >> $(GDBINIT_FILE)
	@echo SIGNAL SIGHUP >> $(GDBINIT_FILE)
	@echo end >> $(GDBINIT_FILE)
	@echo file $(TARGET).elf >> $(GDBINIT_FILE)
	@echo target remote $(DEBUG_HOST):$(DEBUG_PORT) >> $(GDBINIT_FILE)
ifeq ($(DEBUG_BACKEND),simulavr)
	@echo load >> $(GDBINIT_FILE)
endif
	@echo break main >> $(GDBINIT_FILE)
debug: gdb-config $(TARGET).elf
ifeq ($(DEBUG_BACKEND), avarice)
	@echo Starting AVaRICE - Press enter when "waiting to connect" message displays.
	@$(WINSHELL) /c start avarice --jtag $(JTAG_DEV) --erase --program --file \
	$(TARGET).elf $(DEBUG_HOST):$(DEBUG_PORT)
	@$(WINSHELL) /c pause
else
	@$(WINSHELL) /c start simulavr --gdbserver --device $(MCU) --clock-freq \
	$(DEBUG_MFREQ) --port $(DEBUG_PORT)
endif
	@$(WINSHELL) /c start avr-$(DEBUG_UI) --command=$(GDBINIT_FILE)
# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT = $(OBJCOPY) --debugging
COFFCONVERT += --change-section-address .data-0x800000
COFFCONVERT += --change-section-address .bss-0x800000
COFFCONVERT += --change-section-address .noinit-0x800000
COFFCONVERT += --change-section-address .eeprom-0x810000
coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-avr $< $(TARGET).cof
extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof
# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock $< $@
%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $@ || exit 0
# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -S -z $< > $@
# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< > $@
# Create library from object files.
.SECONDARY : $(TARGET).a
.PRECIOUS : $(OBJ)
%.a: $(OBJ)
	@echo
	@echo $(MSG_CREATING_LIBRARY) $@
	$(AR) $@ $(OBJ)
# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(ALL_CFLAGS) $^ --output $@ $(LDFLAGS)
# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@
# Compile: create object files from C++ source files.
$(OBJDIR)/%.o : %.cpp
	@echo
	@echo $(MSG_COMPILING_CPP) $<
	$(CC) -c $(ALL_CPPFLAGS) $< -o $@
# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@
# Compile: create assembler files from C++ source files.
%.s : %.cpp
	$(CC) -S $(ALL_CPPFLAGS) $< -o $@
# Assemble: create object files from assembler source files.
$(OBJDIR)/%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@
# Create preprocessed source for use in sending a bug report.
%.i : %.c
	$(CC) -E -mmcu=$(MCU) -I. $(CFLAGS) $< -o $@
# Target: clean project.
clean: begin clean_list end
clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).eep
	$(REMOVE) $(TARGET).cof
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(TARGET).hex.prev
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(SRC:.c=.i)
#	$(REMOVEDIR) .dep
# Create object files directory
$(shell mkdir $(OBJDIR) 2>/dev/null)
# Include the dependency files.
# -include $( -mkdir .dep ) $(wildcard .dep/*)
# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion \
build elf hex eep lss sym coff extcoff \
clean clean_list program debug gdb-config


# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)


# Hey Emacs, this is a -*- makefile -*-
# >> Ten plik zosta³ zmodyfikowany patrz: opcje linkera i kompilatora.
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
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
#
#----------------------------------------------------------------------------
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make coff = Convert ELF to AVR COFF.
#
# make extcoff = Convert ELF to AVR Extended COFF.
#
# make program = Download the hex file to the device, using avrdude.
# Please customize the avrdude settings below first!
#
# make debug = Start either simulavr or avarice as specified for debugging,
# with avr-gdb or avr-insight as the front end for debugging.
#
# make filename.s = Just compile filename.c into the assembler code only.
#
# make filename.i = Create a preprocessed source file for use in submitting
# bug reports to the GCC project.
#
# To rebuild project do "make clean" then "make all".
#----------------------------------------------------------------------------