#include "Engine.h"
#include <stdlib.h>
#include <memory.h>
#include <windows.h>
#include <math.h>
#include <chrono>

//
//  You are free to modify this file
//

//  is_key_pressed(int button_vk_code) - check if a key is pressed,
//                                       use keycodes (VK_SPACE, VK_RIGHT, VK_LEFT, VK_UP, VK_DOWN, 'A', 'B')
//
//  get_cursor_x(), get_cursor_y() - get mouse cursor position
//  is_mouse_button_pressed(int button) - check if mouse button is pressed (0 - left button, 1 - right button)
//  clear_buffer() - set all pixels in buffer to 'black'
//  is_window_active() - returns true if window is active
//  schedule_quit_game() - quit game after act()

enum color
{
    white = ~0 & ~(255 << 24), // let's think int is 32 bit type
    blue = 255,
    green = blue << 8,
    red = green << 8
};

float max_fps = 60;
std::chrono::steady_clock::time_point last_frame_time;
bool is_game_ended;

bool are_objects_colliding(int fx, int fy, int fs, int sx, int sy, int ss) // f - first, s (if first letter) - second, s (if last letter) - size
{
    return sx <= (fx + fs) && sx >= fx && sy <= (fy + fs) && sy >= fy || fx <= (sx + ss) && fx >= sx && fy <= (sy + ss) && fy >= sy;
}

double deg_to_rad(int x)
{
    return (double)x * 3.141592 / 180;
}

void draw_quad(int x, int y, int s, int color)
{
    for (int _y = y; _y < y + s; _y++)
        for (int _x = x; _x < x + s; _x++)
        {
            if (_y < 0 || _y >= SCREEN_HEIGHT || _x < 0 || _x >= SCREEN_WIDTH)
                continue;
            buffer[_y][_x] = color; // not buffer[_x][_y] because buffer[SCREEN_HEIGHT][SCREEN_WIDTH] but not buffer[SCREEN_WIDTH][SCREEN_HEIGHT]
        }
}

void draw_circle(int x, int y, int s, int color) // s = radius * 2, x and y - top left pixel of quad collider of circle
{
    double angle;
    int x1, y1;

    for (angle = 0; angle < 360; angle += 0.1)
    {
        x1 = (int)(s * cos(deg_to_rad(angle)));
        y1 = (int)(s * sin(deg_to_rad(angle)));

        for (int i = x1; i > -x1; --i)
        {
            if (y + y1 + s < 0 || y + y1 + s >= SCREEN_HEIGHT || x + i + s < 0 || x + i + s >= SCREEN_WIDTH) // player circles cannot reach window bounds but this check wont be incorrect
                continue;
            buffer[y + y1 + s][x + i + s] = color;
        }
    }
}

typedef struct circles
{
    int fx, fy; // first x, first y
    int sx, sy; // second ...
    bool is_to_left; // rotate to left? (by clock arrow)
    int degree_per_frame; // degree per frame to rotate
    int total_degree;
    int R; // radius from window center in pixels
    int size;

    void change_direction()
    {
        is_to_left = !is_to_left;
    }

    void compute_location()
    {
        total_degree -= is_to_left ? -degree_per_frame : degree_per_frame;

        if (total_degree > 360)
            total_degree %= 360;
        else if (total_degree < 0)
            total_degree = 360 - total_degree;

        fx = (int)(sin(deg_to_rad(total_degree)) * R) + SCREEN_WIDTH / 2;
        fy = (int)(cos(deg_to_rad(total_degree)) * R) + SCREEN_HEIGHT / 2;
        sx = (int)(sin(deg_to_rad(total_degree + 180)) * R) + SCREEN_WIDTH / 2;
        sy = (int)(cos(deg_to_rad(total_degree + 180)) * R) + SCREEN_HEIGHT / 2;
    }

    void draw()
    {
        compute_location();
        draw_circle(fx, fy, size / 2, green);
        draw_circle(sx, sy, size / 2, green);
    }

    circles(bool _istoleft, int _degreeperfr, int _R, int _size) // if need to init by constr
    {
        is_to_left = _istoleft;
        degree_per_frame = _degreeperfr;
        R = _R;
        size = _size;
    }
}circles;

circles p(false, 5, 100, 30);

typedef struct quad
{
    int x;
    int y;
    int size = 30;
    int dx; // moving by x axis
    int dy; // moving by y axis
    bool is_enemy;

    bool need_to_destroy()
    {
        return (y + size) < 0 || (x + size) < 0 || x > SCREEN_WIDTH;
    }

    void move()
    {
        x += dx;
        y -= dy;
    }

    void draw()
    {
        move();

        if (need_to_destroy())
            instantiate(rand() & 1);

        draw_quad(x, y, size, is_enemy ? red : white);
    }

    void instantiate(bool _isenemy)
    {
        static int xrange = 80;

        if (rand() & 1) // moving from bottom to top of window
        {
            x = SCREEN_WIDTH / 2 + ((rand() & 1) ? -xrange : xrange);
            dx = 0;
            dy = 4;
        }
        else // moving by angle
        {
            x = (rand() & 1) ? -size : SCREEN_WIDTH;
            dx = 4;

            if (x == SCREEN_WIDTH)
                dx = -dx;

            dy = 3;
        }

        y = SCREEN_HEIGHT;
        is_enemy = _isenemy;
    }
}quad;

quad q;

// initialize game data in this function
void initialize()
{
    srand((unsigned)time(0));

    q.instantiate(rand() & 1);

    is_game_ended = false;
}

// this function is called to update game data,
// dt - time elapsed since the previous update (in seconds)
void act(float dt)
{
    if (is_key_pressed(VK_ESCAPE))
        schedule_quit_game();

    if (is_key_pressed('R')) // for test
    {
        q.instantiate(q.is_enemy);
        is_game_ended = false;
    }

    if (is_game_ended)
        return;

    static float time_after_button_pressed = 0; // static let me instantiate mind that C# and C++ both have great difference between its

    if (is_key_pressed(VK_SPACE))
    {
        if (time_after_button_pressed < 0.1f) // prevent often pressings
            return;

        p.change_direction();
        time_after_button_pressed = 0;
        return;
    }

    time_after_button_pressed += dt;
}

// fill buffer in this function
// uint32_t buffer[SCREEN_HEIGHT][SCREEN_WIDTH] - is an array of 32-bit colors (8 bits per R, G, B)
void draw()
{
    if (is_game_ended)
        return;

    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_frame_time).count() < (1000.0f / max_fps))
        return;

    last_frame_time = std::chrono::steady_clock::now();

    memset(buffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint32_t)); // set all cells of buffer to 0

    p.draw();
    q.draw();

    if (are_objects_colliding(p.fx, p.fy, p.size, q.x, q.y, q.size))
    {
        if (!q.is_enemy)
            q.instantiate(rand() & 1);
        else
            is_game_ended = true;
    }
    else if (are_objects_colliding(p.sx, p.sy, p.size, q.x, q.y, q.size))
    {
        if (!q.is_enemy)
            q.instantiate(rand() & 1);
        else
            is_game_ended = true;
    }
}

// free game data in this function
void finalize()
{
    // dynamic memory is not used
}