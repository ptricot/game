#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

struct Enemy {
    int chunk_i;
    int chunk_j;
    int attack_time = 50;
    int current_attack_frame = -1;
    Enemy(int i, int j) : chunk_i(i), chunk_j(j) {}
};

class GameState {
    public:
        int x;
        int y;
        int speed;
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
        int render_distance; // how many chunks to render in each direction
        int center_x; // pixel position of center of screen
        int center_y;
        int range;
        int life;
        int max_life;
        int invulnerability_frames;
        float cone_angle;
        int x_chunks; // how many chunks vertical in level
        int y_chunks; // horizontal
        std::string state;
        std::vector<int> player_render; // where to render the player on screen
        std::vector<int> shift; // shift between game pixels and render pixels
        std::vector<int> current_chunk;
        std::string level;
        std::vector<std::vector<int>> walls;
        std::vector<std::vector<int>> projectiles; // each projectile is [x, y, radius, speed_x, speed_y]
        std::unordered_map<int, Enemy*> enemies;

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
            x_chunks = imax;
            y_chunks = jmax;
            for (int i = 0; i < imax; i++) {
                for (int j = 0; j < jmax; j++) {
                    file >> row[j];
                    // Player start position
                    if (row[j] == 2) {
                        current_chunk = {i, j};
                    }
                    // Enemy
                    if (row[j] == 4) {
                        enemies[i * jmax + j] = new Enemy(i, j);
                    }
                }
                walls.push_back(row);
            }
        }

        void init(std::vector<int>& arg_player_render, std::vector<int>& arg_shift) {
            // display variables
            level = "1_1";
            load_level(level);
            half_player_width = 50;
            half_player_height = 50;
            player_render = arg_player_render;
            center_x = (player_render[0] + player_render[2]) / 2;
            center_y = (player_render[1] + player_render[3]) / 2;
            shift = arg_shift;
            render_distance = (std::max)(arg_shift[0], arg_shift[1]) / 100 + 2;
            
            x = 100 * current_chunk[0] + half_player_height;
            y = 100 * current_chunk[1] + half_player_width;
            state = "playing";

            // character stats
            speed = 10;
            range = 250;
            attack_time = 50;
            current_attack_frame = -1;
            cone_angle = 1.2f;
            life = 3;
            max_life = 3;
            invulnerability_frames = -1;
        }

        void move() {
            // target movement
            int diag_coef = (right != left && down != up) ? 10 : 14; 
            int dx = (down - up) * speed * diag_coef / 10;
            int dy = (right - left) * speed * diag_coef / 10;

            // Initialize movement bounds
            int minx = (std::min)(x, x + dx);
            int maxx = (std::max)(x, x + dx);
            int miny = (std::min)(y, y + dy);
            int maxy = (std::max)(y, y + dy);

            // Check target position
            int new_x = x + dx;
            int new_y = y + dy;
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
            x = new_x;
            y = new_y;

            // update chunk
            current_chunk[0] = x / 100;
            current_chunk[1] = y / 100;
        }

        void attack(int& cx, int& cy) {
            // Update current attack
            if (current_attack_frame >= 0 && current_attack_frame < attack_time) current_attack_frame++;
            else if (current_attack_frame == attack_time) current_attack_frame = -1;

            // Check for new attack
            if (current_attack_frame == -1 && lbutton) {
                if (cx == center_x && cy == center_y) return;
                int attack_direction_x = cx - center_x;
                int attack_direction_y = cy - center_y;
                attack_angle = std::atan2(attack_direction_x, attack_direction_y);
                current_attack_frame = 0;
                std::cout << "attack at angle " << attack_angle <<'\n';
            }
        }

        void enemy_attack() {
            // check enemies in range (4 chunks away)
            for (int i = std::max(0, current_chunk[0] - 4); i < std::min(x_chunks, current_chunk[0] + 4); i++) {
                for (int j = std::max(0, current_chunk[1] - 4); j < std::min(y_chunks, current_chunk[1] + 4); j++) {
                    if (walls[i][j] == 4) {
                        // TODO
                    }
                }
            }
        }

        void update_projectiles() {
            // TODO
        }
        
        void run_frame(int cx, int cy) {
            move();
            attack(cx, cy);
            enemy_attack();
            // Update invulnerability frames
            if (invulnerability_frames >= 0 && invulnerability_frames < 10) invulnerability_frames++;
            else if (invulnerability_frames == 10) invulnerability_frames = -1;
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
        }

    private:
    
        bool is_wall(int i, int j) {
            return walls[i][j] == 1 || walls[i][j] == 4;
        }

        void delete_projectile(int i) {
            int n = projectiles.size();
            if (i >= n) return;
            std::swap(projectiles[i], projectiles[n]);
            projectiles.pop_back();
        }
        
};

extern GameState gameState;