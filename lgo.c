#include <raylib.h>
#include <string.h>

#ifdef PLATFORM_WEB
    #include <emscripten.h>
#endif

#define SCREEN_W 1920
#define SCREEN_H 1080

#define CELL_SIZE 40
#define GRID_MAX_SIZE 10

typedef bool grid_t[GRID_MAX_SIZE][GRID_MAX_SIZE];

Font font;

RenderTexture2D render_tex;

grid_t main_grid;
grid_t main_next;
grid_t main_grid_first_step;

grid_t target_grid_first_step;
grid_t target_grid;
grid_t target_next;

bool simulation_playing = false;
bool grids_match = false;
bool grid_updated = false;
bool won = false;

int current_size = 3;

Vector2 mouse_pos;
Vector2 main_grid_pos;
Vector2 target_grid_pos;

int count_neighbours(grid_t grid, int x, int y)
{
    int count = 0;
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
                continue;

            int nx = x + dx;
            int ny = y + dy;

            if (nx < 0 || nx >= current_size || ny < 0 || ny >= current_size)
                continue;

            count += grid[nx][ny];
        }
    }

    return count;
}

void update_grid(grid_t grid, grid_t next)
{
    for (int y = 0; y < current_size; y++)
    {
        for (int x = 0; x < current_size; x++)
        {
            int n = count_neighbours(grid, x, y);
            next[x][y] = grid[x][y] ? (n == 2 || n == 3) : (n == 3);
        }
    }

    memcpy(grid, next, sizeof(grid_t));
}

void draw_grid(grid_t grid, Vector2 pos)
{
    for (int y = 0; y < current_size; y++)
    {
        for (int x = 0; x < current_size; x++)
        {
            Color color = grid[x][y] ? BLACK : WHITE;

            DrawRectangle(x * CELL_SIZE + pos.x, y * CELL_SIZE + pos.y,
                    CELL_SIZE, CELL_SIZE, color);
            DrawRectangleLines(x * CELL_SIZE + pos.x, y * CELL_SIZE + pos.y,
                    CELL_SIZE, CELL_SIZE, GRAY);
        }
    }
}

void place_cell(grid_t grid, Vector2 pos)
{
    int x = (mouse_pos.x - pos.x) / CELL_SIZE;
    int y = (mouse_pos.y - pos.y) / CELL_SIZE;

    if (x < 0 || x >= current_size || y < 0 || y >= current_size)
        return;

    grid[x][y] = !grid[x][y];
}

void draw_text(const char *text, float x, float y, float size, Color color)
{
    Vector2 text_size = MeasureTextEx(font, text, size, 1.0f);
    DrawTextEx(font, text, (Vector2){x - text_size.x / 2.0f, y - text_size.y / 2.0f},
            size, 1.0f, color);
}

bool check_grids_match(grid_t a, grid_t b)
{
    for (int y = 0; y < current_size; y++)
    {
        for (int x = 0; x < current_size; x++)
        {
            if (a[x][y] != b[x][y])
                return false;
        }
    }

    return true;
}

bool is_stable(grid_t grid)
{
    grid_t copy;
    grid_t next;

    memcpy(copy, grid, sizeof(grid_t));
    update_grid(copy, next);

    return check_grids_match(grid, copy);
}

void generate_target()
{
    grid_t target;
    grid_t next;

    for (int y = 0; y < current_size; y++)
    {
        for (int x = 0; x < current_size ; x++)
        {
            target[x][y] = GetRandomValue(0, 1);
        }
    }

    memcpy(target_grid_first_step, target, sizeof(grid_t));
    update_grid(target, next);

    int alive = 0;

    for (int y = 0; y < current_size; y++)
    {
        for (int x = 0; x < current_size; x++)
        {
            alive += target[x][y];
        }
    }

    if (alive < 3 || is_stable(target))
    {
        generate_target();
        return;
    }

    memcpy(target_grid, target, sizeof(grid_t));
}

