#include <gb/gb.h>
#include <gb/bcd.h>
#include <gb/cgb.h>
#include <gb/sgb.h>
#include <gbdk/console.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <rand.h>
#include <time.h>

#include "font.h"

#define PALETTE_COLOR_1 0U
#define PALETTE_COLOR_2 1U
#define PALETTE_COLOR_3 2U
#define PALETTE_COLOR_4 3U

#define TILE_ID_COLOR 0x0U
#define TILE_ID_MARKER 0x67U

typedef uint8_t uint5_t;
#define UINT5_MIN (0x00U)
#define UINT5_MAX (0x1fU)

// Turns a five bit color value into an eight bit color value by repeating the
// three most significant bits in the three least.
// TODO: wouldn't be enough ((byte) << 3))
#define EXTEND(byte) ((byte) << 3) | ((byte) >> 2)

// Set default colors
uint5_t colors[4][3] = {
    {UINT5_MAX, UINT5_MAX, UINT5_MAX}, // white
    {UINT5_MAX, UINT5_MIN, UINT5_MIN}, // red
    {UINT5_MIN, UINT5_MAX, UINT5_MIN}, // green
    {UINT5_MIN, UINT5_MIN, UINT5_MAX}  // blue
};

palette_color_t raw_colors[4];

// return black if background color is light, white otherwise
uint16_t text_color (uint8_t bg)
{
	// https://stackoverflow.com/a/946734
	const uint16_t gray = colors[bg][0] * 10 / 3 + colors[bg][1] * 10 / 6 + colors[bg][2] / 10;
	return (gray > 23 ? RGB_BLACK : RGB_WHITE); // 186/256, is like 23/32(5bit)
}

// Load a palette of four colors into the raw_colors array and the colors array.
void loadColorsFromPalette(palette_color_t * palette)
{
    int i = 0;
    for (i = 0; i < 4; i++) {
        raw_colors[i] = palette[i];
        colors[i][0] = raw_colors[i] & UINT5_MAX;
        colors[i][1] = (raw_colors[i] >> 5) & UINT5_MAX;
        colors[i][2] = (raw_colors[i] >> 10) & UINT5_MAX;
    }
}

// Wait some number of frames.
void wait(uint8_t frames)
{
    for (frames; frames != 0; --frames) {
        wait_vbl_done();
    }
}

// Convert a hex digit to a tile number.
uint8_t hex(const uint8_t nibble)
{
    if (nibble < 10) {
        return (nibble + 0x10); // 16th tile zero '0'
    } else {
        return (nibble - 10 + 0x41); // 321st tile 'a'
    }
}

// SHow an ascii character at a given coordinate.
void show_char_xy(const uint8_t x, const uint8_t y, const char what)
{
    gotoxy(x, y);
    setchar(what);
}

// Show a byte in hexadecimal at the given coordinate and the one to its right.
void show_hex_byte_xy(const uint8_t x, const uint8_t y, const uint8_t byte)
{
    static uint8_t digit;
    
    digit = hex((byte >> 4) & 0xf);
    set_tile_xy(x, y, digit);

    digit = hex(byte & 0x0f);
    set_tile_xy(x+1, y, digit);
}
        
// Print the passed in color formatted as a CGB 15 bit color.
inline void print_raw(const uint8_t x, const uint8_t y, const palette_color_t raw)
{
    show_hex_byte_xy(x, y, (raw >> 8) & 0xff);
    show_hex_byte_xy(x+2, y, raw & 0xff);
}

// Print the passed in color formatted as an HTML color code.
inline void print_html(const uint8_t x, const uint8_t y, const uint8_t *p)
{
	set_tile_xy(x, y, 3); // #
    show_hex_byte_xy(x+1, y, EXTEND(p[0]));
    show_hex_byte_xy(x+3, y, EXTEND(p[1]));
    show_hex_byte_xy(x+5, y, EXTEND(p[2]));
}

// Print the passed in color formatted as decimal R, G, B.
inline void print_decimal(const uint8_t x, const uint8_t y, const uint8_t *p)
{
    static uint8_t i;
    static uint8_t extended;
    static BCD bcd;

    show_char_xy(x, y, 'R');
    show_char_xy(x, y+1, 'G');
    show_char_xy(x, y+2, 'B');

    for (i = 0; i < 3; i++) {
        // Extend from 5 bit to 8 bit and store as 16 bit for BCD.
        extended = (p[i] << 3) | (p[i] >> 2);

        // Built in BCD conversion is for uint16_t so this is a little inefficient.
        uint2bcd(extended, &bcd);

        // Hundreds place 
        set_tile_xy(x+1, y+i, hex((bcd >> 8) & 0x0f));
        // Tens place.
        // This cast is to convince the compiler to use one swp instruction instead of four 16 bit shifts.
        set_tile_xy(x+2, y+i, hex(((*((uint8_t*)(&bcd))) >> 4) & 0x0f));
        // Ones place
        set_tile_xy(x+3, y+i, hex(bcd & 0x0f));
    }
}

