#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"


#define DAY_PUBLISHED 20225

// Macros
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// C64 resolution
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 200
#define CELL_SIZE 8
#define HALF_CELL (int)(CELL_SIZE / 2)

// +1 - 2 So scrolling looks seamless and taking into account UI bars
#define GRID_WIDTH (int)(WINDOW_WIDTH / CELL_SIZE - 1)
// +1 So scrolling looks seamless
#define GRID_HEIGHT (int)(WINDOW_HEIGHT / CELL_SIZE + 1)
#define GRID_CELLS (GRID_WIDTH * GRID_HEIGHT)

#define TRAIL_WIDTH (GRID_WIDTH * 2)
#define TRAIL_HEIGHT (GRID_HEIGHT * 2)
#define TRAIL_CELLS (TRAIL_WIDTH * TRAIL_HEIGHT)

#define WINDOW_CAPTION "ZAPZIPZAP"
#define FPS 30  // SHeeeeeshhh, Apple iPhone you really making things difficult
#define MUSIC_VOLUME 0.07f

#define NAME_LEN 3
#define SCORE_LEN 8
#define LUCK 16

#define MIN_DRAG_LENGTH 16.0f
#define MIN_TAP_TICKS 20
#define SECONDS_IN_DAY 86400

#define ZAP_BONUS_SCORE 100
#define ZAP_VISUAL_COOLDOWN 10

#define SCROLL_MINIMUM 10
#define SCROLL_MAXIMUM 30

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


typedef enum {
    MENU,
    GAME,
    SCORE,
} SCENE;

// Ordering of characters in spritesheet
typedef enum {
    C64_BACKWARD,
    C64_FORWARD,
    C64_TRAIL_SW,
    C64_TRAIL_NW,
    C64_TRAIL_NE,
    C64_TRAIL_SE,
    C64_PLAYER,
    C64_COLLECTIBLE,
    C64_BOLT,
    C64_TROPHY,
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
    C64_EXCLAMATION,
    C64_COLON,
    C64_APOSTROPHE,
    C64_UP_ARROW,
    C64_DOWN_ARROW,
    C64_SOLID,
    C64_BLANK,
} C64_CHARACTER;

typedef enum {
    WALL_BROKEN,
    WALL_BACKWARD,
    WALL_FORWARD,
} WALL_STATE;

typedef enum {
    NO_DIRECTION,
    NORTH_EAST,
    SOUTH_EAST,
    SOUTH_WEST,
    NORTH_WEST,
} MOVE_DIRECTION;

typedef enum {
    NORTH,
    EAST,
    SOUTH,
    WEST,
} SCROLL_DIRECTION;


typedef struct {
    float screen_width;
    float screen_height;
    float scale;
    float offset_x;
    float offset_y;
} WindowParameters;

typedef struct {
    MOVE_DIRECTION direction;
    bool mouse_down;
    bool mouse_clicked;
    bool select;
    uint32_t held_timer;
    Vector2 mouse_pos;
    Vector2 drag_start;
} InputBuffer;

typedef struct {
    int day;
    int hours;
    int minutes;
    int seconds;
} DateBuffer;
