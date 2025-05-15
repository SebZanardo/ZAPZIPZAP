/*#include <stdio.h>*/
/*#include <assert.h>*/
#include <stdint.h>
#include <stdbool.h>
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
void check_for_collectible(int new_index);
int player_index(int x, int y, MOVE_DIRECTION dir);
bool should_stop_zip(int x, int y, MOVE_DIRECTION dir);
void new_maze_row();


// GLOBALS /////////////////////////////////////////////////////////////////////
WindowParameters window;
WALL_STATE grid[GRID_CELLS];

// TODO: Would be good to make these only GRID_CELLS in length for easy scroll
MOVE_DIRECTION trail[TRAIL_CELLS];
bool collectibles[TRAIL_CELLS];

int BOUNDS_X = (GRID_WIDTH - 2) * 2;
int BOUNDS_Y = (GRID_HEIGHT - 2) * 2;

bool is_action_mode = true;

// TODO: Load from web
uint32_t highscore = 360;
char highscore_name[NAME_LEN] = {C64_S, C64_E, C64_B};

uint32_t score = 0;
uint32_t ticks = 0;
bool gameover = false;

int row_index = 0; // Points to the first row to be rendered
SCROLL_DIRECTION scroll_dir = NORTH;
int scroll_counter = 40;
int scroll_x = 0;
int scroll_y = 0;
int scroll_speed = 4;

// NOTE: Both must be even or both must be odd! (37,25) is centre.
// TODO: Would be good to change to proper x, y system...
int px = 37;
int py = 25;
int zaps = 100;


