
#pragma once

#include <cmath>

#include "Projectile.h"

class Enemy {
    public:
        int chunk_i;
        int chunk_j;
        int life;
        int max_life;
        int attack_time = 100;
        int current_attack_frame = -1;
        Enemy(int i, int j, int life = 10) : chunk_i(i), chunk_j(j), life(life), max_life(life) {}

    bool take_damage(int damage) {
        life -= damage;
        return life <= 0;
    }

    bool attack_available() {
        return current_attack_frame == -1;
    }

    void run_frame() {
        if (current_attack_frame == -1) {
            return;
        }
        else if (current_attack_frame == attack_time) {
            current_attack_frame = -1;
        }
        else {
            current_attack_frame++;
        }
    }

    Projectile* attack(int player_x, int player_y, int chunk_size) {
        // enemy center
        int enemy_x = chunk_size * chunk_i + chunk_size / 2;
        int enemy_y = chunk_size * chunk_j + chunk_size / 2;
        // attack angle
        float angle = std::atan2(player_x - enemy_x, player_y - enemy_y);
        // create projectile
        Projectile *proj = new Projectile(enemy_x, enemy_y, angle);
        current_attack_frame = 0;
        return proj;
    }
};