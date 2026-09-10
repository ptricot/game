#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <typeinfo>

#include "GameData.h"
#include "Projectile.h"
#include "Enemy.h"
#include "Attack.h"
#include "Boss.h"
#include "PlayerStats.h"

class GameState {
    public:
        int player_x;
        int player_y;
        int pointer_x;
        int pointer_y;
        bool up;
        bool down;
        bool left;
        bool right;
        bool lbutton;
        int hitbox_radius;
        int invulnerability_frame; // current invulnerability frame
        int invulnerability_time; // Number of frames for invilnerability
        int max_chunks_i; // how many chunks vertical in level
        int max_chunks_j; // horizontal
        int current_chunk_i;
        int current_chunk_j;
        int chunk_size; // in pixels
        int enemy_range;
        bool pause = false;
        PlayerStats* player_stats;
        RenderStats* render_stats;
        Attack* attack;
        std::string state = "playing";
        std::string area;
        Level* level;
        int level_number;
        std::vector<std::vector<int>> walls;
        std::unordered_map<int, Enemy*> enemies;
        std::vector<DisplayStats*> menu_displays;
        std::vector<ButtonStats*> menu_buttons;
        Boss *boss = nullptr;
        std::unordered_map<int, std::vector<Projectile*>> projectiles;

        void init(const std::vector<int>& arg_player_render, const std::vector<int>& arg_shift) {

            // render variables
            chunk_size = 50;
            hitbox_radius = 30;
            render_stats =  new RenderStats(
                arg_player_render,
                arg_shift,
                chunk_size
            );

            // level
            level_number = 0;
            level = &levels[level_number];
            load_level();

            enemy_range = 6; // in chunks

            // player stats
            player_stats = new PlayerStats();
            attack = new Attack(
                player_stats->attack_time,
                player_stats->range,
                player_stats->cone_angle,
                hitbox_radius
            );
            invulnerability_frame = -1;
            invulnerability_time = 20;

            // stats menu
            init_stats_menu();
        }
        
        void run_frame(const int cx, const int cy) {
            pointer_x = cx;
            pointer_y = cy;
            if (pause) return;

            move();
            player_attack();
            player_stats->run_frame();
            if (area == "enemy") {
                enemy_attack();
                run_enemies_frame();
            }
            else if (area == "boss") {
                run_boss_frame();
            }
            update_projectiles(projectiles);
            update_invulnerability_frame();
            take_damage();
        }

        void key_down(char key) {
            switch (key)
            {
                case 'W': up = true;    break;
                case 'A': left = true;  break;
                case 'S': down = true;  break;
                case 'D': right = true; break;
                case 'P': {
                    if (pause) pause = false;
                    else pause = true;
                    break;
                }
                case 'C': {
                    if (state == "menu_stats") {
                        pause = false;
                        state = "playing";
                    }
                    else {
                        pause = true;
                        state = "menu_stats";
                    }
                    break;
                }
            }
        }

        void key_up(char key) {
            switch (key)
            {
                case 'W': up = false;    break;
                case 'A': left = false;  break;
                case 'S': down = false;  break;
                case 'D': right = false; break;
            }
        }

        void on_lbuttondown() {
            lbutton = true;

            // check clicking on buttons
            if (state == "menu_stats") {
                for (ButtonStats *button : menu_buttons) {
                    if (
                        button->render_top <= pointer_x &&
                        button->render_bottom >= pointer_x &&
                        button->render_left <= pointer_y &&
                        button->render_right >= pointer_y
                    ) {
                        // clicked on the button
                        player_stats->update_stat(button->stat, button->increase);

                        // update button show/hide state
                        if (button->increase) {
                            button->hide = !player_stats->can_increase(button->stat);
                            button->opposite->hide = !player_stats->can_decrease(button->stat);
                        }
                        else {
                            button->hide = !player_stats->can_decrease(button->stat);
                            button->opposite->hide = !player_stats->can_increase(button->stat);
                        }
                    }
                }
            }
        }

        void on_lbuttonup() {
            lbutton = false;
        }

        void end_game() {
            clear_enemies();
            clear_projectiles();
            clear_stats_menu(); // Delete menus before player stats
            delete player_stats;
            delete render_stats;
            delete attack;
            delete level;
        }

