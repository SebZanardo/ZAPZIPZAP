#include <stdio.h>
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


void load_maze();
void load_game();
void reset();
void handle_resize();
void set_offset(MOVE_DIRECTION dir);
MOVE_DIRECTION valid_move();
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
void render_highscore();
void render_maze();
void render_integer(Vector2 pos, int value, int length, bool vertical, Color colour);
void render_string(Vector2 pos, char* s, int length, bool vertical, Color colour, bool convert);
C64_CHARACTER char_index(char c);


// GLOBALS /////////////////////////////////////////////////////////////////////
WindowParameters window;
InputBuffer input_buffer;
DateBuffer date_buffer;
bool handheld;

uint32_t seed;
uint32_t ticks = 0;

// ASSETS
Music music;

Sound sfx_blocked;
Sound sfx_collect;
Sound sfx_scroll_east;
Sound sfx_scroll_north;
Sound sfx_scroll_south;
Sound sfx_scroll_west;
Sound sfx_select;
Sound sfx_zap;
Sound sfx_zip;
Sound sfx_left;
Sound sfx_right;
Sound sfx_up;
Sound sfx_down;
Sound sfx_highscore;

Texture2D spritesheet;
Texture2D buttons;
Texture2D logo;
Image icon;

Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};

SCENE scene = MENU;

WALL_STATE grid[GRID_CELLS];

MOVE_DIRECTION trail[TRAIL_CELLS];
bool collectibles[TRAIL_CELLS];

int bounds_x = (GRID_WIDTH - 1) * 2;
int bounds_y = (GRID_HEIGHT - 1) * 2;

bool is_action_mode = true;
int ui_index = 0;
int ui_timer = 0;

// TODO: Load from web or from file
uint32_t action_highscore = 0;
char action_highscore_name[] = {C64_Z, C64_I, C64_P};

uint32_t puzzle_highscore = 0;
char puzzle_highscore_name[] = {C64_Z, C64_A, C64_P};

