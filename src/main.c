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

    // +1 So scrolling looks seamless
    WALL_STATE grid[GRID_CELLS];

    // Random starting maze
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1) + 1;
    }

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    Texture2D spritesheet = LoadTexture("src/resources/spritesheet.png");
    Vector2 pos = (Vector2) {0, 0};
    Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};

    int player_x = 20;
    int player_y = 12;
    int BOUNDS_X = GRID_WIDTH * 2;
    int BOUNDS_Y = GRID_HEIGHT * 2;

    MOVE_DIRECTION last_move = NO_DIRECTION;
    int zaps = 10;
    int zap_cooldown = 10;
    int zap_cooldown_counter = 0;
    int steps = 0;
    bool moved = false;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) handle_resize(&window);

        // INPUT
        moved = true;
        steps = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_Z)) {
            last_move = SOUTH_WEST;
            while (grid[new_player_position(player_x, player_y, last_move)] != WALL_FORWARD) {
                if (player_x < 0 || player_y > BOUNDS_Y) break;
                player_x--;
                player_y++;
                steps++;
            }
        } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_R)) {
            last_move = NORTH_EAST;
            while (grid[new_player_position(player_x, player_y, last_move)] != WALL_FORWARD) {
                if (player_x > BOUNDS_X || player_y < 0) break;
                player_x++;
                player_y--;
                steps++;
            }
        } else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            last_move = NORTH_WEST;
            while (grid[new_player_position(player_x, player_y, last_move)] != WALL_BACKWARD) {
                if (player_x < 0 || player_y < 0) break;
                player_x--;
                player_y--;
                steps++;
            }
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_C)) {
            last_move = SOUTH_EAST;
            while (grid[new_player_position(player_x, player_y, last_move)] != WALL_BACKWARD) {
                if (player_x > BOUNDS_X || player_y > BOUNDS_Y) break;
                player_x++;
                player_y++;
                steps++;
            }
        } else {
            moved = false;
        }


        // UPDATE
        if (moved && steps == 0 && zaps > 0) {
            zap_cooldown_counter = zap_cooldown;
            grid[new_player_position(player_x, player_y, last_move)] = WALL_BROKEN;
            zaps--;
        }

        if (zap_cooldown_counter > 0) {
            zap_cooldown_counter--;
        }

        // RENDER
        BeginTextureMode(target);
        ClearBackground(C64_BLUE);

        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                pos.x = x * CELL_SIZE + CELL_SIZE;
                pos.y = y * CELL_SIZE;

                if (grid[y * GRID_WIDTH + x] == WALL_BROKEN) {
                    continue;
                }
                char_rect.x = C64_BACKWARD * CELL_SIZE;
                if (grid[y * GRID_WIDTH + x] == WALL_FORWARD) {
                    char_rect.x = C64_FORWARD * CELL_SIZE;
                }

                DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_BLUE);
            }
        }

        int render_x = player_x * 0.5f * CELL_SIZE + CELL_SIZE + 4;
        int render_y = player_y * 0.5f * CELL_SIZE;
        DrawCircle(
            render_x,
            render_y,
            2,
            zap_cooldown_counter > 0 ? C64_YELLOW : C64_WHITE
        );

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

        EndTextureMode();

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


int new_player_position(int x, int y, MOVE_DIRECTION dir) {
    assert (dir != NO_DIRECTION);
    switch (dir) {
        case NO_DIRECTION:
            return -1;
        case NORTH_EAST:
            return (int)((y - 1) / 2) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_EAST:
            return (int)(y / 2) * GRID_WIDTH + (int)((x + 1) / 2);
        case SOUTH_WEST:
            return (int)(y / 2) * GRID_WIDTH + (int)(x / 2);
        case NORTH_WEST:
            return (int)((y - 1) / 2) * GRID_WIDTH + (int)(x / 2);
    }
}
