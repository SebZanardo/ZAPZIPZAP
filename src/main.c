/*#include <stdio.h>*/
#include "constants.h"


#ifdef WEB_BUILD
#include <emscripten/emscripten.h>

EM_JS(bool, IsMobile, (), {
    if (typeof navigator !== 'undefined') {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    }
    return false;
});
#endif


void handle_resize();
void check_for_collectible(int new_index);
int player_index(int x, int y);
int player_maze_index(int x, int y, MOVE_DIRECTION dir);
bool should_stop_zip(int x, int y, MOVE_DIRECTION dir);
void new_maze_row();
void new_maze_col();
void new_scroll_direction();
void update_time_and_seed();
void update_input_buffer();
void render_menu();
void render_game();
void render_maze();
void render_integer(Vector2 pos, int value, int length, bool vertical, Color colour);
// TODO: this takes a string
void render_string(Vector2 pos, C64_CHARACTER* s, int length, bool vertical, Color colour);
// TODO: make a function to map string to C64_CHARACTER*

// GLOBALS /////////////////////////////////////////////////////////////////////
WindowParameters window;
InputBuffer input_buffer;
DateBuffer date_buffer;

uint32_t seed;
uint32_t ticks = 0;

// ASSETS
Music music;
Texture2D spritesheet;
Texture2D buttons;
Texture2D logo;
Image icon;

Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};

SCENE scene = MENU;

WALL_STATE grid[GRID_CELLS];

MOVE_DIRECTION trail[TRAIL_CELLS];
bool collectibles[TRAIL_CELLS];

int BOUNDS_X = (GRID_WIDTH - 1) * 2;
int BOUNDS_Y = (GRID_HEIGHT - 1) * 2;

bool is_action_mode = true;

// TODO: Load from web or from file
uint32_t puzzle_highscore = 360;
C64_CHARACTER puzzle_highscore_name[NAME_LEN] = {C64_S, C64_E, C64_B};

uint32_t action_highscore = 999;
C64_CHARACTER action_highscore_name[NAME_LEN] = {C64_Z, C64_E, C64_N};

uint32_t score = 0;
bool gameover = false;

int row_index = 0; // Points to the first row to be rendered
int col_index = 0; // Points to the first col to be rendered
SCROLL_DIRECTION scroll_dir = NORTH;
int scroll_counter = 40;
int scroll_x = 0;
int scroll_y = 0;
int scroll_speed = 1;

// NOTE: Both must be even or both must be odd! (37,25) is centre.
int px = 37;
int py = 25;
int zaps = 8;
int zap_cooldown_counter = 0;