char enter_name[] = {C64_A, C64_A, C64_A};

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
int ox = 0;
int oy = 0;
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
    music = LoadMusicStream("src/resources/theme.mp3");
    SetMusicVolume(music, MUSIC_VOLUME);
    PlayMusicStream(music);

    #ifdef WEB_BUILD
        handheld = IsMobile();
    #endif

    sfx_blocked = LoadSound("src/resources/blocked.mp3");
    sfx_collect = LoadSound("src/resources/collect.mp3");
    sfx_scroll_east = LoadSound("src/resources/scroll_east.mp3");
    sfx_scroll_north = LoadSound("src/resources/scroll_north.mp3");
    sfx_scroll_south = LoadSound("src/resources/scroll_south.mp3");
    sfx_scroll_west = LoadSound("src/resources/scroll_west.mp3");
    sfx_select = LoadSound("src/resources/select.mp3");
    sfx_zap = LoadSound("src/resources/zap.mp3");
    SetSoundVolume(sfx_zap, 0.4f);
    sfx_zip = LoadSound("src/resources/zip.mp3");
    SetSoundVolume(sfx_zip, 0.6f);
    sfx_up = LoadSound("src/resources/up.mp3");
    sfx_down = LoadSound("src/resources/down.mp3");
    sfx_left = LoadSound("src/resources/left.mp3");
    sfx_right = LoadSound("src/resources/right.mp3");
    sfx_highscore = LoadSound("src/resources/highscore.mp3");

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    spritesheet = LoadTexture("src/resources/fontsheet.png");
    buttons = LoadTexture("src/resources/buttons.png");
    logo = LoadTexture("src/resources/logo.png");
    icon = LoadImage("src/resources/icon.png");

    SetWindowIcon(icon);
    handle_resize();

    // Get random seed from time
    reset();
    load_maze();

    ////////////////////////////////////////////////////////////////////////////
    // GAME LOOP ///////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    while (!WindowShouldClose()) {
        // PRE-UPDATE //////////////////////////////////////////////////////////
        UpdateMusicStream(music);
        if (IsWindowResized()) handle_resize();

        // INPUT ///////////////////////////////////////////////////////////////
        update_input_buffer();

        // Set offset direction
        set_offset(input_buffer.direction);

        // UPDATE //////////////////////////////////////////////////////////////
        switch (scene) {
            case MENU:
                update_time_and_seed();
                if (input_buffer.select && ui_index != 0) {
                    load_game();
                } else if (
                    (input_buffer.mouse_clicked && input_buffer.mouse_pos.x < window.screen_width / 2) ||
                    input_buffer.direction == NORTH_WEST || input_buffer.direction == SOUTH_WEST
                ) {
                    if (ui_index == -1) {
                        load_game();
                    }
                    PlaySound(sfx_left);
                    ui_index = -1;
                } else if (
                    (input_buffer.mouse_clicked && input_buffer.mouse_pos.x >= window.screen_width / 2) ||
                    input_buffer.direction == NORTH_EAST || input_buffer.direction == SOUTH_EAST
                ) {
                    if (ui_index == 1) {
                        load_game();
                    }
                    PlaySound(sfx_right);
                    ui_index = 1;
                }
                break;
            case GAME:
                if (ui_timer > 0) {
                    if (!gameover) {
                        // Countdown
                        if (ui_timer == 39) {
                            PlaySound(sfx_scroll_south);
                        } else if (ui_timer == 24) {
                            PlaySound(sfx_scroll_west);
                        } else if (ui_timer == 9) {
                            PlaySound(sfx_scroll_east);
                        } else if (ui_timer == 1) {
                            PlaySound(sfx_scroll_north);
                        }
                    } else {
                        // GAMEOVER delay
                        if (ui_timer == 78) {
                            PlaySound(sfx_select);
                        }
                    }
                    ui_timer--;
                } else if (!gameover) {
                    if (px < 0 || px > TRAIL_WIDTH || py < 0 || py > TRAIL_HEIGHT) {
                        gameover = true;
                        ui_timer = 80;
                    }

                    int new_index = 0;
                    int steps = 0;
                    if (input_buffer.direction != NO_DIRECTION) {
                        // ZIP
                        while (1) {
                            if (should_stop_zip(px, py, input_buffer.direction)) break;
                            new_index = player_index(px + ox, py + oy);
                            if (trail[new_index] != NO_DIRECTION) {
                                if (steps == 0) {
                                    PlaySound(sfx_blocked);
                                }
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
                            // Broke wall
                            float pitch = 1 - (float)GetRandomValue(1, 8) / 32.0f;
                            SetSoundPitch(sfx_zap, pitch);
                            PlaySound(sfx_zap);
                            grid[new_index] = WALL_BROKEN;
                            zap_cooldown_counter = ZAP_VISUAL_COOLDOWN;
                            zaps--;
                        } else {
                            // No wall to break, edge of map or trail (BONK)
                            PlaySound(sfx_blocked);
                        }
                    } else {
                        // ZIP SOUND pick random pitch
                        if (steps > 0) {
                            float pitch = 1 - (float)GetRandomValue(1, 8) / 32.0f;
                            SetSoundPitch(sfx_zip, pitch);
                            PlaySound(sfx_zip);
                        }
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

                        // Check if no moves left
                        if (valid_move() == NO_DIRECTION) {
                            gameover = true;
                            ui_timer = 80;
                        }
                    }
                } else {
                    if (is_action_mode) {
                        reset();
                        if (score > action_highscore) {
                            PlaySound(sfx_highscore);
                            scene = SCORE;
                            // Load last highscore name into enter
                            for (int i = 0; i < NAME_LEN; i++) {
                                enter_name[i] = action_highscore_name[i];
                            }
                        } else {
                            scene = MENU;
                        }
                    } else {
                        reset();
                        if (score > puzzle_highscore) {
                            PlaySound(sfx_highscore);
                            scene = SCORE;
                            // Load last highscore name into enter
                            for (int i = 0; i < NAME_LEN; i++) {
                                enter_name[i] = puzzle_highscore_name[i];
                            }
                        } else {
                            scene = MENU;
                        }
                    }
                }
                break;

            case SCORE:
                if (gameover) {
                    if (input_buffer.direction != NO_DIRECTION || input_buffer.select) {
                        PlaySound(sfx_right);
                        gameover = false;
                    }
                } else {
                    if (input_buffer.select) {
                        PlaySound(sfx_right);
                        ui_index++;
                    }

                    if (input_buffer.direction == NORTH_EAST) {
                        PlaySound(sfx_right);
                        ui_index++;
                    } else if (input_buffer.direction == SOUTH_WEST) {
                        PlaySound(sfx_left);
                        ui_index--;
                        if (ui_index < 0) {
                            gameover = true;
                            ui_index = 0;
                        }
                    } else if (input_buffer.direction == NORTH_WEST) {
                        PlaySound(sfx_up);
                        // Increment letter
                        enter_name[ui_index]++;
                        if (enter_name[ui_index] > C64_Z) {
                            enter_name[ui_index] = C64_A;
                        }
                    } else if (input_buffer.direction == SOUTH_EAST) {
                        PlaySound(sfx_down);
                        // Decrement letter
                        enter_name[ui_index]--;
                        if (enter_name[ui_index] < C64_A) {
                            enter_name[ui_index] = C64_Z;
                        }
                    }

                    if (ui_index > 2) {
                        PlaySound(sfx_select);
                        // Set highscore!
                        if (is_action_mode) {
                            for (int i = 0; i < NAME_LEN; i++) {
                                action_highscore_name[i] = enter_name[i];
                            }
                            action_highscore = score;
                        } else {
                            for (int i = 0; i < NAME_LEN; i++) {
                                puzzle_highscore_name[i] = enter_name[i];
                            }
                            puzzle_highscore = score;
                        }
                        scene = MENU;
                    }
                }
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
                render_highscore();
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

    UnloadSound(sfx_blocked);
    UnloadSound(sfx_collect);
    UnloadSound(sfx_scroll_east);
    UnloadSound(sfx_scroll_north);
    UnloadSound(sfx_scroll_south);
    UnloadSound(sfx_scroll_west);
    UnloadSound(sfx_select);
    UnloadSound(sfx_zap);
    UnloadSound(sfx_zip);
    UnloadSound(sfx_left);
    UnloadSound(sfx_right);
    UnloadSound(sfx_up);
    UnloadSound(sfx_down);
    UnloadSound(sfx_highscore);

    UnloadImage(icon);
    UnloadTexture(spritesheet);
    UnloadTexture(buttons);
    UnloadTexture(logo);

    CloseAudioDevice();
    CloseWindow();
}


void load_maze() {
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1) + 1;
    }
}


