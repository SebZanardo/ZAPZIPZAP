/*#include <stdio.h>*/
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "raylib.h"
#include "constants.h"


typedef struct {
    float screen_width;
    float screen_height;
    float scale;
    float offset_x;
    float offset_y;
} WindowParameters;


void handle_resize();
void check_for_collectible(int new_index, int *zaps);
int player_index(int x, int y, MOVE_DIRECTION dir);
bool should_stop_zip(int x, int y, MOVE_DIRECTION dir);


// GLOBALS /////////////////////////////////////////////////////////////////////
WindowParameters window;
WALL_STATE grid[GRID_CELLS];
MOVE_DIRECTION trail[TRAIL_CELLS];
bool collectibles[TRAIL_CELLS];

int BOUNDS_X = (GRID_WIDTH - 1) * 2;
int BOUNDS_Y = (GRID_HEIGHT - 1) * 2;

bool scroll = false;

// TODO: Load from web
uint32_t highscore = 360;
char highscore_name[NAME_LEN] = {C64_S, C64_E, C64_B};

uint32_t score = 0;


int main(void) {
// INITIALISATION //////////////////////////////////////////////////////////////
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_CAPTION);
    SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTargetFPS(FPS);
    SetRandomSeed(0);

    Image icon = LoadImage("src/resources/icon.png");
    SetWindowIcon(icon);

    handle_resize();

    // Random starting maze
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1) + 1;
    }

    // NOTE: Both must be even or both must be odd! (37,25) is centre
    int px = 37;
    int py = 25;

    // Setup trail and collectibles
    for (int y = 0; y < TRAIL_HEIGHT; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            trail[y * TRAIL_WIDTH + x] = NO_DIRECTION;
            if (x == px && y == py) continue;
            if (GetRandomValue(0, LUCK) != 0) continue;
            collectibles[y * TRAIL_WIDTH + x] = x % 2 == y % 2;
        }
    }

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    Texture2D spritesheet = LoadTexture("src/resources/fontsheet.png");
    Vector2 pos = (Vector2) {0, 0};
    Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};


    MOVE_DIRECTION direction = NO_DIRECTION;
    int zaps = 3;
    int zap_cooldown = 10;
    int zap_cooldown_counter = 0;
    int steps = 0;
    int new_index = 0;
    int ox;
    int oy;

