
#pragma once

#include <iostream>
#include <string>
#include <vector>

std::vector<int> required_experience = {10, 20, 30, 41, 52, 64, 76, 89, 103, 118, 134, 151, 169, 188, 208, 230, 254};

class PlayerStats {
    public:
        int life;
        int max_life;
        int default_life;
        int default_life_regen; // number of life regenerated per 10 seconds (600 frames)
        int life_regen_cooldown = 0; // 0 means infinity
        int life_regen_frame = 0;
        int level = 0;
        int experience = 0;
        bool max_level = false;
        int speed;
        int default_speed;
        int speed_increment = 1;
        int range;
        int default_range;
        int range_increment = 30;
        int attack_speed; // number of attacks per 10 seconds (600 frames)
        int default_attack_speed;
        int attack_time;
        int attack_speed_increment = 2;
        int attack_damage;
        int default_attack_damage;
        int attack_damage_increment = 2;
        int invulnerability_time = 20;
        float cone_angle;
        float cone_angle_increment = 0.1f;
        float default_cone_angle;
        bool invulnerable = false; // FOR DEVELOPPMENT ONLY
        int available_points = 0;
        std::unordered_map<char, int> allocated_points;
        std::unordered_map<char, int> maximum_points = {
            {'L', 100},
            {'l', 100},
            {'D', 1000},
            {'S', 50},
            {'s', 8},
            {'R', 20},
            {'C', 30},
            {'c', 12},
        };
        PlayerStats(int max_life = 3, int life_regen = 0, int speed = 2, int range = 150, int attack_speed = 8, int attack_damage = 8, float cone_angle = 1.2f) :
            life(max_life),
            max_life(max_life),
            default_life(max_life),
            default_life_regen(life_regen),
            life_regen_cooldown(life_regen == 0 ? 0 : 600 / life_regen),
            speed(speed),
            default_speed(speed),
            range(range),
            default_range(range),
            attack_speed(attack_speed),
            default_attack_speed(attack_speed),
            attack_time(600 / attack_speed),
            attack_damage(attack_damage),
            default_attack_damage(attack_damage),
            cone_angle(cone_angle),
            default_cone_angle(cone_angle) {}

        void gain_experience(int value) {

            if (max_level) return;

            experience += value;

            while (experience >= required_experience[level]) {
                // level up
                experience -= required_experience[level];
                level ++;
                available_points++;
                if (level == required_experience.size()) {
                    max_level = true;
                    experience = 0;
                }
            }
        }

        void run_frame() {
            // handle life regen
            if (life_regen_cooldown > 0) {
                if (life_regen_frame >= life_regen_cooldown) {
                    life = (std::min)(life + 1, max_life);
                    life_regen_frame = 1;
                }
                else {
                    life_regen_frame++;
                }
            }
        }

        bool take_damage(int damage) {
            life -= damage;
            return life <= 0;
        }

        bool can_increase(char stat) {
            return allocated_points[stat] < maximum_points[stat];
        }

        bool can_decrease(char stat) {
            return allocated_points[stat] != 0;
        }

        void update_stat(char stat, bool increase) {
            // increase / decrease points
            // cannot decrease 0
            if (!increase && !can_decrease(stat)) return;
            // cannot allocate point if no available
            if (increase && !can_increase(stat)) return;
            // cannot allocate points above max
            if (increase && allocated_points[stat] >= maximum_points[stat]) return;

            // increase stat points
            if (increase) {
                allocated_points[stat]++;
                available_points--;
            }
            // decrease stat points
            else {
                allocated_points[stat]--;
                available_points++;
            }

            // update stat
            switch (stat)
            {
                case 'L': {// max life
                    // remove or add the same amount to life and max_life (minimum 1 life)
                    int dlife = default_life + allocated_points['L'] - max_life;
                    life = (std::max)(1, life + dlife);
                    max_life += dlife;
                    break;
                }
                case 'l': // life regen
                    // one point gives one life regenerated every 10 seconds (600 frames)
                    life_regen_cooldown = allocated_points['l'] == 0 ? 0 : 600 / (default_life_regen + allocated_points['l']);
                    break;

                case 'D': // attack damage
                    attack_damage = default_attack_damage + attack_damage_increment * allocated_points['D'];
                    break;

                case 'S':// attack speed
                    attack_speed = default_attack_speed + attack_speed_increment * allocated_points['S'];
                    attack_time = 600 / attack_speed;
                    break;

                case 's': // movement speed, max 10
                    speed = default_speed + speed_increment * allocated_points['s'];
                    break;
                
                case 'R': // range
                    range = default_range + range_increment * allocated_points['R'];
                    break;
                
                case 'C':   // cone angle
                case 'c': // negative cone angle
                    cone_angle = default_cone_angle + cone_angle_increment * (allocated_points['C'] - allocated_points['c']);
                    break;
            }
        }
};
