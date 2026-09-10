
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <d2d1.h>
#include <typeinfo>

struct RenderStats {
    int half_player_width; // in pixels
    int half_player_height; // in pixels
    int render_distance; // how many chunks to render in each direction
    std::vector<int> player_render;
    int center_x; // pixel position of center of screen
    int center_y;
    int boss_life_bar_ymin;
    int boss_life_bar_ymax;
    int player_life_bar_ymin;
    int player_life_bar_ymax;
    int player_life_bar_x;
    int menu_stats_margin = 40;
    int stats_xmin;
    int left_stats_ymin;
    int stats_xmax;
    int left_stats_ymax;
    int right_stats_ymin;
    int right_stats_ymax;
    int stats_picture_width;
    int top_stats_xmin;
    int top_stats_ymin;
    int top_stats_xmax;
    int top_stats_ymax;
    int stat_display_width;
    int bottom_text_shift = 20;
    int text_height = 20;
    int stats_x_margin = 10;
    int stats_initial_x_margin = 30;
    int stats_y_margin = 10;
    int stat_button_width = 20;
    bool render_portal = true;
    std::vector<int> shift;
    RenderStats(
        const std::vector<int>& arg_player_render,
        const std::vector<int>& arg_shift,
        int chunk_size
    ) :
        half_player_width(chunk_size / 2),
        half_player_height(chunk_size / 2),
        player_render(arg_player_render),
        center_x((player_render[0] + player_render[2]) / 2),
        center_y((player_render[1] + player_render[3]) / 2),
        shift(arg_shift),
        boss_life_bar_ymin(center_y - 2 * arg_shift[1] / 3),
        boss_life_bar_ymax(center_y + 2 * arg_shift[1] / 3),
        player_life_bar_ymin(center_y - arg_shift[1] / 3),
        player_life_bar_ymax(center_y + arg_shift[1] / 3),
        player_life_bar_x(2 * arg_shift[0] - 40),
        left_stats_ymin(menu_stats_margin),
        stats_xmin(2 * menu_stats_margin + 40.0f),
        left_stats_ymax(center_y - menu_stats_margin),
        stats_xmax(2 * center_x - menu_stats_margin),
        right_stats_ymin(center_y + menu_stats_margin),
        right_stats_ymax(2 * center_y - menu_stats_margin),
        stats_picture_width((left_stats_ymax - left_stats_ymin) * 0.4f),
        stat_display_width((left_stats_ymax - left_stats_ymin) * 0.4f),
        top_stats_ymin(center_y - 150.0f),
        top_stats_xmin(menu_stats_margin),
        top_stats_ymax(center_y + 150.0f),
        top_stats_xmax(menu_stats_margin + 40.0f),
        render_distance((std::max)(arg_shift[0], arg_shift[1]) / chunk_size + 2) {}
};

struct DisplayStats {
    int render_top; // Absolute
    int render_left;
    int render_bottom;
    int render_right;
    std::wstring display_text;
    const std::type_info& stat_type;
    int* display_int_stat;
    float* display_float_stat;
    DisplayStats(
        int render_top,
        int render_left,
        int render_bottom,
        int render_right,
        std::wstring display_text,
        const std::type_info& stat_type,
        int* display_int_stat = nullptr,
        float* display_float_stat = nullptr
    ) :
        render_top(render_top),
        render_left(render_left),
        render_bottom(render_bottom),
        render_right(render_right),
        display_text(display_text),
        stat_type(stat_type),
        display_int_stat(display_int_stat),
        display_float_stat(display_float_stat) {}
};

struct ButtonStats {
    char stat;
    bool increase;
    bool hide;
    int render_top; // Absolute
    int render_left;
    int render_bottom;
    int render_right;
    ButtonStats *opposite = nullptr;
    ButtonStats(
        int render_top,
        int render_left,
        int render_bottom,
        int render_right,
        bool increase,
        char stat,
        bool hide = false
    ) :
        render_top(render_top),
        render_left(render_left),
        render_bottom(render_bottom),
        render_right(render_right),
        increase(increase),
        stat(stat),
        hide(hide) {}
};

struct Level {
    int number;
    int next_level_number;
    std::string name;
    std::string area_type; // "enemy", "boss", "rest"
    Level(int number, std::string name, std::string area_type) :
        number(number), next_level_number(number+1), name(name), area_type(area_type) {}
};

std::vector<Level> levels = {{0, "1_1", "enemy"}, {1, "1_2", "enemy"}, {2, "1_3", "enemy"}, {3, "1_4", "enemy"}, {4, "1_5", "boss"}, {5, "1_6", "rest"}};

struct Colors {
    const D2D1::ColorF background{20.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF ground{1.0f, 1.0f, 1.0f};
    const D2D1::ColorF fire{200.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF cold{20.0f / 255, 20.0f / 255, 200.0f / 255};
    const D2D1::ColorF physical{20.0f / 255, 120.0f / 255, 20.0f / 255};
    const D2D1::ColorF blood{80.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF stats{240.0f / 255, 0.0f, 0.0f};
    const D2D1::ColorF menu_stats{20.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF boss_life_outline{50.0f / 255, 10.0f / 255, 10.0f / 255};
    const D2D1::ColorF boss_life_fill{120.0f / 255, 45.0f / 255, 30.0f / 255};
    const D2D1::ColorF player_life_outline{50.0f / 255, 10.0f / 255, 10.0f / 255};
    const D2D1::ColorF player_life_fill{200.0f / 255, 55.0f / 255, 80.0f / 255};
};
