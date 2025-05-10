#include <assert.h>
#include "raylib.h"
#include "core.h"


#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_CAPTION "raylib template"
#define FPS 0

DEFINE_TYPED_QUEUE(int8_t, Queue)
DEFINE_TYPED_STACK(int8_t, Stack)
DEFINE_TYPED_GRID(int8_t, Grid)


const uint8_t NODES = 255;

int8_t value;
Stack stack;
Queue queue;
Grid grid;


int main(void) {
    Stack_init(&stack, NODES);
    Queue_init(&queue, NODES + 1);
    Grid_init(&grid, NODES, NODES);

////////////////////////////////////////////////////////////////////////////////
// BASIC UNIT TESTS ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
    assert(MIN('a', 'b') == 'a');
    assert(MIN(8.0f, -8) == -8);

    assert(MAX('a', 'b') == 'b');
    assert(MAX(8.0f, -8) == 8.0f);

    for (uint8_t i = 0; i < NODES; i++) {
        assert(Stack_append(&stack, GetRandomValue(INT8_MIN, INT8_MAX)));
        assert(Queue_append(&queue, GetRandomValue(INT8_MIN, INT8_MAX)));
    }
    assert(Stack_length(&stack) == NODES);
    assert(Queue_length(&queue) == NODES);
    for (uint8_t i = 0; i < NODES; i++) {
        assert(Stack_pop(&stack, &value));
        assert(Queue_pop(&queue, &value));
    }
    assert(Stack_length(&stack) == 0);
    assert(Queue_length(&queue) == 0);

    for (uint8_t y = 0; y < NODES; y++) {
        for (uint8_t x = 0; x < NODES; x++) {
            assert(Grid_get(&grid, x, y) == 0);
        }
    }
    for (uint8_t y = 0; y < NODES; y++) {
        for (uint8_t x = 0; x < NODES; x++) {
            Grid_set(&grid, x, y, 1);
            assert(Grid_get(&grid, x, y) == 1);
        }
    }
    Grid_fill(&grid, 2);
    for (uint8_t y = 0; y < NODES; y++) {
        for (uint8_t x = 0; x < NODES; x++) {
            assert(Grid_get(&grid, x, y) == 2);
        }
    }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_CAPTION);
    SetTargetFPS(FPS);
    while (!WindowShouldClose()) {
        // INPUT
        ;

        // UPDATE
        ;

        // RENDER
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(8, 8);
        EndDrawing();
    }

    CloseWindow();

    // DEINITIALISE
    Stack_free(&stack);
    Queue_free(&queue);
    Grid_free(&grid);
}