void load_game() {
    PlaySound(sfx_select);

    if (ui_index == 1) {
        is_action_mode = false;
        bounds_x = (GRID_WIDTH - 2) * 2;
        bounds_y = (GRID_HEIGHT - 1) * 2;
    } else {
        is_action_mode = true;
        bounds_x = (GRID_WIDTH - 1) * 2;
        bounds_y = (GRID_HEIGHT - 1) * 2;
    }

    ui_timer = 50;

    reset();

    score = 0;
    gameover = false;

    // Random starting maze
    load_maze();

    // Setup trail and collectibles
    for (int y = 0; y < TRAIL_HEIGHT; y++) {
        for (int x = 0; x < TRAIL_WIDTH; x++) {
            int new_index = y * TRAIL_WIDTH + x;
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

    scene = GAME;
}


void reset() {
    update_time_and_seed();

    ui_index = 0;

    px = 37;
    py = 25;
    ox = 0;
    oy = 0;
    zaps = 8;
    zap_cooldown_counter = 0;

    row_index = 0; // Points to the first row to be rendered
    col_index = 0; // Points to the first col to be rendered
    scroll_dir = NORTH;
    scroll_counter = 40;
    scroll_x = 0;
    scroll_y = 0;
    scroll_speed = 1;
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


void set_offset(MOVE_DIRECTION dir) {
    switch (dir) {
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
}


MOVE_DIRECTION valid_move() {
    // Prove that there is still a valid move!
    for (int i = 1; i < 5; i++) {
        set_offset(i);
        int nx = px + ox;
        int ny = py + oy;

        int new_index = player_index(nx, ny);
        // Not trying to move into trail
        if (trail[new_index] == NO_DIRECTION) {
            // Not trying to move off map
            if (nx > 0 && nx < bounds_x && ny > 0 && ny < bounds_y) {
                // Not trying to move into wall and no zaps left
                bool wall = should_stop_zip(px, py, i);
                if (!wall || (zaps > 0 && wall)) {
                    return i;
                    break;
                }
            }
        }
    }
    return NO_DIRECTION;
}


void check_for_collectible(int new_index) {
    if (collectibles[new_index]) {
        // Collected zap
        PlaySound(sfx_collect);
        collectibles[new_index] = false;
        zaps++;
        zaps = MIN(zaps, GRID_HEIGHT - 1);
        if (is_action_mode) score += ZAP_BONUS_SCORE;
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
            return x <= 0 || y >= bounds_y || grid[player_maze_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_EAST:
            return x >= bounds_x || y <= 0 || grid[player_maze_index(x, y, dir)] == WALL_BACKWARD;
        case NORTH_WEST:
            return x <= 0 || y <= 0 || grid[player_maze_index(x, y, dir)] == WALL_FORWARD;
        case SOUTH_EAST:
            return x >= bounds_x || y >= bounds_y || grid[player_maze_index(x, y, dir)] == WALL_FORWARD;
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

    // PLAY NOTE depending on direction
    if (scroll_dir == NORTH) {
        PlaySound(sfx_scroll_south);
    } else if (scroll_dir == EAST) {
        PlaySound(sfx_scroll_east);
    } else if (scroll_dir == SOUTH) {
        PlaySound(sfx_scroll_north);
    } else if (scroll_dir == WEST) {
        PlaySound(sfx_scroll_west);
    }

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
    }
    SetRandomSeed(seed);
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
    input_buffer.mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !input_buffer.mouse_down) {
        input_buffer.drag_start = input_buffer.mouse_pos;
        input_buffer.mouse_down = true;
        input_buffer.held_timer = 0;
    }

    if (input_buffer.mouse_down && scene != MENU) {
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
    Rectangle button_rect = (Rectangle) {0, 0, 96, 96};

    ClearBackground(C64_BLUE);

    // Maze
    render_maze();

    // To cover scrolling maze and for UI
    DrawRectangle(0, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);
    DrawRectangle(WINDOW_WIDTH - CELL_SIZE, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);

    // Alternating logo and controls
    logo_rect.x = 0;
    if ((ticks / 120) % 2 == 0) {
        logo_rect.x = logo_rect.width;
    }
    DrawTextureRec(logo, logo_rect, (Vector2) {88, 32}, C64_WHITE);

    // Action highscore
    render_string(pos, action_highscore_name, NAME_LEN, true, C64_YELLOW, false);

    pos.y = 11 * CELL_SIZE;
    render_integer(pos, action_highscore, SCORE_LEN, true, C64_WHITE);

    pos.y = 14 * CELL_SIZE;
    render_string(pos, "ACTION BEST :", 13, true, C64_YELLOW, true);

    // Puzzle highscore
    pos.x = WINDOW_WIDTH - CELL_SIZE;
    pos.y = 0;
    render_string(pos, puzzle_highscore_name, NAME_LEN, true, C64_YELLOW, false);

    pos.y = 11 * CELL_SIZE;
    render_integer(pos, puzzle_highscore, SCORE_LEN, true, C64_WHITE);

    pos.y = 14 * CELL_SIZE;
    render_string(pos, "PUZZLE BEST :", 13, true, C64_YELLOW, true);

    // New daily in
    int length = 27;
    pos.x = 56;
    pos.y = 16;
    DrawRectangleV(pos, (Vector2) {length * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, "NEW DAILY MAZE IN   :  :  !", length, false, C64_WHITE, true);
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
    DrawRectangleV(pos, (Vector2) {24 * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, "TODAY'S SEED :", length, false, C64_LIGHT_BLUE, true);

    pos.x = 248;
    render_integer(pos, seed, 8, false, C64_LIGHT_BLUE);

    // Credits
    pos.x = 120;
    pos.y = 176;
    length = 10;
    DrawRectangleV(pos, (Vector2) {length * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, "SEBZANARDO", length, false, C64_LIGHT_BLUE, true);
    pos.x = 116;
    pos.y = 184;
    length = 11;
    DrawRectangleV(pos, (Vector2) {length * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, "HUNTERSTARR", length, false, C64_LIGHT_BLUE, true);

    // TODO: Daily login/attempt streak

    // BUTTONS!
    pos.x = 7;
    pos.y = 55;
    button_rect.x = button_rect.width * 2;
    DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
    button_rect.x = button_rect.width * 3;
    DrawTextureRec(buttons, button_rect, pos, C64_WHITE);

    pos.x = 215;
    pos.y = 55;
    button_rect.x = button_rect.width * 2;
    DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
    button_rect.x = button_rect.width * 4;
    DrawTextureRec(buttons, button_rect, pos, C64_WHITE);

    // Button Cursor
    button_rect.x = button_rect.width;
    if ((ticks / FPS) % 2 == 0) {
        button_rect.x = 0;
    }

    if (ui_index == -1) {
        pos.x = 7;
        pos.y = 55;
        DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
    } else if (ui_index == 1) {
        pos.x = 215;
        pos.y = 55;
        DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
    }
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
                DrawTextureRec(spritesheet, char_rect, pos, zap_cooldown_counter > 0 ? C64_YELLOW : C64_LIGHT_GREY);
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
    for (int i = 0; i < GRID_HEIGHT - 1; i++) {
        Color colour = i < zaps ? C64_YELLOW : C64_LIGHT_BLUE;
        DrawTextureRec(spritesheet, char_rect, pos, colour);
        pos.y += CELL_SIZE;
    }

    pos.x = 0;
    pos.y = 0;
    // Name
    render_string(pos, puzzle_highscore_name, NAME_LEN, true, C64_YELLOW, false);

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

    if (ui_timer > 0) {
        pos.x = 104;
        pos.y = 72;
        DrawRectangleV(pos, (Vector2) {14 * CELL_SIZE, CELL_SIZE}, C64_BLUE);
        if (!gameover) {
            if (ui_timer < 40) {
                render_string(pos, "LOAD", 4, false, C64_WHITE, true);
                pos.x += 5 * CELL_SIZE;
            }
            if (ui_timer < 25) {
                render_string(pos, "READY", 6, false, C64_WHITE, true);
                pos.x += 6 * CELL_SIZE;
            }
            if (ui_timer < 10) {
                render_string(pos, "RUN", 3, false, C64_WHITE, true);
            }
        } else {
            pos.x += 24;
            render_string(pos, "GAMEOVER", 8, false, C64_WHITE, true);
        }
    }
}


void render_highscore() {
    Vector2 pos = (Vector2) {0, 0};
    Rectangle logo_rect = (Rectangle) {0, 0, 144, 144};
    Rectangle button_rect = (Rectangle) {0, 0, 96, 96};

    ClearBackground(C64_BLUE);

    // Maze
    render_maze();

    // Today's seed
    pos.x = 72;
    pos.y = 8;
    DrawRectangleV(pos, (Vector2) {24 * CELL_SIZE, CELL_SIZE}, C64_BLUE);
    render_string(pos, "TODAY'S SEED :", 15, false, C64_LIGHT_BLUE, true);

    pos.x = 248;
    render_integer(pos, seed, 8, false, C64_LIGHT_BLUE);

    // To cover scrolling maze and for UI
    DrawRectangle(0, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);
    DrawRectangle(WINDOW_WIDTH - CELL_SIZE, 0, CELL_SIZE, WINDOW_HEIGHT, C64_BLUE);

    // Panel
    logo_rect.x = logo_rect.width * 2;
    DrawTextureRec(logo, logo_rect, (Vector2) {88, 32}, C64_WHITE);

    if (gameover) {
        // Icon for mode
        pos.x = 112;
        pos.y = 55;
        if (is_action_mode) {
            button_rect.x = button_rect.width * 2;
            DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
            button_rect.x = button_rect.width * 3;
            DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
        } else {
            button_rect.x = button_rect.width * 2;
            DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
            button_rect.x = button_rect.width * 4;
            DrawTextureRec(buttons, button_rect, pos, C64_WHITE);
        }

        // New best
        pos.x = 128;
        pos.y = 61;
        render_string(pos, "NEW BEST", 8, false, C64_YELLOW, true);

        // Score
        pos.x = 184;
        pos.y = 140;
        render_integer(pos, score, SCORE_LEN, false, C64_YELLOW);
    } else {
        pos.x = 140;
        for (int i = 0; i < NAME_LEN; i++) {
            pos.y = 90;
            Color colour = i == ui_index ? C64_YELLOW : C64_WHITE;
            char_rect.x = C64_UP_ARROW * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, colour);
            pos.y += CELL_SIZE;

            char_rect.x = enter_name[i] * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, colour);

            pos.y += CELL_SIZE;
            char_rect.x = C64_DOWN_ARROW * CELL_SIZE;
            DrawTextureRec(spritesheet, char_rect, pos, colour);

            pos.x += CELL_SIZE * 2;
        }
    }
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


void render_string(Vector2 pos, char* s, int length, bool vertical, Color colour, bool convert) {
    for (int i = 0; i < length; i++) {
        if (convert) {
            char_rect.x = char_index(s[i]) * CELL_SIZE;
        } else {
            char_rect.x = s[i] * CELL_SIZE;
        }
        DrawTextureRec(spritesheet, char_rect, pos, colour);
        if (vertical) {
            pos.y += CELL_SIZE;
        } else {
            pos.x += CELL_SIZE;
        }
    }
}


C64_CHARACTER char_index(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 65 + 20;
    } else if (c >= '0' && c <= '9') {
        return c - 48 + 10;
    } else if (c == '!') {
        return C64_EXCLAMATION;
    } else if (c == ':') {
        return C64_COLON;
    } else if (c == '\'') {
        return C64_APOSTROPHE;
    }
    return 52;
}
