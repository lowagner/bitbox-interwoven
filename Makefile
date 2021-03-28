# DO NOT FORGET to define BITBOX environment variable 

USE_SDCARD = 1      # allow use of SD card for io

NAME = inwoven
SRC_FILES = chip edit-instrument edit-song edit-track font game io name \
            physics sprite
DEFINES += VGA_MODE=320

GAME_C_FILES=$(SRC_FILES:%=src/%.c)
GAME_H_FILES=$(SRC_FILES:%=src/%.h)

# see this file for options
BITBOX=bitbox
include $(BITBOX)/kernel/bitbox.mk

src/font.c: src/mk_font.py
	python2 src/mk_font.py

clean::
	rm -f src/font.c src/font.h

destroy:
	rm -f RECENT16.TXT *.*16

very-clean: clean destroy
