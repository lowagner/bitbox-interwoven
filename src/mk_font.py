#!/usr/bin/env python2

starting_index = 0

characters = [
   ["*  *", # 0 - null
    " ** ",
    " ** ",
    "*  *"],
   [" ** ", # 1 - half sine wave
    "*  *",
    "    ",
    "    "],
   ["    ", # 2 - second half sine wave
    "    ",
    "*  *",
    " ** "],
   ["   *", # 3 - part of saw wave
    "  **",
    " *  ",
    "*   "],
   ["   *", # 4 - second part of saw wave
    "  * ",
    "**  ",
    "*   "],
   [" *  ", # 5 - first part of square
    " *  ",
    " *  ",
    " ***"],
   ["****", # 6 - second part of square wave
    "*  *",
    "*  *",
    "*  *"],
   ["", # 7
    "",
    "",
    ""],
   ["    ", # 8 - white noise spectrum
    "    ",
    "****",
    "****"],
   ["*   ", # 9 - red noise spectrum
    "**  ",
    "*** ",
    "****"],
   ["   *", # 10 - violet noise spectrum
    "  **",
    " ***",
    "****"],
   ["   *", # 11 - bend up
    "   *",
    "  * ",
    "**  "],
   ["**  ", # 12 - bend down
    "  * ",
    "   *",
    "   *"],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", # 16
    "",
    "",
    ""],
   ["", # 17
    "",
    "",
    ""],
   ["", # 18
    "",
    "",
    ""],
   ["", # 19
    "",
    "",
    ""],
   ["", # 20
    "",
    "",
    ""],
   ["", # 21
    "",
    "",
    ""],
   ["", # 22
    "",
    "",
    ""],
   ["", # 23
    "",
    "",
    ""],
   ["", # 24
    "",
    "",
    ""],
   ["", # 25
    "",
    "",
    ""],
   ["", # 26
    "",
    "",
    ""],
   ["", # 27
    "",
    "",
    ""],
   ["", # 28
    "",
    "",
    ""],
   ["", # 29
    "",
    "",
    ""],
   ["", # 30
    "",
    "",
    ""],
   ["", # 31
    "",
    "",
    ""],
   ["    ", # ' '
    "    ",
    "    ",
    "    "],
   [" *  ", # !
    " *  ",
    "    ",
    " *  "],
   ["* * ", # "
    "* * ",
    "    ",
    "    "],
   [" * *", # #
    "****",
    " * *",
    "****"],
   [" ***", # $
    "* * ",
    " * *",
    "*** "],
   ["** *", # %
    "* * ",
    " * *",
    "* **"],
   [" ** ", # &
    "****",
    "* * ",
    " ** "],
   [" *  ", # '
    " *  ",
    "    ",
    "    "],
   [" ** ", # (
    "**  ",
    "**  ",
    " ** "],
   [" ** ", # )
    "  **",
    "  **",
    " ** "],
   [" * *", # *
    "  * ",
    " * *",
    "    "],
   ["  * ", # +
    " ***",
    "  * ",
    "    "],
   ["    ", # ,
    "    ",
    " ** ",
    "  * "],
   ["    ", # -
    " ***",
    "    ",
    "    "],
   ["    ", # . = 46
    "    ",
    " ** ",
    " ** "],
   ["   *", # /
    "  * ",
    " *  ",
    "*   "],
   ["****", # 0
    "** *",
    "** *",
    "****"],
   ["*** ", # 1
    " ** ",
    " ** ",
    "****"],
   ["*** ", # 2
    "  **",
    "**  ",
    "****"],
   ["****", # 3
    " ***",
    "   *",
    "****"],
   ["* **", # 4
    "****",
    "  **",
    "  **"],
   ["****", # 5
    "*** ",
    "  **",
    "*** "],
   ["****", # 6
    "**  ",
    "* **",
    "****"],
   ["****", # 7
    "  **",
    " ** ",
    " ** "],
   ["*** ", # 8
    "* **",
    "** *",
    " ***"],
   ["****", # 9
    "** *",
    "  **",
    "****"],
   [" *  ", # :
    "    ",
    " *  ",
    "    "],
   [" *  ", # ;
    "    ",
    " ** ",
    "  * "],
   ["  * ", # <
    " *  ",
    "  * ",
    "    "],
   [" ***", # =
    "    ",
    " ***",
    "    "],
   [" *  ", # >
    "  * ",
    " *  ",
    "    "],
   ["**  ", # ?
    "  * ",
    " ** ",
    "*   "],
   ["****", # @
    "   *",
    "** *",
    "****"],
   [" ** ", # A
    "* **",
    "****",
    "* **"],
   ["*** ", # B
    "* **",
    "** *",
    "****"],
   [" ***", # C
    "*** ",
    "*** ",
    " ***"],
   ["*** ", # D
    "** *",
    "** *",
    "*** "],
   ["****", # E
    "*** ",
    "**  ",
    "****"],
   ["****", # F
    "**  ",
    "*** ",
    "**  "],
   [" ***", # G
    "**  ",
    "** *",
    "****"],
   ["* **", # H
    "****",
    "* **",
    "* **"],
   ["****", # I
    " ** ",
    " ** ",
    "****"],
   ["****", # J
    "  **",
    "  **",
    "*** "],
   ["** *", # K
    "*** ",
    "** *",
    "** *"],
   ["**  ", # L
    "**  ",
    "**  ",
    "****"],
   ["****", # M
    "****",
    "* **",
    "*  *"],
   ["** *", # N
    "** *",
    "* **",
    "* **"],
   [" ** ", # O
    "** *",
    "** *",
    " ** "],
   ["****", # P
    "** *",
    "****",
    "**  "],
   ["****", # Q
    "*  *",
    "* **",
    "****"],
   ["*** ", # R
    "** *",
    "*** ",
    "* **"],
   ["****", # S
    "**  ",
    "  **",
    "****"],
   ["****", # T
    " ** ",
    " ** ",
    " ** "],
   ["* **", # U
    "* **",
    "* **",
    "****"],
   ["** *", # V
    "** *",
    "*** ",
    " *  "],
   ["*  *", # W
    "** *",
    "****",
    "****"],
   ["** *", # X
    "  * ",
    "** *",
    "** *"],
   ["** *", # Y
    "** *",
    " ** ",
    " **"],
   ["****", # Z
    "  **",
    "**  ",
    "****"],
   ["*** ", # [
    "**  ",
    "**  ",
    "*** "],
   ["*   ", # \
    " *  ",
    "  * ",
    "   *"],
   [" ***", # ]
    "  **",
    "  **",
    " ***"],
   ["  * ", # ^
    " * *",
    "    ",
    "    "],
   ["    ", # _
    "    ",
    "    ",
    "****"],
   [" *  ", # `
    "  * ",
    "    ",
    "    "],
   ["    ", # a
    " ***",
    "* **",
    "****"],
   ["**  ", # b
    "*** ",
    "** *",
    "*** "],
   ["    ",
    " ***", # c
    "**  ",
    " ***"],
   ["  **", # d
    " ***",
    "* **",
    " ***"],
   [" ** ", # e
    "** *",
    "*** ",
    " ***"],
   [" ***", # f
    "**  ",
    "*** ",
    "**  "],
   [" ** ", # g
    "* **",
    " ***",
    "*** "],
   ["**  ", # h
    "**  ",
    "****",
    "** *"],
   [" ** ", # i
    "    ",
    " ** ",
    " ***"],
   ["  **", # j
    "    ",
    "  **",
    "****"],
   ["**  ", # k
    "** *",
    "*** ",
    "** *"],
   [" ** ", # l
    " ** ",
    " ** ",
    " ** "],
   ["    ", # m
    "****",
    "* **",
    "*  *"],
   ["    ", # n
    "*** ",
    "** *",
    "** *"],
   ["    ", # o
    " ** ",
    "** *",
    " ** "],
   ["*** ", # p
    "** *",
    "*** ",
    "**  "],
   [" ***", # q
    "* **",
    " ***",
    "  **"],
   ["    ", # r
    "*** ",
    "** *",
    "**  "],
   ["    ",
    " ***", # s
    " ** ",
    "*** "],
   [" ** ", # t
    "****",
    " ** ",
    " ***"],
   ["    ", # u
    "* **",
    "* **",
    "****"],
   ["    ", # v
    "** *",
    "*** ",
    " *  "],
   ["    ",
    "*  *", # w
    "** *",
    "****"],
   ["    ", # x
    "** *",
    "  * ",
    "** *"],
   ["    ", # y
    " * *",
    " ***",
    "***"],
   ["    ", # z
    "*** ",
    " ** ",
    " ***"],
   [" ** ", # {
    "**  ",
    " *  ",
    " ** "],
   ["  * ", # |
    "  * ",
    "  * ",
    "    "],
   [" ** ", # }
    "  **",
    "  * ",
    " ** "],
   ["   *", # ~
    " ***",
    " *  ",
    "    "],
   ["    ", # 127 - down arrow
    "    ",
    " * *",
    "  * "],
   [" ** ", # 128 - integral
    " *  ",
    " *  ",
    "**  "],
   ["****", # 129 - Gamma (skip Alpha, Beta, they look like A, B)
    "**  ",
    "**  ",
    "**  "],
   ["  * ", # 130 - Delta
    " * *",
    "** *",
    "****"],
   ["****", # 131 - Theta (skip Epsilon, Zeta, Eta, they look like E, Z, H)
    "** *",
    "* **",
    "****"],
   [" *  ", # 132 - Lambda (skip Iota, Kappa, they look like l, K)
    "* * ",
    "* **",
    "* **"],
   ["****", # 133 - Xi (skip Mu, Nu, they look like M, N)
    " ** ",
    "    ",
    "****"],
   ["****", # 134 - Pi (skip Omicron, it looks like O)
    "* **",
    "* **",
    "* **"],
   ["****", # 135 - Sigma (skip Rho, it looks like P)
    " ** ",
    "*   ",
    "****"],
   ["* **", # 136 - Phi (skip Tau, Upsilon, they look like T, Y)
    " * *",
    "* * ",
    "** *"],
   ["*  *", # 137 - Psi (skip Chi, it looks like X)
    "* * ",
    " *  ",
    "* **"],
   [" ** ", # 138 - Omega
    "*  *",
    "** *",
    "** *"],
   ["", # 139
    "",
    "",
    ""],
   ["", # 140
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", # 144
    "",
    "",
    ""],
   ["   *", # 145 - alpha
    " ** ",
    "* * ",
    "** *"],
   [" ** ", # 146 - beta
    " ** ",
    " * *",
    "****"],
   ["*  *", # 147 - gamma
    " ** ",
    "* * ",
    "**  "],
   [" *  ", # 148 - delta
    " ** ",
    "* **",
    " ** "],
   [" ** ", # 149 - epsilon
    " ***",
    "*   ",
    " ** "],
   [" ***", # 150 - zeta
    "*   ",
    "*** ",
    "   * "],
   ["*** ", # 151 - eta
    "*  *",
    "*  *",
    "   * "],
   [" ** ", # 152 - theta
    "** *",
    "* **",
    " ** "],
   ["    ", # 153 - iota
    " ** ",
    " ** ",
    " ** "],
   ["    ", # 154 - kappa
    "** *",
    "*** ",
    "** *"],
   ["**  ", # 155 - lambda
    "  * ",
    " * *",
    "*  *"],
   ["    ", # 156 - mu
    "*  *",
    "****",
    "   *"],
   ["    ", # 157 - nu
    "** *",
    " ** ",
    " *  "],
   ["**  ", # 158 - xi
    "*** ",
    "*   ",
    " ***"],
   ["    ", # 159 - skip omicron, go to pi
    "****",
    "* * ",
    "* * "],
   ["  * ", # 160 - rho 
    " * *",
    " ** ",
    "**  "],
   ["    ", # 161 - sigma (circle)
    " ***",
    "* * ",
    " *  "],
   [" ** ", # 162 - sigma (open)
    "*   ",
    " ***",
    "   *"],
   ["    ", # 163 - tau
    "****",
    " ** ",
    " ** "],
   ["    ", # 164 - upsilon
    "*  *",
    " * *",
    " ** "],
   ["*   ", # 165 - phi
    " ** ",
    " ** ",
    "   *"],
   ["** *", # 166 - chi
    " ** ",
    "* * ",
    "  **"],
   [" * *", # 167 - psi
    " ** ",
    " ***",
    "*   "],
   ["    ", # 168 - omega
    "*  *",
    "** *",
    " ** "],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["", 
    "",
    "",
    ""],
   ["*   ", # 212 - start of sine jump
    " *  ",
    "  **",
    "    "],
   ["    ", # 213 - end of sine jump
    "**  ",
    "  * ",
    "   *"],
   ["    ", # 214 - start of split-saw
    "    ",
    " ***",
    "*   "],
   ["   *", # 215 - end of split-saw
    "  * ",
    "**  ",
    "    "],
   ["*  *", # 216 - inverted sine up
    " ** ",
    "    ",
    "    "],
   ["    ", # 217 - inverted sine down
    "    ",
    " ** ",
    "*  *"],
   ["*  *", # 218 - drop
    " *  ",
    "  * ",
    "    "],
   ["    ", # 219 - ramp
    "  * ",
    " *  ",
    "*  *"],
   ["    ", # 220 - lowering mountain
    " *  ",
    "* * ",
    "   *"],
   ["   *", # 221 - rising valley
    "* * ",
    " *  ",
    "    "],
   ["  * ", # 222 - rising mountain
    " * *",
    "*   ",
    "    "],
   ["    ", # 223 - lowering valley
    "*   ",
    " * *",
    "  * "],
   ["****", # 224 - RANDOMIZE 0
    "****",
    "****",
    "****"],
   ["* * ", # 225 - RANDOMIZE 1
    " * *",
    "* * ",
    " * *"],
   [" * *", # 226 - RANDOMIZE 2
    "* * ",
    " * *",
    "* * "],
   ["   *", # 227 - RANDOMIZE 3
    "*   ",
    "    ",
    " *  "],
   ["    ", # 228 - RANDOMIZE 4
    "    ",
    "****",
    "****"],
   ["    ", # 229 - RANDOMIZE 5
    "****",
    "****",
    "    "],
   ["****", # 230 - RANDOMIZE 6
    "****",
    "    ",
    "    "],
   ["****", # 231 - RANDOMIZE 7
    "    ",
    "    ",
    "****"],
   ["    ", # 232 - RANDOMIZE 8
    "    ",
    "    ",
    "****"],
   ["    ", # 233 - RANDOMIZE 9
    "    ",
    "****",
    "    "],
   ["    ", # 234 - RANDOMIZE 10
    "****",
    "    ",
    "    "],
   ["****", # 235 - RANDOMIZE 11
    "    ",
    "    ",
    "    "],
   ["*   ", # 236 - RANDOMIZE 12
    "*   ",
    "*   ",
    "*   "],
   [" *  ", # 237 - RANDOMIZE 13
    " *  ",
    " *  ",
    " *  "],
   ["  * ", # 238 - RANDOMIZE 14
    "  * ",
    "  * ",
    "  * "],
   ["   *", # 239 - RANDOMIZE 15
    "   *",
    "   *",
    "   *"], 
   ["    ", # 240 -> zero, require empty
    "    ",
    "    ",
    "    "],
   ["    ", # 241
    "    ",
    "    ",
    "****"],
   ["    ", # 242
    "    ",
    "****",
    "    "],
   ["    ", # 243
    "    ",
    "****",
    "****"],
   ["    ", # 244
    "****",
    "    ",
    "    "],
   ["    ", # 245
    "****",
    "    ",
    "****"],
   ["    ", # 246
    "****",
    "****",
    "    "],
   ["    ", # 247
    "****",
    "****",
    "****"],
   ["****", # 248
    "    ",
    "    ",
    "    "],
   ["****", # 249
    "    ",
    "    ",
    "****"],
   ["****", # 250
    "    ",
    "****",
    "    "],
   ["****", # 251
    "    ",
    "****",
    "****"],
   ["****", # 252
    "****",
    "    ",
    "    "],
   ["****", # 253
    "****",
    "    ",
    "****"],
   ["****", # 254 
    "****",
    "****",
    "    "],
   ["****", # 255
    "****",
    "****",
    "****"],
]

