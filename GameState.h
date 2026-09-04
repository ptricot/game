#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "GameData.h"

class GameState {
    public:
        int player_x;
        int player_y;
        bool up;
        bool down;
        bool left;
        bool right;
        bool lbutton;
        int attack_time; // how many frames for an attack
        int current_attack_frame;
        float attack_angle;
        int half_player_width; // in pixels
        int half_player_height; // in pixels
        int hitbox_radius;
        int render_distance; // how many chunks to render in each direction
        int center_x; // pixel position of center of screen
        int center_y;
        int range;
        int invulnerability_frame; // current invulnerability frame
        int invulnerability_time; // Number of frames for invilnerability
        float cone_angle;
        int max_chunks_i; // how many chunks vertical in level
        int max_chunks_j; // horizontal
        int current_chunk_i;
        int current_chunk_j;
        PlayerStats* player_stats;
        Attack* attack;
        std::string state;
        std::vector<int> player_render; // where to render the player on screen
        std::vector<int> shift; // shift between game pixels and render pixels
        std::string level;
        std::vector<std::vector<int>> walls;
        std::unordered_map<int, Enemy*> enemies;
        std::unordered_map<int, std::vector<Projectile*>> projectiles; 

        void load_level(const std::string& level_name) {
            std::ifstream file("data/levels/" + level_name + ".txt");

            if (!file) {
                std::cout << "Failed to open level: " 
                        << level_name << "\n";
                return;
            }

            std::vector<int> row(100);
            int imax, jmax;
            file >> imax >> jmax;
            max_chunks_i = imax;
            max_chunks_j = jmax;
            for (int i = 0; i < imax; i++) {
                for (int j = 0; j < jmax; j++) {
                    file >> row[j];
                    // Player start position
                    if (row[j] == 2) {
                        current_chunk_i = i;
                        current_chunk_j = j;
                    }
                    // Enemy
                    if (row[j] == 4) {
                        enemies[i * max_chunks_j + j] = new Enemy(i, j);
                    }
                }
                walls.push_back(row);
            }
        }

        void init(const std::vector<int>& arg_player_render, const std::vector<int>& arg_shift) {
            // display variables
            level = "1_1";
            load_level(level);
            half_player_width = 50;
            half_player_height = 50;
            hitbox_radius = 40;
            player_render = arg_player_render;
            center_x = (player_render[0] + player_render[2]) / 2;
            center_y = (player_render[1] + player_render[3]) / 2;
            shift = arg_shift;
            render_distance = (std::max)(arg_shift[0], arg_shift[1]) / 100 + 2;
            
            player_x = 100 * current_chunk_i + half_player_height;
            player_y = 100 * current_chunk_j + half_player_width;
            state = "playing";

            // character stats
            player_stats = new PlayerStats(3, 10, 250, 1.2f);
            attack = new Attack(50, 250, 1.2f);
            invulnerability_frame = -1;
            invulnerability_time = 20;
        }

        void move() {
            // target movement
            int diag_coef = (right != left && down != up) ? 10 : 14; 
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
            const int i = new_x / 100;
            const int j = new_y / 100;
            // Check adjacent chunks and snap position
            // left
            if (is_wall(i, j-1) == 1 && new_y % 100 < half_player_width) {
                new_y = 100 * j + half_player_width;
            }
            // right
            if (is_wall(i, j+1) == 1 && new_y % 100 > (100 - half_player_width)) {
                new_y = 100 * (j+1) - half_player_width;
            }
            // up
            if (is_wall(i-1, j) == 1 && new_x % 100 < half_player_height) {
                new_x = 100 * i + half_player_height;
            }
            // down
            if (is_wall(i+1, j) == 1 && new_x % 100 > (100 - half_player_height)) {
                new_x = 100 * (i+1) - half_player_height;
            }
            // up left
            if (is_wall(i-1, j-1) == 1 && new_y % 100 < half_player_width && new_x % 100 < half_player_height) {
                // Only do smaller snap
                if (half_player_width - new_y % 100 < half_player_height - new_x % 100) new_y = 100 * j + half_player_width;
                else new_x = 100 * i + half_player_height;
            }
            // up right
            if (is_wall(i-1, j+1) == 1 && new_y % 100 > (100 - half_player_width) && new_x % 100 < half_player_height) {
                if (new_y % 100 - 100 + half_player_width < half_player_height - new_x % 100) new_y = 100 * (j+1) - half_player_width;
                else new_x = 100 * i + half_player_height;
            }
            // down right
            if (is_wall(i+1, j+1) == 1 && new_y % 100 > (100 - half_player_width) && new_x % 100 > (100 - half_player_height)) {
                if (new_y % 100 - 100 + half_player_width < new_x % 100 - 100 + half_player_height) new_y = 100 * (j+1) - half_player_width;
                else new_x = 100 * (i+1) - half_player_height;
            }
            // down left
            if (is_wall(i+1, j-1) == 1 && new_y % 100 < half_player_width && new_x % 100 > (100 - half_player_height)) {
                if (half_player_width - new_y % 100 < new_x % 100 - 100 + half_player_height) new_y = 100 * j + half_player_width;
                else new_x = 100 * (i+1) - half_player_height;
            }
            
            // update position
            player_x = new_x;
            player_y = new_y;

            // update chunk
            current_chunk_i = player_x / 100;
            current_chunk_j = player_y / 100;
        }