    private:

        void load_level() {

            clear_enemies();
            clear_projectiles();
            walls.clear();
            area = level->area_type;

            std::cout << "Loading level: " << level->name << "\n";

            std::ifstream file("levels/" + level->name + ".txt");
            if (!file) {
                std::cout << "Failed to open level: " << level->name << "\n";
                return;
            }
            int imax, jmax;
            file >> imax >> jmax;
            max_chunks_i = imax;
            max_chunks_j = jmax;
            std::vector<int> row(jmax);
            for (int i = 0; i < imax; i++) {
                for (int j = 0; j < jmax; j++) {
                    file >> row[j];
                    // Player start position
                    if (row[j] == 2) {
                        current_chunk_i = i;
                        current_chunk_j = j;
                        player_x = chunk_size * current_chunk_i + chunk_size / 2;
                        player_y = chunk_size * current_chunk_j + chunk_size / 2;
                    }
                    // Enemy
                    else if (row[j] == 4) {
                        enemies[i * max_chunks_j + j] = new Enemy(i, j);
                    }
                    // Boss
                    else if (row[j] == 5) {
                        boss = new Boss(i * chunk_size + chunk_size / 2, j * chunk_size + chunk_size / 2);
                        render_stats->render_portal = false;
                        row[j] = 0;
                    }
                }
                walls.push_back(row);
            }
        }

        void move() {
            // target movement
            int diag_coef = (right != left && down != up) ? 10 : 15; 
            int dx = (down - up) * player_stats->speed * diag_coef / 10;
            int dy = (right - left) * player_stats->speed * diag_coef / 10;

            // Initialize movement bounds
            int minx = (std::min)(player_x, player_x + dx);
            int maxx = (std::max)(player_x, player_x + dx);
            int miny = (std::min)(player_y, player_y + dy);
            int maxy = (std::max)(player_y, player_y + dy);

            // Check target position
            int new_x = player_x + dx;
            int new_y = player_y + dy;
            const int i = new_x / chunk_size;
            const int j = new_y / chunk_size;
            // Check adjacent chunks and snap position
            // left
            if (is_wall(i, j-1) == 1 && new_y % chunk_size < render_stats->half_player_width) {
                new_y = chunk_size * j + render_stats->half_player_width;
            }
            // right
            else if (is_wall(i, j+1) == 1 && new_y % chunk_size > (chunk_size - render_stats->half_player_width)) {
                new_y = chunk_size * (j+1) - render_stats->half_player_width;
            }
            // up
            if (is_wall(i-1, j) == 1 && new_x % chunk_size < render_stats->half_player_height) {
                new_x = chunk_size * i + render_stats->half_player_height;
            }
            // down
            else if (is_wall(i+1, j) == 1 && new_x % chunk_size > (chunk_size - render_stats->half_player_height)) {
                new_x = chunk_size * (i+1) - render_stats->half_player_height;
            }
            // up left
            if (
                is_wall(i-1, j-1) == 1 &&
                new_y % chunk_size < render_stats->half_player_width &&
                new_x % chunk_size < render_stats->half_player_height
            ) {
                // Only do smaller snap
                if (render_stats->half_player_width - new_y % chunk_size < render_stats->half_player_height - new_x % chunk_size) {
                    new_y = chunk_size * j + render_stats->half_player_width;
                }
                else new_x = chunk_size * i + render_stats->half_player_height;
            }
            // down right
            else if (
                is_wall(i+1, j+1) == 1 &&
                new_y % chunk_size > (chunk_size - render_stats->half_player_width) &&
                new_x % chunk_size > (chunk_size - render_stats->half_player_height)
            ) {
                if (new_y % chunk_size - chunk_size + render_stats->half_player_width < new_x % chunk_size - chunk_size + render_stats->half_player_height) {
                    new_y = chunk_size * (j+1) - render_stats->half_player_width;
                }
                else new_x = chunk_size * (i+1) - render_stats->half_player_height;
            }
            // up right
            if (
                is_wall(i-1, j+1) == 1 &&
                new_y % chunk_size > (chunk_size - render_stats->half_player_width) &&
                new_x % chunk_size < render_stats->half_player_height
            ) {
                if (new_y % chunk_size - chunk_size + render_stats->half_player_width < render_stats->half_player_height - new_x % chunk_size) {
                    new_y = chunk_size * (j+1) - render_stats->half_player_width;
                }
                else new_x = chunk_size * i + render_stats->half_player_height;
            }
            // down left
            else if (
                is_wall(i+1, j-1) == 1 &&
                new_y % chunk_size < render_stats->half_player_width &&
                new_x % chunk_size > (chunk_size - render_stats->half_player_height)
            ) {
                if (render_stats->half_player_width - new_y % chunk_size < new_x % chunk_size - chunk_size + render_stats->half_player_height) {
                    new_y = chunk_size * j + render_stats->half_player_width;
                }
                else new_x = chunk_size * (i+1) - render_stats->half_player_height;
            }
            
            // update position
            player_x = new_x;
            player_y = new_y;

            // update chunk
            current_chunk_i = player_x / chunk_size;
            current_chunk_j = player_y / chunk_size;

            // Check if entering portal
            if (
                !boss &&
                walls[current_chunk_i][current_chunk_j] == 3
            ) {
                level_number = level->next_level_number;
                if (level_number < levels.size()) level = &levels[level_number];
                load_level();
            }
        }