bool button(Rectangle rect, char *text)
{
    bool hovered = CheckCollisionPointRec(mouse_pos, rect);
    Color color = hovered ? GRAY : WHITE;

    DrawRectangleRec(rect, color);
    draw_text(text, rect.x + rect.width / 2, rect.y + rect.height / 2 - 5.0f, 35.0f, BLACK);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void ui()
{
    Vector2 btn_size = {80.0f, 30.0f};
    float font_size = 35.0f;
    float padding = 10.0f;

    int grid_pixel_size = CELL_SIZE * current_size;

    float main_bottom = main_grid_pos.y + grid_pixel_size;
    float main_center_x = main_grid_pos.x + grid_pixel_size / 2.0f;
    float target_center_x = target_grid_pos.x + grid_pixel_size / 2.0f;

    float grid_top = main_grid_pos.y;

    Rectangle step_btn_rect = {main_center_x - btn_size.x / 2.0f,
        main_bottom + padding, btn_size.x, btn_size.y};

    Rectangle solution_btn_rect = {main_center_x - btn_size.x, 
        step_btn_rect.y + btn_size.y + padding, btn_size.x * 2.0f, btn_size.y};

    Rectangle restart_btn_rect = {main_center_x - btn_size.x,
        solution_btn_rect.y + btn_size.y + padding, btn_size.x * 2.0f, btn_size.y};

    Rectangle next_btn_rect = {main_center_x - btn_size.x, 
        step_btn_rect.y + btn_size.y + padding * 5.0f, btn_size.x * 2.0f, btn_size.y};

    if (button(step_btn_rect, "step"))
    {
        memcpy(main_grid_first_step, main_grid, sizeof(grid_t));
        simulation_playing = true;
    }

    if (button(solution_btn_rect, "solution"))
        memcpy(main_grid, target_grid_first_step, sizeof(grid_t));

    if (grids_match)
    {
        if (button(next_btn_rect, "next") && current_size < GRID_MAX_SIZE)
        {
            current_size++;
            grids_match = false;
            grid_updated = false;
            simulation_playing = false;
            generate_target();
            memset(main_grid, false, sizeof(grid_t));
        }
    }

    DrawRectangle(1300.0f, 0.0f, 2.0f, SCREEN_H, GRAY);

    draw_text("target", target_center_x,
              main_bottom + padding * 2.0f, font_size, BLACK);

    if (grids_match)
        draw_text("match!", main_center_x, grid_top - padding * 2.0f, font_size, GREEN);

    else if (grid_updated && simulation_playing)
    {
        draw_text("no match", main_center_x,
                  grid_top - padding * 2.0f, font_size, RED);

        if (button(restart_btn_rect, "restart"))
        {
            grid_updated = false;
            simulation_playing = false;
            memcpy(main_grid, main_grid_first_step, sizeof(grid_t));
        }
    }

    if (won)
    {
        draw_text("you won!", main_center_x, grid_top - padding * 5.0f, font_size, GREEN);

        if (button(restart_btn_rect, "restart"))
        {
            won = false;
            current_size = 3;
            grids_match = false;
            grid_updated = false;
            simulation_playing = false;
            generate_target();
            memset(main_grid, false, sizeof(grid_t));
        }
    }
}

void UpdateDrawFrame()
{
    float actual_screen_w = GetScreenWidth();
    float actual_screen_h = GetScreenHeight();

    main_grid_pos = (Vector2){SCREEN_W / 2 - (CELL_SIZE * current_size) / 2.0f,
            SCREEN_H / 2 - (CELL_SIZE * current_size) / 2.0f - 50.0f};
    target_grid_pos = (Vector2){SCREEN_W / 2 - (CELL_SIZE * current_size) / 2.0f + 650.0f,
        SCREEN_H / 2 - (CELL_SIZE * current_size) / 2.0f - 50.0f};

    Vector2 m = GetMousePosition();
    mouse_pos = (Vector2){m.x * (SCREEN_W / actual_screen_w),
        m.y * (SCREEN_H / actual_screen_h)};

    if (current_size == GRID_MAX_SIZE && grids_match)
        won = true;

    if (simulation_playing && !grid_updated)
    {
        update_grid(main_grid, main_next);
        grid_updated = true;
        grids_match = check_grids_match(main_grid, target_grid);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !simulation_playing)
        place_cell(main_grid, main_grid_pos);
    
    BeginTextureMode(render_tex);
        ClearBackground(WHITE);
        draw_grid(main_grid, main_grid_pos);
        draw_grid(target_grid, target_grid_pos);
        ui();
    EndTextureMode();

    BeginDrawing();
        Rectangle src = {0.0f, 0.0f, render_tex.texture.width, -render_tex.texture.height};
        Rectangle dest = {0.0f, 0.0f, actual_screen_w, actual_screen_h};
        DrawTexturePro(render_tex.texture, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    EndDrawing();
}

int main()
{
    InitWindow(SCREEN_W, SCREEN_H, "life goes on");
    ToggleFullscreen();

    font = LoadFont("assets/FSEX300.ttf");
    render_tex = LoadRenderTexture(SCREEN_W, SCREEN_H);

    generate_target();

    #ifdef PLATFORM_WEB
        emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
    #else
        SetTargetFPS(120);
        while (!WindowShouldClose())
            UpdateDrawFrame();
    #endif

    CloseWindow();
    return 0;
}
