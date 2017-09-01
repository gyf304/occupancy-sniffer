# Makefile for ESP8266 projects
#
# Thanks to:
# - zarya
# - Jeroen Domburg (Sprite_tm)
# - Christian Klippel (mamalala)
# - Tommie Gannert (tommie)
#
# Changelog:
# - 2014-10-06: Changed the variables to include the header file directory
# - 2014-10-06: Added global var for the Xtensa tool root
# - 2014-11-23: Updated for SDK 0.9.3
# - 2014-12-25: Replaced esptool by esptool.py

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	= build
FW_BASE		= firmware

# base directory for the compiler
XTENSA_TOOLS_ROOT ?= /Volumes/esp/esp-open-sdk/xtensa-lx106-elf/bin

PATH := $(XTENSA_TOOLS_ROOT):$(PATH)

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= /Volumes/esp/esp-open-sdk/sdk

# esptool.py path and port
ESPTOOL		?= /Volumes/esp/esptool-2.1/esptool.py
ESPPORT		?= /dev/cu.wchusbserial1430
ESPTOOL_FLAGS	?= 
ESPTOOL_WRITE_FLAGS ?= -fm dout -fs 1MB -ff 20m
ESPTOOL_BAUDRATE = 115200

TTY		?= miniterm.py
TTY_BAUDRATE	= 115200
TTY_DEBUG_BAUDRATE	= 74880

# 74880

# name for the target project
TARGET		= app

# which modules (subdirectories) of the project to include in compiling
MODULES		= driver user
EXTRA_INCDIR    = include

# libraries used in this project, mainly provided by the SDK
LIBS		= c gcc hal pp phy net80211 lwip wpa main

# compiler flags using during compilation of source files
CFLAGS		= -I$(SDK_BASE)/include -L$(SDK_BASE)/lib -Wall -Wno-unused-variable -Wno-unused-but-set-variable -std=c99 -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH -DDEBUG

# linker flags used to generate the main object file
LDFLAGS		= -I$(SDK_BASE)/include -L$(SDK_BASE)/lib -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

# linker script used for the above linkier step
# LD_SCRIPT	= eagle.app.v6.ld
LD_SCRIPT = eagle.app.v6.1m.ld

# various paths from the SDK used in this project
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

# we create two different files for uploading into the flash
# these are the names and options to generate them
FW_FILE_1_ADDR	= 0x00000
FW_FILE_2_ADDR	= 0x10000
FW_INIT_DATA_DEFAULT_ADDR = 0xFC000
FW_BLANK_ADDR = 0xFE000

FW_INIT_DATA_DEFAULT_BIN = $(SDK_BASE)/bin/esp_init_data_default.bin
FW_BLANK_BIN = $(SDK_BASE)/bin/blank.bin

# select which tools to use as compiler, librarian and linker
CC		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
AR		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc

####
#### no user configurable options below here
####
SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ		:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR	:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

FW_FILE_1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
FW_FILE_2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs flash clean

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)

$(FW_BASE)/%.bin: $(TARGET_OUT) | $(FW_BASE)
	$(vecho) "FW $(FW_BASE)/"
	$(Q) $(ESPTOOL) elf2image -o $(FW_BASE)/ $(TARGET_OUT)

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(FW_BASE):
	$(Q) mkdir -p $@

flash: $(FW_FILE_1) $(FW_FILE_2)
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPTOOL_BAUDRATE) write_flash $(ESPTOOL_WRITE_FLAGS) $(FW_FILE_1_ADDR) $(FW_FILE_1) $(FW_FILE_2_ADDR) $(FW_FILE_2)

erase:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPTOOL_BAUDRATE) erase_flash

flash-init:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPTOOL_BAUDRATE) write_flash --no-compress $(FW_INIT_DATA_DEFAULT_ADDR) $(FW_INIT_DATA_DEFAULT_BIN) $(FW_BLANK_ADDR) $(FW_BLANK_BIN)

get-mac:
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPTOOL_BAUDRATE) read_mac

tty:
	$(TTY) $(ESPPORT) $(TTY_BAUDRATE)

tty-debug:
	$(TTY) $(ESPPORT) $(TTY_DEBUG_BAUDRATE)

clean:
	$(Q) rm -rf $(FW_BASE) $(BUILD_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
