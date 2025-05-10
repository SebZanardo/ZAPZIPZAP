/*#include <stdio.h>*/
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "constants.h"


typedef struct {
    int screen_width; int screen_height;
    int scale;
    int offset_x;
    int offset_y;
} WindowParameters;


void handle_resize(WindowParameters *window);


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
    int GRID_WIDTH = WINDOW_WIDTH / CELL_SIZE + 1;
    int GRID_HEIGHT = WINDOW_HEIGHT / CELL_SIZE + 1;
    int GRID_CELLS = GRID_WIDTH * GRID_HEIGHT;
    int grid[GRID_CELLS];

    // Random starting maze
    for (int i = 0; i < GRID_CELLS; i++) {
        grid[i] = GetRandomValue(0, 1);
    }

    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

    Texture2D spritesheet = LoadTexture("src/resources/spritesheet.png");
    Vector2 pos = (Vector2) {0, 0};
    Rectangle char_rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};
    /*Rectangle rect = (Rectangle) {0, 0, CELL_SIZE, CELL_SIZE};*/

    int player_x = 20;
    int player_y = 12;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            handle_resize(&window);
        }

        // INPUT
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_Z)) {
            if (!grid[(int)(player_y / 2) * GRID_WIDTH + (int)(player_x / 2)])
            {
                player_x--;
                player_y++;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_R)) {
            if (!grid[(int)((player_y - 1) / 2) * GRID_WIDTH + (int)((player_x + 1) / 2)])
            {
                player_x++;
                player_y--;
            }
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            if (grid[(int)((player_y - 1) / 2) * GRID_WIDTH + (int)((player_x) / 2)])
            {
                player_x--;
                player_y--;
            }
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_C)) {
            if (grid[(int)(player_y / 2) * GRID_WIDTH + (int)((player_x + 1) / 2)])
            {
                player_x++;
                player_y++;
            }
        }

        // UPDATE
        ;

        // RENDER
        BeginTextureMode(target);
        ClearBackground(C64_BLUE);

        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                pos.x = x * CELL_SIZE;
                pos.y = y * CELL_SIZE;

                char_rect.x = C64_BACKWARD * CELL_SIZE;
                if (grid[y * GRID_WIDTH + x]) {
                    char_rect.x = C64_FORWARD * CELL_SIZE;
                }

                DrawTextureRec(spritesheet, char_rect, pos, C64_LIGHT_BLUE);
            }
        }

        int render_x = player_x * 0.5f * CELL_SIZE + 4;
        int render_y = player_y * 0.5f * CELL_SIZE;
        /*printf("%d, %d\n", player_x, player_y);*/
        DrawCircle(
            render_x,
            render_y,
            2,
            C64_WHITE
        );

        /*DrawTexture(spritesheet, 0, 0, C64_LIGHT_BLUE);*/

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
