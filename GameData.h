
#pragma once

#include<iostream>
#include <d2d1.h>

struct Enemy {
    int chunk_i;
    int chunk_j;
    int attack_time = 80;
    int current_attack_frame = -1;
    Enemy(int i, int j) : chunk_i(i), chunk_j(j) {}
};

struct Projectile {
    float x;
    float y;
    int radius;
    float speed;
    float angle;
    char damage_type; // P : physical, F : fire, C : cold
    Projectile(float x, float y, int radius, float speed, float angle, char damage_type) :
        x(x), y(y), radius(radius), speed(speed), angle(angle), damage_type(damage_type) {}
};

class Attack {
    public: 
        int total_frames; // how many frames for an attack
        int range;
        float cone_angle;
        bool attacking = false;
        float angle = 0.0f;
        int current_frame = 0;
        float current_angle = 0.0f;
        int xbeg = 0; // relative to player_x and player_y
        int ybeg = 0;
        int xend = 0;
        int yend = 0;

        Attack(int total_frames, int range, float cone_angle) :
            total_frames(total_frames), range(range), cone_angle(cone_angle) {}

        void start(float arg_angle) {
            if(attacking) {
                return;
            }
            angle = arg_angle;
            attacking = true;
            current_frame = 0;
            update();
        }

        void run_frame() {
            if (!attacking) return;
            if (current_frame = total_frames) {
                std::cout << "finished attack\n";
                current_frame = -1;
                attacking = false;
                return;
            }
            update();
            current_frame++;
        }

    private:
        void update() {
            int attack_percentage = current_frame * 100 / total_frames;
            current_angle = angle + cone_angle * (attack_percentage - 50) / 100;
            int xbeg = 60 * std::sin(angle);
            int ybeg = 60 * std::cos(angle);
            int xend = range * std::sin(angle);
            int yend = range * std::cos(angle);
        }
};

struct PlayerStats {
    int life;
    int max_life;
    int speed;
    int range;
    float cone_angle;
    PlayerStats(int max_life, int speed, int range, int cone_angle) :
        life(max_life), max_life(max_life), speed(speed), range(range), cone_angle(cone_angle) {}
};

struct Colors {
    const D2D1::ColorF background{20.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF ground{1.0f, 1.0f, 1.0f};
    const D2D1::ColorF fire{200.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF cold{20.0f / 255, 20.0f / 255, 200.0f / 255};
    const D2D1::ColorF physical{20.0f / 255, 200.0f / 255, 20.0f / 255};
    const D2D1::ColorF blood{80.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF stats{240.0f / 255, 0.0f, 0.0f};
};
