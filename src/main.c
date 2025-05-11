/*#include <stdio.h>*/
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "raylib.h"
#include "constants.h"


typedef struct {
    int screen_width; int screen_height;
    int scale;
    int offset_x;
    int offset_y;
} WindowParameters;


void handle_resize(WindowParameters *window);
void check_for_collectible(bool collectibles[TRAIL_CELLS], int new_index, int *zaps);
int new_player_position(int x, int y, MOVE_DIRECTION dir);


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_CAPTION);
    SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTargetFPS(FPS);
    SetRandomSeed(0);
    HideCursor();

    WindowParameters window;
    handle_resize(&window);

    // Random starting maze
    WALL_STATE grid[GRID_CELLS];
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1) + 1;
    }

    // NOTE: Both must be even or both must be odd!
    int px = 37;
    int py = 25;

    // Setup trail and collectibles
    MOVE_DIRECTION trail[TRAIL_CELLS];
    bool collectibles[TRAIL_CELLS];

    for (int y = 0; y < TRAIL_HEIGHT; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            trail[y * TRAIL_WIDTH + x] = NO_DIRECTION;
            if (x == px && y == py) continue;
            if (GetRandomValue(0, LUCK) != 0) continue;
            collectibles[y * TRAIL_WIDTH + x] = x % 2 == y % 2;
        }
    }

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    Texture2D spritesheet = LoadTexture("src/resources/spritesheet.png");
    Vector2 pos = (Vector2) {0, 0};
    Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};

    int BOUNDS_X = GRID_WIDTH * 2;
    int BOUNDS_Y = GRID_HEIGHT * 2;

    MOVE_DIRECTION last_move = NO_DIRECTION;
    uint32_t score = 0;
    uint32_t highscore = 360;
    char highscore_name[NAME_LEN] = {C64_S, C64_E, C64_B};
    int zaps = 3;
    int zap_cooldown = 10;
    int zap_cooldown_counter = 0;
    int steps = 0;
    int new_index = 0;
    bool moved = false;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) handle_resize(&window);

        // INPUT
        moved = true;
        steps = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_Z)) {
            last_move = SOUTH_WEST;
            while (1) {
                if (px <= 0 || py > BOUNDS_Y) break;
                if (grid[new_player_position(px, py, last_move)] == WALL_BACKWARD) break;
                new_index = (py+1) * TRAIL_WIDTH + (px-1);
                if (trail[new_index] != NO_DIRECTION) {
                    last_move = NO_DIRECTION;
                    break;
                }
                check_for_collectible(collectibles, new_index, &zaps);
                px--;
                py++;
                trail[py * TRAIL_WIDTH + px] = last_move;
                steps++;
            }
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_P)) {
            last_move = NORTH_EAST;
            while (1) {
                if (px > BOUNDS_X || py <= 0) break;
                if (grid[new_player_position(px, py, last_move)] == WALL_BACKWARD) break;
                new_index = (py-1) * TRAIL_WIDTH + (px+1);
                if (trail[new_index] != NO_DIRECTION) {
                    last_move = NO_DIRECTION;
                    break;
                }
                check_for_collectible(collectibles, new_index, &zaps);
                px++;
                py--;
                trail[py * TRAIL_WIDTH + px] = last_move;
                steps++;
            }
        } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_Q)) {
            last_move = NORTH_WEST;
            while (1) {
                if (px <= 0 || py <= 0) break;
                if (grid[new_player_position(px, py, last_move)] == WALL_FORWARD) break;
                new_index = (py-1) * TRAIL_WIDTH + (px-1);
                if (trail[new_index] != NO_DIRECTION) {
                    last_move = NO_DIRECTION;
                    break;
                }
                check_for_collectible(collectibles, new_index, &zaps);
                px--;
                py--;
                trail[py * TRAIL_WIDTH + px] = last_move;
                steps++;
            }
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_COMMA)) {
            last_move = SOUTH_EAST;
            while (1) {
                if (px > BOUNDS_X || py > BOUNDS_Y) break;
                if (grid[new_player_position(px, py, last_move)] == WALL_FORWARD) break;
                new_index = (py+1) * TRAIL_WIDTH + (px+1);
                if (trail[new_index] != NO_DIRECTION) {
                    last_move = NO_DIRECTION;
                    break;
                }
                check_for_collectible(collectibles, new_index, &zaps);
                px++;
                py++;
                trail[py * TRAIL_WIDTH + px] = last_move;
                steps++;
            }
        } else {
            moved = false;
        }

        // UPDATE
        if (moved) {
            if (last_move == NO_DIRECTION) {
                // NOTE: Hit into tail
            } else if (steps == 0 && zaps > 0) {
                zap_cooldown_counter = zap_cooldown;
                grid[new_player_position(px, py, last_move)] = WALL_BROKEN;
                zaps--;
            }
        }

        if (zap_cooldown_counter > 0) {
            zap_cooldown_counter--;
        }

        score++;

        // RENDER
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

    // DEINITIALISE
    UnloadTexture(spritesheet);
    CloseWindow();
}


void handle_resize(WindowParameters *window) {
    window->screen_width = GetScreenWidth();
    window->screen_height = GetScreenHeight();
    window->scale = MIN(
        window->screen_width / WINDOW_WIDTH,
        window->screen_height / WINDOW_HEIGHT
    );
    window->offset_x = (window->screen_width - window->scale * WINDOW_WIDTH) / 2;
    window->offset_y = (window->screen_height - window->scale * WINDOW_HEIGHT) / 2;
}


void check_for_collectible(bool collectibles[TRAIL_CELLS], int new_index, int *zaps) {
    if (collectibles[new_index]) {
        collectibles[new_index] = false;
        (*zaps)++;
        *zaps = MIN(*zaps, GRID_HEIGHT - 1);
    }
}


int new_player_position(int x, int y, MOVE_DIRECTION dir) {
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
    assert(true == false);
    return -1;
}