        void player_attack() {
            // Update current attack frame
            attack->run_frame();

            // Check for new attack
            if (attack->available() && lbutton && (pointer_x != render_stats->center_x || pointer_y != render_stats->center_y)) {
                // Update attack stats
                attack->update_range(player_stats->range);
                attack->update_cone_angle(player_stats->cone_angle);
                attack->update_attack_time(player_stats->attack_time);
                // Start new attack
                float angle = std::atan2(pointer_x - render_stats->center_x, pointer_y - render_stats->center_y);
                attack->start(angle);
            }

            // Check enemy collision
            if (attack->attacking) {
                if (area == "enemy") {
                    // Find enemies on attack line
                    const std::vector<int> keys = enemies_on_line(
                        player_x + attack->xbeg,
                        player_y + attack->ybeg,
                        player_x + attack->xend,
                        player_y + attack->yend
                    );

                    for (const int key : keys) {
                        if (enemies.find(key) == enemies.end()) {
                            std::cout << "Warning : hitting non existing enemy\n";
                            continue;
                        }
                        // Check if current attack already hit enemy
                        if (attack->already_hit(key)) {
                            return;
                        }
                        // Otherwise hit enemy
                        attack->hit_enemy(key);
                        Enemy *enemy = enemies[key];
                        bool dead = enemy->take_damage(player_stats->attack_damage);
                        if (dead) {
                            enemy_death(key);
                        }
                    }
                }
                else if (
                    area == "boss" &&
                    boss &&
                    !attack->boss_is_hit &&
                    line_through_rect(
                        player_x + attack->xbeg,
                        player_y + attack->ybeg,
                        player_x + attack->xend,
                        player_y + attack->yend,
                        boss->x - boss->half_height,
                        boss->y - boss->half_width,
                        boss->x + boss->half_height,
                        boss->y + boss->half_width
                    )
                ) {
                    attack->hit_boss();
                    bool dead = boss->take_damage(player_stats->attack_damage);
                    if (dead) {
                        boss_death();
                    }
                }
            }
        }

        void enemy_attack() {
            // check enemies in range
            for (int i = std::max(0, current_chunk_i - enemy_range); i < std::min(max_chunks_i, current_chunk_i + enemy_range); i++) {
                for (int j = std::max(0, current_chunk_j - enemy_range); j < std::min(max_chunks_j, current_chunk_j + enemy_range); j++) {
                    if (walls[i][j] == 4) {
                        int key = i * max_chunks_j + j;
                        Enemy *enemy = enemies[key];
                        if (!enemy) {
                            std::cout << "Warning : missing enemy at chunk " << i << ", " << j << '\n';
                            continue;
                        }
                        // attack
                        if (!enemy->attack_available()) {
                            continue;
                        }
                        Projectile *proj = enemy->attack(player_x, player_y, chunk_size);
                        projectiles[i * max_chunks_j + j].push_back(proj);
                    }
                }
            }
        }

