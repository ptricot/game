
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <d2d1.h>


struct PlayerStats {
    int life;
    int max_life;
    int speed;
    int range;
    int attack_time;
    int hit_damage;
    int invulnerability_time = 20;
    float cone_angle;
    PlayerStats(int max_life = 3, int speed = 4, int range = 150, int attack_time = 50, int hit_damage = 5, float cone_angle = 1.2f) :
        life(max_life), max_life(max_life), speed(speed), range(range), attack_time(attack_time), hit_damage(hit_damage), cone_angle(cone_angle) {}
};

struct Level {
    int number;
    int next_level_number;
    std::string name;
    std::string type; // "enemy", "boss", "rest"
    Level(int number, std::string name, std::string type) :
        number(number), next_level_number(number+1), name(name), type(type) {}
};

std::vector<Level> Levels = {{0, "1_1", "enemy"}, {1, "1_2", "boss"}, {2, "1_3", "rest"}};

struct Colors {
    const D2D1::ColorF background{20.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF ground{1.0f, 1.0f, 1.0f};
    const D2D1::ColorF fire{200.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF cold{20.0f / 255, 20.0f / 255, 200.0f / 255};
    const D2D1::ColorF physical{20.0f / 255, 200.0f / 255, 20.0f / 255};
    const D2D1::ColorF blood{80.0f / 255, 20.0f / 255, 20.0f / 255};
    const D2D1::ColorF stats{240.0f / 255, 0.0f, 0.0f};
};