// GAME LOOP ///////////////////////////////////////////////////////////////////
    while (!WindowShouldClose()) {
        // PRE-UPDATE //////////////////////////////////////////////////////////
        if (IsWindowResized()) handle_resize();

        // INPUT ///////////////////////////////////////////////////////////////
        direction = NO_DIRECTION;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_Z)) {
            direction = SOUTH_WEST;
            ox = -1;
            oy = 1;
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_P)) {
            direction = NORTH_EAST;
            ox = 1;
            oy = -1;
        } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_Q)) {
            direction = NORTH_WEST;
            ox = -1;
            oy = -1;
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_COMMA)) {
            direction = SOUTH_EAST;
            ox = 1;
            oy = 1;
        }

        // UPDATE //////////////////////////////////////////////////////////////
        steps = 0;
        if (direction != NO_DIRECTION) {
            // ZIP
            while (1) {
                if (should_stop_zip(px, py, direction)) break;
                new_index = (py + oy) * TRAIL_WIDTH + (px + ox);
                if (trail[new_index] != NO_DIRECTION) {
                    // NOTE: Hit into trail
                    break;
                }
                check_for_collectible(new_index, &zaps);
                px += ox;
                py += oy;
                trail[py * TRAIL_WIDTH + px] = direction;
                steps++;
            }

            // ZAP
            if (steps == 0 && zaps > 0) {
                zap_cooldown_counter = zap_cooldown;
                grid[player_index(px, py, direction)] = WALL_BROKEN;
                zaps--;
            }
        }

        if (zap_cooldown_counter > 0) {
            zap_cooldown_counter--;
        }

        if (scroll) {
            score++;
        } else {
            score += steps;
        }

        // RENDER //////////////////////////////////////////////////////////////
        BeginTextureMode(target);
        ClearBackground(C64_BLUE);

        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                pos.x = x * CELL_SIZE + CELL_SIZE;
                pos.y = y * CELL_SIZE;

                if (grid[y * GRID_WIDTH + x] == WALL_BROKEN) continue;

                char_rect.x = C64_BACKWARD * CELL_SIZE;
                if (grid[y * GRID_WIDTH + x] == WALL_FORWARD) {
                    char_rect.x = C64_FORWARD * CELL_SIZE;
                }

                DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_BLUE);
            }
        }

        for (int y = 0; y < TRAIL_HEIGHT; y++) {
            for (int x = 0; x < TRAIL_WIDTH; x++) {
                pos.x = (int)(x * HALF_CELL + CELL_SIZE);
                pos.y = (int)(y * HALF_CELL - HALF_CELL);

                MOVE_DIRECTION dir = trail[y * TRAIL_WIDTH + x];
                if (dir != NO_DIRECTION) {
                    char_rect.x = (dir - 1 + C64_TRAIL_NE) * CELL_SIZE;
                    DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_GREY);
                }

                char_rect.x = C64_COLLECTIBLE * CELL_SIZE;
                if (collectibles[y * TRAIL_WIDTH + x]) {
                    DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
                }
            }
        }

        pos.x = (int)(px * HALF_CELL + CELL_SIZE);
        pos.y = (int)(py * HALF_CELL - HALF_CELL);
        char_rect.x = C64_PLAYER * CELL_SIZE;
        DrawTextureRec(spritesheet, char_rect, pos, zap_cooldown_counter > 0 ? C64_YELLOW : C64_WHITE);

        // To cover scrolling maze
        DrawRectangle(0, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);
        DrawRectangle(WINDOW_WIDTH - CELL_SIZE, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);

        // Zaps left
        char_rect.x = C64_BOLT * CELL_SIZE;
        pos.x = WINDOW_WIDTH - CELL_SIZE;
        for (int i = 0; i < zaps; i++) {
            pos.y = i * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
        }

        // Score
        pos.x = 0;
        pos.y = 0;
        for (int i = 0; i < NAME_LEN; i++) {
            char_rect.x = highscore_name[i] * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
            pos.y += CELL_SIZE;
        }

        pos.y += CELL_SIZE; // GAP

        // Highscore
        pos.y = CELL_SIZE * 11;
        int div = 1;
        for (int i = 0; i < SCORE_LEN; i++) {
            char_rect.x = ((highscore / div) % 10 + C64_0) * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, WHITE);
            pos.y -= CELL_SIZE;
            div *= 10;
        }
        pos.y = CELL_SIZE * 12;

        pos.y += CELL_SIZE * 2; // GAP * 2
        if (score > highscore) {
            char_rect.x = C64_TROPHY * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
        } else {
            pos.y += CELL_SIZE; // GAP
        }
        pos.y += CELL_SIZE * 2; // GAP * 2

        // Current score
        pos.y = CELL_SIZE * 24;
        div = 1;
        for (int i = 0; i < SCORE_LEN; i++) {
            char_rect.x = ((score / div) % 10 + C64_0) * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, WHITE);
            pos.y -= CELL_SIZE;
            div *= 10;
        }

        EndTextureMode();

        // Upscaling render target to window
        BeginDrawing();
        ClearBackground(C64_LIGHT_BLUE);

        DrawTexturePro(
            target.texture,
            (Rectangle){
                0,
                0,
                (float)target.texture.width,
                (float)-target.texture.height
            },
            (Rectangle){
                window.offset_x,
                window.offset_y,
                (float)target.texture.width * window.scale,
                (float)-target.texture.height * window.scale
            },
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        /*DrawFPS(0, 0);*/
        EndDrawing();
    }

    // DEINITIALISE ////////////////////////////////////////////////////////////
    UnloadImage(icon);
    UnloadTexture(spritesheet);

    CloseWindow();
}


void handle_resize() {
    window.screen_width = GetScreenWidth();
    window.screen_height = GetScreenHeight();
    window.scale = MIN(
        window.screen_width / WINDOW_WIDTH,
        window.screen_height / WINDOW_HEIGHT
    );
    window.offset_x = (window.screen_width - window.scale * WINDOW_WIDTH) / 2;
    window.offset_y = (window.screen_height - window.scale * WINDOW_HEIGHT) / 2;
}


void check_for_collectible(int new_index, int *zaps) {
    if (collectibles[new_index]) {
        collectibles[new_index] = false;
        (*zaps)++;
        *zaps = MIN(*zaps, GRID_HEIGHT - 1);
        score += ZAP_BONUS_SCORE;
        // NOTE: Collected zap
    }
}


int player_index(int x, int y, MOVE_DIRECTION dir) {
    switch (dir) {
        case NO_DIRECTION:
            break;
        case NORTH_EAST:
            return (int)((y - 1) / 2) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_EAST:
            return (int)(y / 2) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_WEST:
            return (int)(y / 2) * GRID_WIDTH + (int)(x / 2);
        case NORTH_WEST:
            return (int)((y - 1) / 2) * GRID_WIDTH + (int)(x / 2);
    }
    return -1;
}


bool should_stop_zip(int x, int y, MOVE_DIRECTION dir) {
    switch (dir) {
        case NO_DIRECTION:
            break;
        case SOUTH_WEST:
            return x <= 0 || y >= BOUNDS_Y || grid[player_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_EAST:
            return x >= BOUNDS_X || y <= 0 || grid[player_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_WEST:
            return x <= 0 || y <= 0 || grid[player_index(x, y, dir)] == WALL_FORWARD;
        case SOUTH_EAST:
            return x >= BOUNDS_X || y >= BOUNDS_Y || grid[player_index(x, y, dir)] == WALL_FORWARD;
    }
    return true;
}