// Print the build date.
void print_date(void)
{
    gotoxy(5,16);
    puts("2025-10-17");
}

void main(void)
{
    // SRAM is used to save the selected palette.
    static palette_color_t * const sram = (palette_color_t *)_SRAM;

    // Running on Super Game Boy?
    static bool sgb;

    // Super Game Boy palette packets with hardcoded initial colors.
    static uint8_t sgb_pal01[] = {SGB_PAL_01 << 3 | 1, 0xff,0x7f, 0xff,0x7f, 0xff,0x7f, 0,0,  0x1f,0, 0xff,0x7f, 0,0, 0};
    static uint8_t sgb_pal23[] = {SGB_PAL_23 << 3 | 1, 0xff,0x7f, 0xe0,0x03, 0xff,0x7f, 0,0,  0,0x7c, 0xff,0x7f, 0,0, 0};

    // Super Game Boy color rectangles for the four quadrants of the screen.
    static const uint8_t const sgb_attr_blk[] = {
        SGB_ATTR_BLK << 3 | 2, 3, 1,1, 10,0,19,8, 1,2, 0,9,9,17, 1,3,\
        10,9,19,17, 0,0,0,0,0,0,0,0,0,0,0,0
    };

    // Locations for the different info to print.
    static const uint8_t const loc_x[] = {1, 11, 1, 11}; //hex rgb components
    static const uint8_t const loc_y[] = {1, 1, 10, 10};
    static const uint8_t const raw_loc_x[] = {3, 13, 3, 13}; // 15 bit GBC color
    static const uint8_t const raw_loc_y[] = {2, 2, 11, 11};
    static const uint8_t const html_loc_x[] = {1, 11, 1, 11}; // web #012ABC
    static const uint8_t const html_loc_y[] = {7, 7, 16, 16};
    static const uint8_t const decimal_loc_x[] = {3, 13, 3, 13}; // rgb decimal
    static const uint8_t const decimal_loc_y[] = {4, 4, 13, 13};
    
    static const uint8_t const marker_loc_x[4][3] = {{0, 3, 6}, {10, 13, 16}, {0, 3, 6}, {10, 13, 16}};
    static const uint8_t const marker_loc_y[4] = {1, 1, 10, 10};

    static uint8_t selected_color, selected_component;
    static uint8_t new_buttons, old_buttons;
    static bool color_changed_selected = true;
    static bool color_changed_all = true;
    static uint8_t i, j;
    static uint8_t *p;
    static uint16_t new_text_color;

    // Display mode: HTML color code style hex digits or RGB decimal components.
    static uint8_t mode;

    // Used to save/restore the selected value when pressing A or B.
    static uint5_t backup_component;

    // Turn everything on. Sprites are not used.
    SHOW_BKG;
    DISPLAY_ON;

    // PAL SGB requires a four frame delay before sending packets.
    if (!DEVICE_SUPPORTS_COLOR) {
        wait(4);
        sgb = sgb_check();
    }

    // Start menu black text on white bg
    set_bkg_palette_entry(0, 2, RGB_WHITE);
    set_bkg_palette_entry(0, 3, RGB_BLACK);

    // Supports GBC, GBA, and SGB
    if (!DEVICE_SUPPORTS_COLOR && !sgb) {
        puts("\n\n  GB Color Picker\n  does not run\n  in B&W. Sorry!");
        print_date();
        return;
    }
    
    // Check whether SRAM is valid
    ENABLE_RAM;
    if (sram[0] != 0xffff) {
        // If so load the intial palette from the save.
        loadColorsFromPalette(sram);
    }
    DISABLE_RAM;

    // Display title and usage
    puts("\n  GB Color Picker\n  ---------------\n\n Use left/right to\n choose a component\n and up/down to\n change it.\n\n A and B toggle\n min, max and back.\n\n Select randomizes.");
    print_date();
    do {
      wait(1);
    } while (joypad() == 0);
    wait(12);

    
	// Overwrite GBDK font tiles with color 1 for the background to avoid the SGB shared color.
    // Unoptimized: 2x font tiles in ROM(GBDK + color1 as background)
    set_bkg_data(0, font_TILE_COUNT, font_tiles);

    // clear screen.
    cls();

    initrand(clock());
    
    // adjust text color
	set_bkg_palette_entry(0, 3, text_color(0));
	set_bkg_palette_entry(1, 3, text_color(1));
	set_bkg_palette_entry(2, 3, text_color(2));
	set_bkg_palette_entry(3, 3, text_color(3));

    // Initialize color areas. Each quadrant of the screen gets a different palette.
    if (sgb) {
        // Draw the Super Game Boy attribute rectangles.
        sgb_transfer(sgb_attr_blk);
    } else {
        // Setup the Game Boy Color tile attribute rectangles.
        VBK_REG = 1;
        fill_rect( 0, 0, 10, 9, PALETTE_COLOR_1);
        fill_rect(10, 0, 10, 9, PALETTE_COLOR_2);
        fill_rect( 0, 9, 10, 9, PALETTE_COLOR_3);
        fill_rect(10, 9, 10, 9, PALETTE_COLOR_4);
        VBK_REG = 0;
    }

    // Fill everything with the tile that will show the selected colors.
    fill_rect( 0, 0, 20, 18, TILE_ID_COLOR);

    // marker at color 1 component 1(red)
    set_tile_xy(marker_loc_x[selected_color][selected_component], 
                marker_loc_y[selected_color], TILE_ID_MARKER);

    // Init the backup component.
    backup_component = colors[selected_color][selected_component];

    // Main loop
    while(true) {
        // For each of the four colors...
        for (i = 0; i < 4; i++) {
            // Calculate the GBC 15 bit blue-green-red ordered color
            raw_colors[i] = RGB(colors[i][0], colors[i][1], colors[i][2]);
		
			// Display the hex value of the newly calculated color
			print_raw(raw_loc_x[i], raw_loc_y[i], raw_colors[i]);

			// Display it formatted as a 24 bit red-green-blue ordered hex value, like the commonly used HTML color codes
			print_html(html_loc_x[i], html_loc_y[i], colors[i]);
		
			// Display it formatted as decimal components
			print_decimal(decimal_loc_x[i], decimal_loc_y[i], colors[i]);            

            // Display it as three separate 5 bit hex formatted components
            for (j = 0; j < 3; j++) {
                show_hex_byte_xy(loc_x[i] + 3 * j, loc_y[i], colors[i][j]);
            }
			
			// text color
            new_text_color = text_color(i);
			set_bkg_palette_entry(i, 3, new_text_color);
            // sgb 4th colors
            switch (i)
            {
                case 0:
                    sgb_pal01[7] = new_text_color & 0xff;
                    sgb_pal01[8] = (new_text_color >> 8) & 0xff;
                break;
                
                case 1:
                    sgb_pal01[13] = new_text_color & 0xff;
                    sgb_pal01[14] = (new_text_color >> 8) & 0xff;
                break;

                case 2:
                    sgb_pal23[7] = new_text_color & 0xff;
                    sgb_pal23[8] = (new_text_color >> 8) & 0xff;
                break;
                
                case 3:
                    sgb_pal23[13] = new_text_color & 0xff;
                    sgb_pal23[14] = (new_text_color >> 8) & 0xff;
                break;

                default:
                break;
            }

        }

        if (color_changed_selected || color_changed_all) {
            // Save the colors to SRAM so the .sav file can be used to import a palette.
            ENABLE_RAM;
            for (i = 0; i < 4; i++) {
                sram[i] = raw_colors[i];
            }
            DISABLE_RAM;
            // Change the display palette onscreen.
            if (sgb) {
                sgb_pal01[3] = raw_colors[0] & 0xff;
                sgb_pal01[4] = (raw_colors[0] >> 8) & 0xff;
                sgb_pal01[9] = raw_colors[1] & 0xff;
                sgb_pal01[10] = (raw_colors[1] >> 8) & 0xff;
                sgb_pal23[3] = raw_colors[2] & 0xff;
                sgb_pal23[4] = (raw_colors[2] >> 8) & 0xff;
                sgb_pal23[9] = raw_colors[3] & 0xff;
                sgb_pal23[10] = (raw_colors[3] >> 8) & 0xff;

                // Super Game Boy transfers are slow so only update what has changed.
                if ((selected_color <= 1) || color_changed_all) {
                    sgb_transfer(sgb_pal01);
                }
                if ((selected_color >= 2) || color_changed_all) {
                    sgb_transfer(sgb_pal23);
                }
            } else {
                // Each of the four colors gets its own palette.
                if (color_changed_all) {
                    set_bkg_palette_entry(PALETTE_COLOR_1, 1, raw_colors[0]);
                    set_bkg_palette_entry(PALETTE_COLOR_2, 1, raw_colors[1]);
                    set_bkg_palette_entry(PALETTE_COLOR_3, 1, raw_colors[2]);
                    set_bkg_palette_entry(PALETTE_COLOR_4, 1, raw_colors[3]);
                } else {
                    // Set the newly calculated color into the second entry of a palette.
                    // Using the second entry because of SGB transparency.
                    set_bkg_palette_entry(PALETTE_COLOR_1 + selected_color, 1, raw_colors[selected_color]);
                }
            }
        }

        // A repeat delay
        // SGB is slow enough to change colors as it is.
        if (!sgb || !color_changed_selected) {
            wait(1);
        }

        color_changed_selected = false;
        color_changed_all = false;

        // Add an initial delay before repeat when a button is newly pressed so you can make precise changes
        if (old_buttons != new_buttons) {
            old_buttons = new_buttons;
            for (i = 20; i != 0; --i) {
                if (joypad()) {
                    wait(1);
                } else {
                    old_buttons = 0;
                    break;
                }
            }
        }

        // Wait until something happens.
        do {
            wait(1);
            new_buttons = joypad();
            // Check if SRAM has changed and if so, reload.
            // This is useful in an emulator for pasting palettes into memory directly.
            ENABLE_RAM;
            for (i = 0; i < 4; i++) 
            {
                if (sram[i] != raw_colors[i]) 
                {
                    color_changed_all = true;
                    color_changed_selected = true;
                    break;
                }
            }
            if (color_changed_all) {
                loadColorsFromPalette(sram);
            }
            DISABLE_RAM;
        } while (!(new_buttons || old_buttons || color_changed_all));

        // D-pad is pressed. Update.
        switch (new_buttons) {
            case J_LEFT:
            // De-select the current component
            show_hex_byte_xy(loc_x[selected_color] + 3 * selected_component, 
                loc_y[selected_color], colors[selected_color][selected_component]);
            set_tile_xy(marker_loc_x[selected_color][selected_component], 
                marker_loc_y[selected_color], TILE_ID_COLOR);
            
            // Cycle to select the previous color component or color
            if (selected_component > 0) {
                selected_component -= 1;
            } else {
                selected_component = 2;
                if (selected_color > 0) {
                    selected_color -= 1;
                } else {
                    selected_color = 3;
                }
            }
            
            // Highlight the new component
            set_tile_xy(marker_loc_x[selected_color][selected_component], 
                marker_loc_y[selected_color], TILE_ID_MARKER);
            
            // Set the backup component.
            backup_component = colors[selected_color][selected_component];

            // A slightly longer delay for switching components than changing a component
            wait(1);
            break;

            case J_RIGHT:
            // De-select the current component
            show_hex_byte_xy(loc_x[selected_color] + 3 * selected_component, loc_y[selected_color], colors[selected_color][selected_component]);
            set_tile_xy(marker_loc_x[selected_color][selected_component], 
                marker_loc_y[selected_color], TILE_ID_COLOR);

            // Cycle to select the previous color component or color
            if (selected_component < 2) {
                selected_component += 1;
            } else {
                selected_component = 0;
                if (selected_color < 3) {
                    selected_color += 1;
                } else {
                    selected_color = 0;
                }
            }

            // Highlight the new component
            set_tile_xy(marker_loc_x[selected_color][selected_component], 
                marker_loc_y[selected_color], TILE_ID_MARKER);

            // Set the backup component.
            backup_component = colors[selected_color][selected_component];

            // A slightly longer delay for switching components than changing a component
            wait(1);
            break;

            case J_UP:
            // Increment the currently selected component if not already maxed
            if (colors[selected_color][selected_component] < UINT5_MAX) {
                colors[selected_color][selected_component] += 1;
                // Set the backup component.
                backup_component = colors[selected_color][selected_component];
                color_changed_selected = true;
            }
            break;

            case J_DOWN:
            // Decrement the currently selected component if not already zero
            if (colors[selected_color][selected_component] > UINT5_MIN) {
                colors[selected_color][selected_component] -= 1;
                // Set the backup component.
                backup_component = colors[selected_color][selected_component];
                color_changed_selected = true;
            }
            break;

            case J_A:
            if (colors[selected_color][selected_component] == UINT5_MAX) {
                colors[selected_color][selected_component] = backup_component;
            } else {
                colors[selected_color][selected_component] = UINT5_MAX;
            }
            color_changed_selected = true;
            break;

            case J_B:
            if (colors[selected_color][selected_component] == UINT5_MIN) {
                colors[selected_color][selected_component] = backup_component;
            } else {
                colors[selected_color][selected_component] = UINT5_MIN;
            }
            color_changed_selected = true;
            break;

            case J_SELECT:
                // Randomize colors, randw doesn't randomize MSB
				palette_color_t rand_pal[4] = {
                    (rand() | rand() << 8),
                    (rand() | rand() << 8),
                    (rand() | rand() << 8),
                    (rand() | rand() << 8),
				};

                loadColorsFromPalette(rand_pal);

				color_changed_all = true;
				color_changed_selected = true;
            break;
        }
    }
}