        void player_attack(const int& cx, const int& cy) {
            // Update current attack frame
            attack->run_frame();

            // Check for new attack
            if (!attack->attacking && lbutton && (cx != center_x || cy != center_y)) {
                int angle = std::atan2(cx - center_x, cy - center_y);
                attack->start(angle);
            }
        }

        void enemy_attack() {
            // check enemies in range (4 chunks away)
            for (int i = std::max(0, current_chunk_i - 4); i < std::min(max_chunks_i, current_chunk_i + 4); i++) {
                for (int j = std::max(0, current_chunk_j - 4); j < std::min(max_chunks_j, current_chunk_j + 4); j++) {
                    if (walls[i][j] == 4) {
                        int key = i * max_chunks_j + j;
                        Enemy *enemy = enemies[key];
                        if (!enemy) {
                            std::cout << "Warning : missing enemy at chunk " << i << ", " << j << '\n';
                            continue;
                        }
                        // attack
                        if (enemy->current_attack_frame == -1) {
                            // enemy center
                            int enemy_x = 100 * i + 50;
                            int enemy_y = 100 * j + 50;
                            // attack angle
                            float angle = std::atan2(player_x - enemy_x, player_y - enemy_y);
                            // create projectile
                            projectiles[i * max_chunks_j + j].push_back(new Projectile(enemy_x, enemy_y, 10, 4.0f, angle, 'F'));
                            enemy->current_attack_frame = 0;
                        }
                        // finish attack
                        else if (enemy->current_attack_frame == enemy->attack_time) {
                            enemy->current_attack_frame = -1;
                        }
                        // update attack cd
                        else {
                            enemy->current_attack_frame++;
                        }
                    }
                }
            }
        }

        void update_projectiles() {
            // Keep track of projectiles changing chunks
            std::vector<std::pair<int, Projectile*>> moved_projectiles;
            // Go through all projectiles
            for (const auto& [key, _] : projectiles) {
                for (int i = 0; i < projectiles[key].size(); i++) {
                    Projectile *proj = projectiles[key][i];
                    // Compute and update new position
                    proj->x += proj->speed * std::sin(proj->angle);
                    proj->y += proj->speed * std::cos(proj->angle);
                    // Compute old chunk
                    int chunk_i = key / max_chunks_j;
                    int chunk_j = key % max_chunks_j;
                    // Compute new chunk
                    int new_chunk_i = (int)proj->x / 100;
                    int new_chunk_j = (int)proj->y / 100;
                    // Delete projectile when out of bounds
                    if (new_chunk_i < 0 || new_chunk_i >= max_chunks_i || new_chunk_j < 0 || new_chunk_j >= max_chunks_j) {
                        delete_projectile(key,i);
                        i--;
                    }
                    // Change chunk if necessary
                    else if (new_chunk_i != chunk_i || new_chunk_j != chunk_j ) {
                        int new_key = new_chunk_i * max_chunks_j + new_chunk_j;
                        // remove from old chunk
                        int n = projectiles[key].size();
                        std::swap(projectiles[key][i], projectiles[key][n-1]);
                        projectiles[key].pop_back();
                        i--;
                        // add to new chunk
                        moved_projectiles.push_back({new_key, proj});
                    }
                }
            }
            // Add moved projectiles back
            for (const auto& [key, proj] : moved_projectiles) {
                projectiles[key].push_back(proj);
            }
        }

        void update_invulnerability_frame() {
            if (invulnerability_frame >= 0 && invulnerability_frame < invulnerability_time) invulnerability_frame++;
            else if (invulnerability_frame == invulnerability_time) invulnerability_frame = -1;
        }

        void take_damage() {
            // Check invulnerability
            if (invulnerability_frame >= 0) return;
            // Check projectile collision in adjacent chunks
            bool taking_damage = false;
            for (int i = (std::max)(0, current_chunk_i - 1); i <= (std::min)(max_chunks_i , current_chunk_i + 1); i++) {
                for (int j = (std::max)(0, current_chunk_j - 1); j <= (std::min)(max_chunks_j , current_chunk_j + 1); j++) {
                    const int key = i * max_chunks_j + j;
                    if (projectiles.find(key) == projectiles.end()) continue;
                    for (int ind = 0; ind < projectiles[key].size(); ind++) {
                        Projectile* proj = projectiles[key][ind];
                        int limit_dist = hitbox_radius + proj->radius;
                        if (distance_squared(player_x, player_y, proj->x, proj->y) < limit_dist * limit_dist) {
                            taking_damage = true;
                            delete_projectile(key,ind);
                        }
                    }
                }
            }
            // Take damage
            if (taking_damage) {
                player_stats->life --;
                invulnerability_frame = 0;
                if (player_stats->life == 0) state = "dead";
            }
        }
        
        void run_frame(const int cx, const int cy) {
            move();
            player_attack(cx, cy);
            enemy_attack();
            update_projectiles();
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
        }

        void on_lbuttonup() {
            lbutton = false;
        }

        void end_game() {
            for (auto [key, enemy] : enemies) {
                delete enemy;
            }
            enemies.clear();
            for (const auto& [key, _] : projectiles) {
                for (Projectile* proj : projectiles[key]) {
                    delete proj;
                }
                projectiles[key].clear();
            }
            projectiles.clear();
            delete player_stats;
            delete attack;
        }

    private:
    
        bool is_wall(int i, int j) {
            return walls[i][j] == 1 || walls[i][j] == 4;
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

        int distance_squared(int x1, int y1, int x2, int y2) {
            int dx = x1 - x2;
            int dy = y1 - y2;
            return dx * dx + dy * dy;
        }
        
};

extern GameState gameState;