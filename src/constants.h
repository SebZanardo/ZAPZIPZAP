// Macros
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// C64 resolution
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 200
#define CELL_SIZE 8

#define WINDOW_CAPTION "alternate"
#define FPS 60


// C64 colours
// Colour names from here: https://sta.c64.org/cbm64col.html
#define C64_BLACK (Color) {0, 0, 0, 255}
#define C64_WHITE (Color) {255, 255, 255, 255}
#define C64_RED (Color) {136, 57, 50, 255}
#define C64_CYAN (Color) {103, 182, 109, 255}
#define C64_PURPLE (Color) {139, 64, 150, 255}
#define C64_GREEN (Color) {85, 160, 73, 255}
#define C64_BLUE (Color) {64, 49, 141, 255}
#define C64_YELLOW (Color) {191, 206, 114, 255}
#define C64_ORANGE (Color) {139, 84, 41, 255}
#define C64_BROWN (Color) {87, 66, 0, 255}
#define C64_PINK (Color) {184, 105, 98, 255}
#define C64_DARK_GREY (Color) {80, 80, 80, 255}
#define C64_GREY (Color) {120, 120, 120, 255}
#define C64_LIGHT_GREEN (Color) {148, 224, 137, 255}
#define C64_LIGHT_BLUE (Color) {120, 105, 196, 255}
#define C64_LIGHT_GREY (Color) {159, 159, 159, 255}

// Ordering of characters in spritesheet
typedef enum {
    C64_FORWARD,
    C64_BACKWARD,
    C64_PLAYER,
    C64_BOLT,
    C64_A,
    C64_B,
    C64_C,
    C64_D,
    C64_E,
    C64_F,
    C64_G,
    C64_H,
    C64_I,
    C64_J,
    C64_K,
    C64_L,
    C64_M,
    C64_N,
    C64_O,
    C64_P,
    C64_Q,
    C64_R,
    C64_S,
    C64_T,
    C64_U,
    C64_V,
    C64_W,
    C64_X,
    C64_Y,
    C64_Z,
    C64_0,
    C64_1,
    C64_2,
    C64_3,
    C64_4,
    C64_5,
    C64_6,
    C64_7,
    C64_8,
    C64_9,
} C64_CHARACTERS;