with open("src/font.c", 'w') as f:
    f.write("//AUTOGENERATED BY mk_font.py\n\n")
    f.write('#include "bitbox.h"\n#include "font.h"\n#include "game.h"\n#include <string.h>\n')
    f.write("uint16_t font[256] CCM_MEMORY;\n")
    f.write("uint16_t font_cache[256] = {\n")
    if starting_index:
        f.write("[%d]="%starting_index)
    else:
        f.write("  ")
    char_set = set()
    for i in range(len(characters)):
        char = characters[i]
        x = 0
        for j in range(len(char)):
            power = 4*j
            for c in char[j]:
                if c != ' ':
                    x |= 1<<power
                power += 1
        if x != 0 and x in char_set and i < 224: # randoms can redo earlier things
            raise Exception('character at %d already in set'%i)
        char_set.add(x)
        if i + 1 == len(characters):
            f.write("%d\n"%x)
        else:
            f.write("%d,\n  "%x)
    f.write("};\n")
    if len(characters) + starting_index > 256:
        print "WARNING, overflow!"
    f.write("""
void font_init()
{   // Initializes the font cache.  make sure to call in game_init()
    memcpy(font, font_cache, sizeof(font_cache));
}
/* //TODO: 
void font_render_character(uint8_t character, int x, int delta_y, uint16_t color_fg, uint16_t color_bg)
*/
void font_render_line_doubled(const uint8_t *text, int x, int delta_y, uint16_t color_fg, uint16_t color_bg)
{   // Renders a line of font that is 2x the font size, with foreground and background color
    // Top-left of text begins at position at (x, vga_line - delta_y) onscreen.
    #ifdef EMULATOR
    if (delta_y < 0 || delta_y >= 8)
    {
        message("invalid internal line for text %s: %d\\n", (const char *)text, delta_y);
        bitbox_die(-1, 0);
        return;
    }
    int text_end_x = x + 9*strlen((const char *)text);
    if (x < 0 || text_end_x >= SCREEN_W)
    {
        message("text (%s) goes off screen! %d<->%d max %d\\n", (const char *)text, x, text_end_x, SCREEN_W);
        for (int i = 0; i < 40; ++i) {
            message(" %d -> %c\\n", i, text[i]);
            if (text[i] == 0) break;
        }
        bitbox_die(-1, 0);
        return;
    }
    #endif
    delta_y = ((delta_y/2))*4; // make delta_y now how much to shift
    uint16_t *dst = draw_buffer + x;
    uint16_t color_choice[2] = { color_bg, color_fg };
    *dst = color_choice[0];
    --text;
    int c;
    while ((c = *(++text)))
    {
        uint8_t row = (font[c] >> delta_y) & 15;
        for (int j=0; j<4; ++j)
        {
            *(++dst) = color_choice[row&1];
            *(++dst) = color_choice[row&1];
            row >>= 1;
        }
        *(++dst) = color_choice[0];
    }
}

void font_render_no_bg_line_doubled(const uint8_t *text, int x, int delta_y, uint16_t color_fg)
{   // Renders a line of font that is 2x the font size, with foreground color only; background color is transparent.
    // Ensure enough contrast on the screen with the existing background color(s), please!
    // Top-left of text begins at position at (x, vga_line - delta_y) onscreen.
    #ifdef EMULATOR
    if (delta_y < 0 || delta_y >= 8)
    {
        message("invalid internal line for text %s: %d\\n", (const char *)text, delta_y);
        bitbox_die(-1, 0);
        return;
    }
    int text_end_x = x + 9*strlen((const char *)text);
    if (x < 0 || text_end_x >= SCREEN_W)
    {
        message("text (%s) goes off screen! %d<->%d max %d\\n", (const char *)text, x, text_end_x, SCREEN_W);
        for (int i = 0; i < 40; ++i) {
            message(" %d -> %c\\n", i, text[i]);
            if (text[i] == 0) break;
        }
        bitbox_die(-1, 0);
        return;
    }
    #endif
    delta_y = ((delta_y/2))*4; // make delta_y now how much to shift
    uint16_t *dst = draw_buffer + x;
    --text;
    int c;
    while ((c = *(++text)))
    {
        uint8_t row = (font[c] >> delta_y) & 15;
        for (int j=0; j<4; ++j)
        {
            if (row&1)
            {
                *(++dst) = color_fg;
                *(++dst) = color_fg;
            }
            else
            {   // don't overwrite current bg color
                dst += 2;
            }
            row >>= 1;
        }
        ++dst;
    }
}
"""
)


with open("src/font.h", 'w') as f:
    f.write("//AUTOGENERATED BY mk_font.py\n\n")
    f.write("#ifndef FONT_H\n#define FONT_H\n#include <stdint.h>\n")
    f.write("extern uint16_t font_cache[256];\nextern uint16_t font[256];\nvoid font_init();\nvoid font_render_line_doubled(const uint8_t *text, int x, int delta_y, uint16_t color_fg, uint16_t color_bg);\n")
    f.write("void font_render_no_bg_line_doubled(const uint8_t *text, int x, int delta_y, uint16_t color_fg);\n");
    f.write("#endif\n")