        void run_enemies_frame() {
            // run frame on all enemies
            for (const auto& [key, enemy] : enemies) {
                enemy->run_frame();
            }
        }

        void run_boss_frame() {
            if (!boss) {
                std::cout << "Warning : running boss frame but no boss\n";
                return;
            }
            std::vector<Projectile*> projs = boss->run_frame(player_x, player_y);
            // compute boss chunk key
            int key = boss->x / chunk_size * max_chunks_j + boss->y / chunk_size;
            // add new projectiles
            for (Projectile* proj : projs) {
                projectiles[key].push_back(proj);
            }
        }

        void enemy_death(int key) {
            if (enemies.find(key) == enemies.end()) {
                std::cout << "Warning : trying to delete unexisting enemy\n";
                return;
            }
            player_stats->gain_experience(enemies[key]->experience);
            int chunk_i = key / max_chunks_j;
            int chunk_j = key % max_chunks_j;
            walls[chunk_i][chunk_j] = 0;
            delete enemies[key];
            enemies.erase(key);
        }

        void boss_death() {
            if (!boss) return;

            player_stats->gain_experience(boss->experience);
            delete boss;
            boss = nullptr;
            render_stats->render_portal = true;
        }

        void update_invulnerability_frame() {
            if (invulnerability_frame >= 0 && invulnerability_frame < player_stats->invulnerability_time) invulnerability_frame++;
            else if (invulnerability_frame == player_stats->invulnerability_time) invulnerability_frame = -1;
        }

        void take_damage() {
            // Check invulnerability
            if (invulnerability_frame >= 0 || player_stats->invulnerable) return;

            // Check projectile collision in adjacent chunks
            int damage = 0;
            for (int i = (std::max)(0, current_chunk_i - 1); i < (std::min)(max_chunks_i , current_chunk_i + 1); i++) {
                for (int j = (std::max)(0, current_chunk_j - 1); j < (std::min)(max_chunks_j , current_chunk_j + 1); j++) {
                    const int key = i * max_chunks_j + j;
                    if (projectiles.find(key) == projectiles.end()) continue;
                    for (int ind = 0; ind < projectiles[key].size(); ind++) {
                        Projectile* proj = projectiles[key][ind];
                        int limit_dist = hitbox_radius + proj->radius;
                        if (distance_squared(player_x, player_y, proj->x, proj->y) < limit_dist * limit_dist) {
                            // register largest hit
                            damage = (std::max)(damage, proj->damage);
                            delete_projectile(key,ind);
                        }
                    }
                }
            }
            // Take damage
            if (damage > 0) {
                bool dead = player_stats->take_damage(damage);
                invulnerability_frame = 0;
                if (dead) state = "dead";
            }
        }

        void update_projectiles(std::unordered_map<int, std::vector<Projectile*>>& projectiles_map) {
            // Keep track of projectiles changing chunks
            std::vector<std::pair<int, Projectile*>> moved_projectiles;
            // Go through all projectiles
            for (const auto& [key, _] : projectiles_map) {
                for (int i = 0; i < projectiles_map[key].size(); i++) {
                    Projectile *proj = projectiles_map[key][i];
                    // Compute and update new position
                    proj->run_frame();

                    // Compute old and new chunk
                    int chunk_i = key / max_chunks_j;
                    int chunk_j = key % max_chunks_j;
                    int new_chunk_i = (int)proj->x / chunk_size;
                    int new_chunk_j = (int)proj->y / chunk_size;

                    // Delete projectile when out of bounds
                    if (new_chunk_i < 0 || new_chunk_i >= max_chunks_i || new_chunk_j < 0 || new_chunk_j >= max_chunks_j) {
                        delete_projectile(key,i);
                        i--;
                    }
                    // Change chunk if necessary
                    else if (new_chunk_i != chunk_i || new_chunk_j != chunk_j ) {
                        int new_key = new_chunk_i * max_chunks_j + new_chunk_j;
                        // remove from old chunk
                        int n = projectiles_map[key].size();
                        std::swap(projectiles_map[key][i], projectiles_map[key][n-1]);
                        projectiles_map[key].pop_back();
                        i--;
                        // add to new chunk
                        moved_projectiles.push_back({new_key, proj});
                    }
                }
            }
            // Add moved projectiles back
            for (const auto& [key, proj] : moved_projectiles) {
                projectiles_map[key].push_back(proj);
            }
        }