int main(void) {
    ////////////////////////////////////////////////////////////////////////////
    // INITIALISATION //////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_CAPTION);
    SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTargetFPS(FPS);

    // TODO: Get seed from day
    SetRandomSeed(0);

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    Texture2D spritesheet = LoadTexture("src/resources/fontsheet.png");
    Image icon = LoadImage("src/resources/icon.png");

    SetWindowIcon(icon);
    handle_resize();

    Vector2 pos = (Vector2) {0, 0};
    Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};

    MOVE_DIRECTION direction = NO_DIRECTION;
    int zap_cooldown = 10;
    int zap_cooldown_counter = 0;
    int steps = 0;
    int new_index = 0;
    int ox = 0;
    int oy = 0;

    // Random starting maze
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1) + 1;
    }

    // Setup trail and collectibles
    for (int y = 0; y < TRAIL_HEIGHT; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            new_index = y * TRAIL_WIDTH + x;
            trail[new_index] = NO_DIRECTION;
            if (x % 2 == y % 2) {
                collectibles[new_index] = false;
                if (GetRandomValue(0, LUCK) == 0) {
                    collectibles[new_index] = true;
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // GAME LOOP ///////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    while (!WindowShouldClose()) {
        // PRE-UPDATE //////////////////////////////////////////////////////////
        if (IsWindowResized()) handle_resize();
        if (px < 0 || px > TRAIL_WIDTH || py < 0 || py > TRAIL_HEIGHT) {
            gameover = true;
        }

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
        if (!gameover) {
            steps = 0;
            if (direction != NO_DIRECTION) {
                // ZIP
                while (1) {
                    if (should_stop_zip(px, py, direction)) break;
                    new_index = ((py + oy + row_index * 2) % TRAIL_HEIGHT) * TRAIL_WIDTH + (px + ox);
                    if (trail[new_index] != NO_DIRECTION) {
                        // NOTE: Hit into trail
                        break;
                    }
                    check_for_collectible(new_index);
                    trail[((py + row_index * 2) % TRAIL_HEIGHT) * TRAIL_WIDTH + px] = direction;
                    px += ox;
                    py += oy;
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

            if (is_action_mode) {
                score++;

                if (ticks % scroll_speed == 0) {
                    if (scroll_counter > 0) {
                        if (scroll_dir == NORTH || scroll_dir == EAST) {
                            scroll_y++;
                            if (scroll_y >= CELL_SIZE) {
                                new_maze_row();

                                row_index++;
                                if (row_index >= GRID_HEIGHT) {
                                    row_index = 0;
                                }

                                scroll_y = 0;
                                py -= 2;
                            }
                        } else if (scroll_dir == SOUTH || scroll_dir == WEST) {
                            scroll_y--;
                            if (scroll_y < 0) {
                                row_index--;

                                if (row_index < 0) {
                                    row_index = GRID_HEIGHT - 1;
                                }

                                new_maze_row();

                                scroll_y = CELL_SIZE;
                                py += 2;
                            }
                        }
                    } else {
                        // Pick random scroll speed
                        scroll_speed = GetRandomValue(SCROLL_SLOWEST, SCROLL_FASTEST);

                        // Pick new random scroll direction
                        scroll_dir = GetRandomValue(0, 3);

                        // Pick new random scroll counter
                        scroll_counter = CELL_SIZE * GetRandomValue(SCROLL_MINIMUM, SCROLL_MAXIMUM);
                    }
                    scroll_counter--;
                }
            } else {
                score += steps;
            }
        }

        // RENDER //////////////////////////////////////////////////////////////
        BeginTextureMode(target);

        ClearBackground(C64_BLUE);

        // Maze
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                pos.x = x * CELL_SIZE + CELL_SIZE - scroll_x;
                pos.y = y * CELL_SIZE - scroll_y;

                new_index = ((row_index + y) % GRID_HEIGHT) * GRID_WIDTH + x;
                if (grid[new_index] == WALL_BROKEN) continue;

                char_rect.x = C64_BACKWARD * CELL_SIZE;
                if (grid[new_index] == WALL_FORWARD) {
                    char_rect.x = C64_FORWARD * CELL_SIZE;
                }

                DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_BLUE);
            }
        }

        // Trail and collectibles
        for (int y = 0; y < TRAIL_HEIGHT; y++) {
            for (int x = 0; x < TRAIL_WIDTH; x++) {
                pos.x = x * HALF_CELL + CELL_SIZE - scroll_x;
                pos.y = y * HALF_CELL - HALF_CELL - scroll_y;

                new_index = ((y + row_index * 2) % TRAIL_HEIGHT) * TRAIL_WIDTH + x;

                char_rect.x = C64_COLLECTIBLE * CELL_SIZE;
                if (collectibles[new_index]) {
                    DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
                }

                /*pos.x += 2;*/
                /*pos.y += 2;*/
                if (trail[new_index] != NO_DIRECTION) {
                    char_rect.x = (trail[new_index] - 1 + C64_TRAIL_SW) * CELL_SIZE;
                    DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_GREY);
                }
            }
        }

        // Player
        pos.x = px * HALF_CELL + CELL_SIZE - scroll_x;
        pos.y = py * HALF_CELL - HALF_CELL - scroll_y;
        char_rect.x = C64_PLAYER * CELL_SIZE;
        DrawTextureRec(spritesheet, char_rect, pos, zap_cooldown_counter > 0 ? C64_YELLOW : C64_WHITE);

        // To cover scrolling maze and for UI
        DrawRectangle(0, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);
        DrawRectangle(WINDOW_WIDTH - CELL_SIZE, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);

        // Zaps left
        char_rect.x = C64_BOLT * CELL_SIZE;
        pos.x = WINDOW_WIDTH - CELL_SIZE;
        pos.y = 0;
        for (int i = 0; i < zaps; i++) {
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
            pos.y += CELL_SIZE;
        }

        // Score
        pos.x = 0;
        pos.y = 0;
        for (int i = 0; i < NAME_LEN; i++) {
            char_rect.x = highscore_name[i] * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
            pos.y += CELL_SIZE;
        }

        // Highscore
        pos.y = CELL_SIZE * 11;
        int div = 1;
        for (int i = 0; i < SCORE_LEN; i++) {
            char_rect.x = ((highscore / div) % 10 + C64_0) * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, WHITE);
            pos.y -= CELL_SIZE;
            div *= 10;
        }

        // Trophy
        pos.y = CELL_SIZE * 14;
        if (score > highscore) {
            char_rect.x = C64_TROPHY * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
        } else {
            pos.y += CELL_SIZE; // GAP
        }

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

        // POST-UPDATE /////////////////////////////////////////////////////////
        ticks++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // DEINITIALISATION ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
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


void check_for_collectible(int new_index) {
    if (collectibles[new_index]) {
        // NOTE: Collected zap
        collectibles[new_index] = false;
        zaps++;
        zaps = MIN(zaps, GRID_HEIGHT - 1);
        score += ZAP_BONUS_SCORE;
    }
}


int player_index(int x, int y, MOVE_DIRECTION dir) {
    switch (dir) {
        case NO_DIRECTION:
            break;
        case NORTH_EAST:
            return ((int)((y - 1) / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_EAST:
            return ((int)(y / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_WEST:
            return ((int)(y / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)(x / 2);
        case NORTH_WEST:
            return ((int)((y - 1) / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)(x / 2);
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


void new_maze_row() {
    // New maze row
    for (int x = 0; x < GRID_WIDTH; x++) {
        grid[row_index * GRID_WIDTH + x] = GetRandomValue(0, 1) + 1;
    }

    // Clear trail, generate new collectibles
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            int index = (row_index * 2 + y) * TRAIL_WIDTH + x;
            trail[index] = NO_DIRECTION;
            collectibles[index] = false;
            if (x % 2 == y % 2) {
                if (GetRandomValue(0, LUCK) != 0) continue;
                collectibles[index] = true;
            }
        }
    }
}

