
VGA_MODE=NONE
#VGA_MODE=320 
USE_SDCARD = 1 # comment out to get no wav files
NO_USB=1

NAME = noisebox
GAME_C_FILES = main.c
GAME_H_FILES =

ifneq ($(VGA_MODE), NONE)
GAME_C_FILES += font.c
GAME_H_FILES += font.h
USE_SDCARD = 1
endif

GAME_C_OPTS = -DVGA_MODE=$(VGA_MODE) -DBITBOX_SAMPLERATE=16000 -DBITBOX_SNDBUF_LEN=1066

# see this file for options
BITBOX=bitbox
include $(BITBOX)/kernel/bitbox.mk
