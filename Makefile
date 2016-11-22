# DO NOT FORGET to define BITBOX environment variable 

#NO_VGA = 1 # comment out to allow graphics, do a "make clean" to reset.
USE_SDCARD = 1 # comment out to get no wav files

NAME = noisebox
GAME_C_FILES = main.c
GAME_H_FILES =

ifndef NO_VGA
GAME_C_FILES += font.c
GAME_H_FILES += font.h
endif

GAME_C_OPTS = -DVGAMODE_320 -DBITBOX_SAMPLERATE=16000 -DBITBOX_SNDBUF_LEN=1066

# see this file for options
include $(BITBOX)/lib/bitbox.mk