int main(void) {
    ////////////////////////////////////////////////////////////////////////////
    // INITIALISATION //////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_CAPTION);
    SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTargetFPS(FPS);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(4096);
    music = LoadMusicStream("src/resources/demo.mp3");
    SetMusicVolume(music, MUSIC_VOLUME);
    PlayMusicStream(music);

    #ifdef WEB_BUILD
        bool handheld = IsMobile();
    #endif

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    spritesheet = LoadTexture("src/resources/fontsheet.png");
    buttons = LoadTexture("src/resources/buttons.png");
    logo = LoadTexture("src/resources/logo.png");
    icon = LoadImage("src/resources/icon.png");

    SetWindowIcon(icon);
    handle_resize();

    // Get random seed from time
    update_time_and_seed();

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

    // Pick scroll direction
    new_scroll_direction();


    ////////////////////////////////////////////////////////////////////////////
    // GAME LOOP ///////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    while (!WindowShouldClose()) {
        // PRE-UPDATE //////////////////////////////////////////////////////////
        UpdateMusicStream(music);
        if (IsWindowResized()) handle_resize();
        if (px < 0 || px > TRAIL_WIDTH || py < 0 || py > TRAIL_HEIGHT) {
            gameover = true;
        }

        // INPUT ///////////////////////////////////////////////////////////////
        update_input_buffer();

        // Set offset direction
        switch (input_buffer.direction) {
            case SOUTH_WEST:
                ox = -1;
                oy = 1;
                break;
            case NORTH_EAST:
                ox = 1;
                oy = -1;
                break;
            case NORTH_WEST:
                ox = -1;
                oy = -1;
                break;
            case SOUTH_EAST:
                ox = 1;
                oy = 1;
                break;
            default:
                ox = 0;
                oy = 0;
        }

        // UPDATE //////////////////////////////////////////////////////////////
        switch (scene) {
            case MENU:
                update_time_and_seed();
                break;
            case GAME:
                if (!gameover) {
                    steps = 0;
                    if (input_buffer.direction != NO_DIRECTION) {
                        // ZIP
                        while (1) {
                            if (should_stop_zip(px, py, input_buffer.direction)) break;
                            new_index = player_index(px + ox, py + oy);
                            if (trail[new_index] != NO_DIRECTION) {
                                // NOTE: Hit into trail (BONK)
                                input_buffer.direction = NO_DIRECTION;
                                break;
                            }
                            check_for_collectible(new_index);
                            trail[player_index(px, py)] = input_buffer.direction;
                            px += ox;
                            py += oy;
                            steps++;
                        }
                    }

                    // ZAP
                    if (steps == 0 && input_buffer.direction != NO_DIRECTION) {
                        new_index = player_maze_index(px, py, input_buffer.direction);
                        if (zaps > 0 && grid[new_index] != WALL_BROKEN) {
                            // NOTE: Broke wall (DESTROY)
                            grid[new_index] = WALL_BROKEN;
                            zap_cooldown_counter = ZAP_VISUAL_COOLDOWN;
                            zaps--;
                        } else {
                            // NOTE: No wall to break, edge of map or trail (BONK)
                        }
                    } else {
                        // NOTE: (ZIP SOUND pick random pitch)
                    }

                    if (zap_cooldown_counter > 0) {
                        zap_cooldown_counter--;
                    }

                    if (is_action_mode) {
                        score++;

                        if (scroll_counter > 0) {
                            if (ticks % scroll_speed == 0) {
                                if (scroll_dir == NORTH) {
                                    scroll_y ++;
                                    if (scroll_y >= CELL_SIZE) {
                                        new_maze_row();

                                        row_index++;
                                        if (row_index >= GRID_HEIGHT) {
                                            row_index = 0;
                                        }

                                        scroll_y = 0;
                                        py -= 2;
                                    }
                                } else if (scroll_dir == SOUTH) {
                                    scroll_y--;
                                    if (scroll_y < 0) {
                                        row_index--;
                                        if (row_index < 0) row_index = GRID_HEIGHT - 1;

                                        new_maze_row();

                                        scroll_y = CELL_SIZE;
                                        py += 2;
                                    }
                                } else if (scroll_dir == EAST) {
                                    scroll_x++;
                                    if (scroll_x >= CELL_SIZE) {
                                        new_maze_col();

                                        col_index++;
                                        if (col_index >= GRID_WIDTH) col_index = 0;

                                        scroll_x = 0;
                                        px -= 2;
                                    }
                                } else if (scroll_dir == WEST) {
                                    scroll_x--;
                                    if (scroll_x < 0) {
                                        col_index--;
                                        if (col_index < 0) col_index = GRID_WIDTH - 1;

                                        new_maze_col();

                                        scroll_x = CELL_SIZE;
                                        px += 2;
                                    }
                                }
                            }
                            scroll_counter--;
                        } else {
                            new_scroll_direction();
                        }
                    } else {
                        score += steps;
                    }
                }
                break;

            case SCORE:
                break;
        }

        // RENDER //////////////////////////////////////////////////////////////
        BeginTextureMode(target);

        switch(scene) {
            case MENU:
                render_menu();
                break;
            case GAME:
                render_game();
                break;
            case SCORE:
                break;
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

        /*DrawCircleV(mouse_pos, 8.0f, C64_GREEN);*/
        /*DrawFPS(0, 0);*/

        EndDrawing();

        // POST-UPDATE /////////////////////////////////////////////////////////
        ticks++;
    }

    ////////////////////////////////////////////////////////////////////////////
    // DEINITIALISATION ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    UnloadMusicStream(music);
    UnloadImage(icon);
    UnloadTexture(spritesheet);
    UnloadTexture(buttons);
    UnloadTexture(logo);

    CloseAudioDevice();
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


int player_index(int x, int y) {
    return ((y + row_index * 2) % TRAIL_HEIGHT) * TRAIL_WIDTH + (x + col_index * 2) % TRAIL_WIDTH;
}


int player_maze_index(int x, int y, MOVE_DIRECTION dir) {
    switch (dir) {
        case NO_DIRECTION:
            break;
        case NORTH_EAST:
            return ((int)((y - 1) / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)((x + 1) / 2 + col_index) % GRID_WIDTH;
        case SOUTH_EAST:
            return ((int)(y / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)((x + 1) / 2 + col_index) % GRID_WIDTH;
        case SOUTH_WEST:
            return ((int)(y / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)(x / 2 + col_index) % GRID_WIDTH;
        case NORTH_WEST:
            return ((int)((y - 1) / 2 + row_index) % GRID_HEIGHT) * GRID_WIDTH + (int)(x / 2 + col_index) % GRID_WIDTH;
    }
    return -1;
}


bool should_stop_zip(int x, int y, MOVE_DIRECTION dir) {
    switch (dir) {
        case NO_DIRECTION:
            break;
        case SOUTH_WEST:
            return x <= 0 || y >= BOUNDS_Y || grid[player_maze_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_EAST:
            return x >= BOUNDS_X || y <= 0 || grid[player_maze_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_WEST:
            return x <= 0 || y <= 0 || grid[player_maze_index(x, y, dir)] == WALL_FORWARD;
        case SOUTH_EAST:
            return x >= BOUNDS_X || y >= BOUNDS_Y || grid[player_maze_index(x, y, dir)] == WALL_FORWARD;
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


void new_maze_col() {
    // New maze row
    for (int y = 0; y < GRID_HEIGHT; y++) {
        grid[y * GRID_WIDTH + col_index] = GetRandomValue(0, 1) + 1;
    }

    // Clear trail, generate new collectibles
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < TRAIL_HEIGHT; y++) {
            int index = y * TRAIL_WIDTH + col_index * 2 + x;
            trail[index] = NO_DIRECTION;
            collectibles[index] = false;
            if (x % 2 == y % 2) {
                if (GetRandomValue(0, LUCK) != 0) continue;
                collectibles[index] = true;
            }
        }
    }
}

void new_scroll_direction() {
    // Pick random scroll speed
    scroll_speed = GetRandomValue(1, 4);

    // Pick new random scroll direction
    SCROLL_DIRECTION random_dir = GetRandomValue(0, 2);

    // Don't pick same scroll directions
    if (random_dir == scroll_dir) {
        scroll_dir = random_dir + 1;
    } else {
        scroll_dir = random_dir;
    }

    // NOTE: PLAY NOTE depending on direction

    // Pick new random scroll counter
    scroll_counter = CELL_SIZE * GetRandomValue(SCROLL_MINIMUM, SCROLL_MAXIMUM);
}


void update_time_and_seed() {
    time_t now = time(NULL);
    date_buffer.day = now / SECONDS_IN_DAY;
    int seconds_until_next_day = SECONDS_IN_DAY - now % SECONDS_IN_DAY;
    date_buffer.hours = seconds_until_next_day / 3600;
    date_buffer.minutes = (seconds_until_next_day % 3600) / 60;
    date_buffer.seconds = seconds_until_next_day % 60;

    if (seed != date_buffer.day) {
        seed = date_buffer.day - DAY_PUBLISHED;
        SetRandomSeed(seed);
    }
}


void update_input_buffer() {
    input_buffer.mouse_pos = GetMousePosition();

    #ifdef WEB_BUILD
        if (!handheld) {
            input_buffer.mouse_pos.x *= window.screen_width / WINDOW_WIDTH;
            input_buffer.mouse_pos.y *= window.screen_height / WINDOW_HEIGHT;
        }
    #endif

    input_buffer.direction = NO_DIRECTION;
    input_buffer.select = false;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !input_buffer.mouse_down) {
        input_buffer.drag_start = input_buffer.mouse_pos;
        input_buffer.mouse_down = true;
        input_buffer.held_timer = 0;
    }

    if (input_buffer.mouse_down) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            // Drag
            Vector2 drag = Vector2Subtract(input_buffer.mouse_pos, input_buffer.drag_start);
            if (Vector2Length(drag) >= MIN_DRAG_LENGTH) {
                if (drag.x >= 0 && drag.y < 0) {
                    input_buffer.direction = NORTH_EAST;
                } else if (drag.x >= 0 && drag.y >= 0) {
                    input_buffer.direction = SOUTH_EAST;
                } else if (drag.x < 0 && drag.y >= 0) {
                    input_buffer.direction = SOUTH_WEST;
                } else if (drag.x < 0 && drag.y < 0) {
                    input_buffer.direction = NORTH_WEST;
                }
            }
            // Tap
            else if (input_buffer.held_timer < MIN_TAP_TICKS) {
                if (
                    input_buffer.mouse_pos.x >= window.screen_width / 2 &&
                    input_buffer.mouse_pos.y < window.screen_height / 2
                ) {
                    input_buffer.direction = NORTH_EAST;
                } else if (
                    input_buffer.mouse_pos.x >= window.screen_width / 2 &&
                    input_buffer.mouse_pos.y >= window.screen_height / 2
                ) {
                    input_buffer.direction = SOUTH_EAST;
                } else if (
                    input_buffer.mouse_pos.x < window.screen_width / 2 &&
                    input_buffer.mouse_pos.y >= window.screen_height / 2
                ) {
                    input_buffer.direction = SOUTH_WEST;
                } else if (
                    input_buffer.mouse_pos.x < window.screen_width / 2 &&
                    input_buffer.mouse_pos.y < window.screen_height / 2
                ) {
                    input_buffer.direction = NORTH_WEST;
                }
            }
            input_buffer.mouse_down = false;
        } else {
            input_buffer.held_timer++;
        }
    }

    if (
        IsKeyPressed(KEY_LEFT) ||
        IsKeyPressed(KEY_Z) ||
        IsKeyPressed(KEY_A)
    ) {
        input_buffer.direction = SOUTH_WEST;
    } else if (
        IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed(KEY_O) ||
        IsKeyPressed(KEY_D)
    ) {
        input_buffer.direction = NORTH_EAST;
    } else if (
        IsKeyPressed(KEY_UP) ||
        IsKeyPressed(KEY_Q) ||
        IsKeyPressed(KEY_W)
    ) {
        input_buffer.direction = NORTH_WEST;
    } else if (
        IsKeyPressed(KEY_DOWN) ||
        IsKeyPressed(KEY_M) ||
        IsKeyPressed(KEY_S)
    ) {
        input_buffer.direction = SOUTH_EAST;
    }

    // For menus
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        input_buffer.select = true;
    }
}


void render_menu() {
    Vector2 pos = (Vector2) {0, 0};
    Rectangle logo_rect = (Rectangle) {0, 0, 144, 144};

    ClearBackground(C64_BLUE);

    // Maze
    render_maze();

    // To cover scrolling maze and for UI
    DrawRectangle(0, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);
    DrawRectangle(WINDOW_WIDTH - CELL_SIZE, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);

    DrawTextureRec(logo, logo_rect, (Vector2) {88, 32}, C64_WHITE);

    // Action highscore
    render_string(pos, action_highscore_name, NAME_LEN, true, C64_YELLOW);

    pos.y = 11 * CELL_SIZE;
    render_integer(pos, action_highscore, SCORE_LEN, true, C64_WHITE);

    // Puzzle highscore
    pos.x = WINDOW_WIDTH - CELL_SIZE;
    pos.y = 0;
    render_string(pos, puzzle_highscore_name, NAME_LEN, true, C64_YELLOW);

    pos.y = 11 * CELL_SIZE;
    render_integer(pos, puzzle_highscore, SCORE_LEN, true, C64_WHITE);

    // New daily in
    int length = 27;
    C64_CHARACTER new_daily[] = {
        C64_N, C64_E, C64_W, C64_BLANK,
        C64_D, C64_A, C64_I, C64_L, C64_Y, C64_BLANK,
        C64_M, C64_A, C64_Z, C64_E, C64_BLANK,
        C64_I, C64_N, C64_BLANK,
        C64_BLANK, C64_BLANK, C64_COLON,
        C64_BLANK, C64_BLANK, C64_COLON,
        C64_BLANK, C64_BLANK, C64_EXCLAMATION
    };
    pos.x = 56;
    pos.y = 16;
    DrawRectangleV(pos, (Vector2) {length * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, new_daily, length, false, C64_WHITE);
    pos.x = 208;
    render_integer(pos, date_buffer.hours, 2, false, WHITE);
    pos.x = 232;
    render_integer(pos, date_buffer.minutes, 2, false, WHITE);
    pos.x = 256;
    render_integer(pos, date_buffer.seconds, 2, false, WHITE);

    // Today's seed
    pos.x = 72;
    pos.y = 8;
    length = 15;
    C64_CHARACTER todays_seed[] = {
        C64_T, C64_O, C64_D, C64_A, C64_Y, C64_APOSTROPHE, C64_S, C64_BLANK,
        C64_S, C64_E, C64_E, C64_D, C64_BLANK,
        C64_COLON, C64_BLANK
    };
    DrawRectangleV(pos, (Vector2) {24 * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, todays_seed, length, false, C64_LIGHT_BLUE);

    pos.x = 248;
    render_integer(pos, seed, 8, false, C64_LIGHT_BLUE);
}


void render_game() {
    Vector2 pos = (Vector2) {0, 0};
    int new_index = 0;

    ClearBackground(C64_BLUE);

    // Maze
    render_maze();

    // Trail and collectibles
    for (int y = 0; y < TRAIL_HEIGHT; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            pos.x = x * HALF_CELL + CELL_SIZE - scroll_x;
            pos.y = y * HALF_CELL - HALF_CELL - scroll_y;

            new_index = player_index(x, y);

            char_rect.x = C64_COLLECTIBLE * CELL_SIZE;
            if (collectibles[new_index]) {
                DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
            }

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

    // Name
    pos.x = 0;
    pos.y = 0;
    render_string(pos, puzzle_highscore_name, NAME_LEN, true, C64_YELLOW);

    // Highscore
    pos.y = CELL_SIZE * 11;
    render_integer(pos, puzzle_highscore, SCORE_LEN, true, C64_WHITE);

    // Trophy
    pos.y = CELL_SIZE * 14;
    if (score > puzzle_highscore) {
        char_rect.x = C64_TROPHY * CELL_SIZE;
        DrawTextureRec(spritesheet, char_rect, pos, C64_YELLOW);
    }

    // Current score
    pos.y = CELL_SIZE * 24;
    render_integer(pos, score, SCORE_LEN, true, C64_WHITE);
}


void render_maze() {
    Vector2 pos = (Vector2) {0, 0};
    int new_index = 0;

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            pos.x = x * CELL_SIZE + CELL_SIZE - scroll_x;
            pos.y = y * CELL_SIZE - scroll_y;

            new_index = ((row_index + y) % GRID_HEIGHT) * GRID_WIDTH + (x + col_index) % GRID_WIDTH;;

            if (grid[new_index] == WALL_BROKEN) continue;

            char_rect.x = C64_BACKWARD * CELL_SIZE;
            if (grid[new_index] == WALL_FORWARD) {
                char_rect.x = C64_FORWARD * CELL_SIZE;
            }

            DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_BLUE);
        }
    }
}


void render_integer(Vector2 pos, int value, int length, bool vertical, Color colour) {
    int div = 1;
    for (int i = 0; i < length; i++) {
        char_rect.x = ((value / div) % 10 + C64_0) * CELL_SIZE;
        DrawTextureRec(spritesheet, char_rect, pos, colour);
        if (vertical) {
            pos.y -= CELL_SIZE;
        } else {
            pos.x -= CELL_SIZE;
        }
        div *= 10;
    }
}


void render_string(Vector2 pos, C64_CHARACTER* s, int length, bool vertical, Color colour) {
    for (int i = 0; i < length; i++) {
        char_rect.x = s[i] * CELL_SIZE;
        DrawTextureRec(spritesheet, char_rect, pos, colour);
        if (vertical) {
            pos.y += CELL_SIZE;
        } else {
            pos.x += CELL_SIZE;
        }
    }
}
