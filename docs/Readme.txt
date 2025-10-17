Game Boy Color Picker

A color picker that runs on Super Game Boy, Game Boy Color, and Game Boy Advance.

Colors on the Game Boy Color (GBC), Game Boy Advance, and Super Game Boy (Super Nintendo) are very different from typical monitors, and even from each other. It can take a surprising amount of time just to find colors that look good when run on original hardware and in emulators that try to be accurate to hardware. There are PC tools to help approximate what colors will look like under various conditions, but they are no substitute for seeing your choices as they will really look.

Game Boy Color Picker is a simple program that runs anywhere Game Boy games can run, including from a flash cart on GBC, Game Boy Advance, Super Game Boy, and emulators. It allows you to choose four colors at a time from the possible 32,768. As you change the colors you can see them in GBC/SNES 15 bit format, ready to use in your project and also in HTML color code format, ready to use in a graphics program.

Controls are: Left and right move between the three components (red, green, and blue) of four different colors, and Up and Down change them with the effect immediately visible. Select randomizes colors.

The selected palette is saved to the beginning of SRAM so your `GBColorPicker.sav` file can be used by programs that read binary GBC palettes. The first eight bytes are also continuously checked against the active colors and reloaded if any difference is found so palettes can be pasted directly into an emulator's memory.

Game Boy Color Picker is written in C and compiles with GBDK. Source code is available at https://github.com/xenophile127/GameBoyColorPicker

Changes in 2025-10-17:
* "Transparent" text background also on SGB.

Changes in 2025-10-15:
* All data onscreen, no modes.
* Select randomizes all colors.
* Text color white on dark background, black on light background.
* Text background same as background color. BUG: doesn't work on Super Game Boy(white background, black text). TODO: understand how to chage SGB colors 3 & 4, prepare font with 3rd color as background.

Changes in 2025-08-15:
* Palettes are reloaded if they are manually changed in an emulator's cartridge memory (SRAM).

Changes in 2024-02-06:
* The A and B buttons now toggle maximum and minimum component values and back.

Changes in 2024-01-30:
* Press Select to show decimal RGB values.
* Super Game Boy support.
* Save the palette in the .sav file.

Changes in 2022-05-15:
* Initial release.

-xenophile 2024-02-06
-plyr0 2025-10-15
