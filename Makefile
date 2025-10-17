# If you move this project you can change the directory 
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
GBDK_HOME = ../../../

PROJECTNAME    = GBColorPicker

# Compile options
CC = $(GBDK_HOME)bin/lcc

# CLFAGS options:
CFLAGS += -Wm-yt3       # MBC1+RAM+BATTERY. MBC1 chosen for small size of SRAM (because only 8 bytes are needed).
CFLAGS += -Wm-ya1       # 8KiB of SRAM.
CFLAGS += -Wm-yc        # Game Boy Color support.
CFLAGS += -Wm-ys        # Super Game Boy support.
CFLAGS += -Wl-j -Wm-yS  # Generate a .sym file with labels in a format commonly supported by emulators.
CFLAGS += -Wf--Werror   # Otherwise warnings might be missed during compilation.
CFLAGS += -Wf--max-allocs-per-node50000  # Recommended setting, but does slow down compilation considerably.

BINS	    = $(PROJECTNAME).gb
CSOURCES   := $(wildcard *.c)

all:	$(BINS)

font.c font.h: font.png
	$(GBDK_HOME)/bin/png2asset.exe ./font.png -spr8x8 -sprite_no_optimize -keep_palette_order -noflip -max_palettes 1 -tiles_only -no_palettes

font.o:	font.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROJECTNAME).gb:	gbcolorpicker.o font.o
	$(CC) $(CFLAGS) -Wm-yn"$(PROJECTNAME)" -o $@ $^

clean:
	rm -f *.o *.lst *.map *.gb *.ihx *.sym *.cdb *.adb *.asm *.noi