        void delete_projectile(int key, int i) {
            if (projectiles.find(key) == projectiles.end()) return;
            int n = projectiles[key].size();
            if (i < 0 || i >= n) {
                std::cout << "Warning : trying to delete inexisting projectile.\n";
                return;
            }
            std::swap(projectiles[key][i], projectiles[key][n-1]);
            Projectile *proj = projectiles[key].back();
            delete proj;
            projectiles[key].pop_back();
        }
    
        bool is_wall(int i, int j) {
            return i < 0 || j < 0 || i >= max_chunks_i || j >= max_chunks_j || walls[i][j] == 1 || walls[i][j] == 4;
        }

        int distance_squared(int x1, int y1, int x2, int y2) {
            int dx = x1 - x2;
            int dy = y1 - y2;
            return dx * dx + dy * dy;
        }

        bool line_through_rect(int xbeg, int ybeg, int xend, int yend, int rect_top, int rect_left, int rect_bottom, int rect_right) {
            // Check if line passes through rectangle
            if (xend != xbeg) {
                // project top boundary of chunk onto line
                double t = (rect_top - xbeg) / double(xend - xbeg);
                int proj_y = ybeg + t * (yend - ybeg);
                // check if projected point is on line and in chunk
                if (t >= 0 && t <= 1 && proj_y >= rect_left && proj_y <= rect_right) return true;

                // project botto; boundary
                t = (rect_bottom - xbeg) / double(xend - xbeg);
                proj_y = ybeg + t * (yend - ybeg);
                if (t >= 0 && t <= 1 && proj_y >= rect_left && proj_y <= rect_right) return true;
            }

            if (yend != ybeg) {
                // project left boundary
                double t = (rect_left - ybeg) / double(yend - ybeg);
                int proj_x = xbeg + t * (xend - xbeg);
                if (t >= 0 && t <= 1 && proj_x >= rect_top && proj_x <= rect_bottom) return true;

                // project right boundary
                t = (rect_right - ybeg) / double(yend - ybeg);
                proj_x = xbeg + t * (xend - xbeg);
                if (t >= 0 && t <= 1 && proj_x >= rect_top && proj_x <= rect_bottom) return true;
            }
            return false;
        }

        bool line_through_chunk(int xbeg, int ybeg, int xend, int yend, int chunk_i, int chunk_j) {
            // Check if the line passes through chunk i, j
            return line_through_rect(
                xbeg, ybeg, xend, yend,
                chunk_i * chunk_size, chunk_j * chunk_size,
                (chunk_i + 1) * chunk_size - 1, (chunk_j + 1) * chunk_size - 1
            );
        }

        std::vector<int> enemies_on_line(int xbeg, int ybeg, int xend, int yend) {
            // Find all keys of chunks on the line between [xbeg,ybeg] and [xend,yend]
            int start_i = xbeg / chunk_size;
            int start_j = ybeg / chunk_size;
            int end_i = xend / chunk_size;
            int end_j = yend / chunk_size;
            std::vector<int> res;
            for (
                int i = (std::max)(0, (std::min)(start_i, end_i));
                i <= (std::min)(max_chunks_i - 1, (std::max)(start_i, end_i));
                i++
            ) {
                for (
                    int j = (std::max)(0, (std::min)(start_j, end_j));
                    j <= (std::min)(max_chunks_j - 1, (std::max)(start_j, end_j));
                    j++
                ) {
                    // Check if enemy at chunk i, j and if line goes through it
                    if (walls[i][j] == 4 && line_through_chunk(xbeg, ybeg, xend, yend, i, j)) {
                        int key = i * max_chunks_j + j;
                        res.push_back(key);
                    }
                }
            }
            return res;
        }
        
        void clear_enemies() {
            for (auto [key, enemy] : enemies) {
                delete enemy;
            }
            enemies.clear();
            boss_death();
        }

        void clear_projectiles() {
            for (const auto& [key, _] : projectiles) {
                for (Projectile* proj : projectiles[key]) {
                    delete proj;
                }
                projectiles[key].clear();
            }
            projectiles.clear();
        }

