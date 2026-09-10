
#pragma once

#include <iostream>
#include <string>
#include <cmath>

#include "Projectile.h"

struct BossEvent {
    int frames;
    int radius;
    int damage;
    char damage_type = 'F';
    std::wstring dialogue;
    std::string type; // "wait", "attack player", "attack radius"
    std::vector<float> values;
    BossEvent(int frames, std::string type, int damage = 1, int radius = 10, std::wstring dialogue = L"", std::vector<float> values = {}) :
        frames(frames), type(type), damage(damage), radius(radius), dialogue(dialogue), values(values) {}
};

class Boss {
    public:
        int x;
        int y;
        int half_width;
        int half_height;
        int life;
        int max_life;
        int event_number = 0;
        int event_frame = 0;
        int experience = 20;
        std::wstring dialogue = L"Hello";
        std::vector<BossEvent> pattern = {
            {140, "wait"},
            {20, "attack player"},
            {20, "attack player"},
            {20, "attack player"},
            {20, "attack player"},
            {80, "attack player"},
            {80, "rain", 1, 8, L"Rain", {
                100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f
            }},
            {100, "wait"},
            {20, "attack player"},
            {20, "attack player"},
            {20, "attack player"},
            {20, "attack player"},
            {80, "attack player"},
            {20, "attack angles", 1, 10, L"", {
                0.0f,
                6.28319f / 8,
                6.28319f / 4,
                6.28319f * 3 / 8,
                6.28319f / 2,
                6.28319f * 5 / 8,
                6.28319f * 3 / 4,
                6.28319f * 7 / 8
            }},
            {80, "wait", 0, 0, L"Big attack"},
            {50, "attack player", 2, 15, L"", {0.3f, -0.3f}}
        };
        BossEvent *event = &pattern[event_number];
        Boss(int x, int y, int half_width = 50, int half_height = 60, int life = 1000) :
            x(x), y(y), half_width(half_width), half_height(half_height), life(life), max_life(life) {}

    bool take_damage(int damage) {
        life -= damage;
        return life <= 0;
    }

    std::vector<Projectile*> run_frame(int player_x, int player_y) {
        if (event_frame < event->frames) {
            event_frame++;
            return {};
        }
        // Start new event
        else {
            event_frame = 0;
            // repeat events in a loop, except for first event
            event_number = event_number % (pattern.size() - 1) + 1;
            event = &pattern[event_number];
            return run_event(player_x, player_y);
        }
    }

    private:
        std::vector<Projectile*> run_event(int player_x, int player_y) {
            dialogue = event->dialogue;

            // launch projectiles towards player
            if (event->type == "attack player") {
                // main angle towards player
                float main_angle = std::atan2(player_x - x, player_y - y);
                std::vector<Projectile*> res = {new Projectile(x, y, main_angle, event->radius)};
                // angles to add to main angles
                for (float angle : event->values) {
                    res.push_back(new Projectile(x, y, main_angle + angle, event->radius));
                }
                return res;
            }

            // launch projectiles at angle
            else if (event->type == "attack angles") {
                std::vector<Projectile*> res;
                for (float angle : event->values) {
                    res.push_back(new Projectile(x, y, angle, event->radius));
                }
                return res;
            }

            // launch projectiles raining from above
            else if (event->type == "rain") {
                std::vector<Projectile*> res;
                for (float start_y : event->values) {
                    res.push_back(new Projectile(
                        0.0f,
                        start_y,
                        6.28319f / 4,        // pi / 2, down
                        event->radius,
                        2.0f,
                        'F',
                        [](int frame){return 0.0f;},
                        [](int frame){return 0.1f * std::cos(frame / 20.0f);}       // wiggle effect
                    ));
                }
                return res;
            }

            return {};
        }
};