        void init_stats_menu() {
            
            std::vector<std::pair<std::wstring, char>> left_stats = {
                {L"Attack damage : ",'D'},
                {L"Attack speed : ",'S'},
                {L"Attack range : ",'R'},
                {L"Increase attack angle : ",'C'},
                {L"Decrease attack angle : ",'c'}
            };

            std::vector<std::pair<std::wstring, char>> right_stats = {
                {L"Maximum life : ",'L'},
                {L"Life regeneration : ",'l'},
                {L"Movement speed : ",'s'}
            };

            // Left window stats
            int current_display_ymin = render_stats->left_stats_ymin + render_stats->stats_picture_width + render_stats->stats_y_margin;
            int current_display_ymax = current_display_ymin + render_stats->stat_display_width;
            int current_button_decrease_ymin = current_display_ymax + render_stats->stats_y_margin;
            int current_button_decrease_ymax = current_button_decrease_ymin + render_stats->stat_button_width;
            int current_button_increase_ymin = current_button_decrease_ymax + render_stats->stats_y_margin;
            int current_button_increase_ymax = current_button_increase_ymin + render_stats->stat_button_width;
            int current_xmin = render_stats->stats_xmin + render_stats->stats_initial_x_margin;
            int current_xmax = current_xmin + render_stats->text_height;

            for (const auto& [text, stat] : left_stats) {
                // Create display & buttons
                menu_displays.push_back(new DisplayStats(current_xmin, current_display_ymin, current_xmax, current_display_ymax, text, typeid(int), &player_stats->allocated_points[stat]));
                menu_buttons.push_back(new ButtonStats(current_xmin, current_button_decrease_ymin, current_xmax, current_button_decrease_ymax, false, stat, true));
                menu_buttons.push_back(new ButtonStats(current_xmin, current_button_increase_ymin, current_xmax, current_button_increase_ymax, true, stat));
                // set opposite buttons
                int n = menu_buttons.size();
                menu_buttons[n-1]->opposite = menu_buttons[n-2];
                menu_buttons[n-2]->opposite = menu_buttons[n-1];
                // update x values
                current_xmin = current_xmax + render_stats->stats_x_margin;
                current_xmax = current_xmin + render_stats->text_height;
            }

            // Right window stats
            current_display_ymin = render_stats->right_stats_ymin + render_stats->stats_picture_width + render_stats->stats_y_margin;
            current_display_ymax = current_display_ymin + render_stats->stat_display_width;
            current_button_decrease_ymin = current_display_ymax + render_stats->stats_y_margin;
            current_button_decrease_ymax = current_button_decrease_ymin + render_stats->stat_button_width;
            current_button_increase_ymin = current_button_decrease_ymax + render_stats->stats_y_margin;
            current_button_increase_ymax = current_button_increase_ymin + render_stats->stat_button_width;
            current_xmin = render_stats->stats_xmin + render_stats->stats_initial_x_margin;
            current_xmax = current_xmin + render_stats->text_height;

            for (const auto& [text, stat] : right_stats) {
                // Create display & buttons
                menu_displays.push_back(new DisplayStats(current_xmin, current_display_ymin, current_xmax, current_display_ymax, text, typeid(int), &player_stats->allocated_points[stat]));
                menu_buttons.push_back(new ButtonStats(current_xmin, current_button_decrease_ymin, current_xmax, current_button_decrease_ymax, false, stat, true));
                menu_buttons.push_back(new ButtonStats(current_xmin, current_button_increase_ymin, current_xmax, current_button_increase_ymax, true, stat));
                // set opposite buttons
                int n = menu_buttons.size();
                menu_buttons[n-1]->opposite = menu_buttons[n-2];
                menu_buttons[n-2]->opposite = menu_buttons[n-1];
                // update x values
                current_xmin = current_xmax + render_stats->stats_x_margin;
                current_xmax = current_xmin + render_stats->text_height;
            }
        }

        void clear_stats_menu() {
            for (int i = 0; i < menu_displays.size(); i++) {
                delete menu_displays[i];
            }
            menu_displays.clear();
            for (int i = 0; i < menu_buttons.size(); i++) {
                delete menu_displays[i];
            }
            menu_buttons.clear();
        }
};

extern GameState gameState;