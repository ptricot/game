
#pragma once

#include <cmath>
#include <unordered_set>

class Attack {
    public: 
        int attack_frames; // how many frames for an attack
        int wait_frames; // how many frames to wait before next attack
        int range;
        int min_range;
        float cone_angle;
        bool attacking = false;
        bool waiting = false;
        float angle = 0.0f;
        int current_frame = 0;
        int xbeg = 0; // relative to player_x and player_y
        int ybeg = 0;
        int xend = 0;
        int yend = 0;
        bool boss_is_hit = false;
        std::unordered_set<int> hit_enemies_keys;

        Attack(int attack_time, int range, float cone_angle, int min_range) :
            range(range), cone_angle(cone_angle), min_range(min_range) {
                update_attack_time(attack_time);
            }

        bool available() {
            return !attacking && !waiting;
        }

        bool already_hit(int key) {
            return hit_enemies_keys.find(key) != hit_enemies_keys.end();
        }

        void hit_enemy(int key) {
            hit_enemies_keys.insert(key);
        }

        void hit_boss() {
            boss_is_hit = true;
        }

        void update_range(int new_range) {
            range = new_range;
        }

        void update_cone_angle(float new_cone_angle) {
            cone_angle = new_cone_angle;
        }

        void update_attack_time(int attack_time) {
            // the faster the attack, the less we wait
            // at attack time of 60 frames or more, 30% attacking and 70% waiting
            // at attack time of 15 frames or less, 100% attacking
            // linear between 15 and 60 frames
            int ratio = (std::max)(30, (std::min)(100, (int)(120 - 1.5f * attack_time)));
            attack_frames = ratio * attack_time / 100;
            wait_frames = (100 - ratio) * attack_time / 100;
        }

        void start(float arg_angle) {
            angle = arg_angle;
            attacking = true;
            current_frame = 0;
            boss_is_hit = false;
            hit_enemies_keys.clear();
            update();
        }

        void run_frame() {
            // Nothing to do
            if (current_frame == -1) return;

            // attacking
            if (current_frame < attack_frames) {
                update();
                current_frame++;
            }
            // done attacking, start waiting
            else if (current_frame == attack_frames) {
                attacking = false;
                waiting = true;
                current_frame++;
            }
            // waiting
            else if (current_frame < attack_frames + wait_frames) {
                current_frame++;
            }
            // done waiting
            else {
                waiting = false;
                current_frame = -1;
                return;
            }
        }

    private:
        void update() {
            int attack_percentage = current_frame * 100 / attack_frames;
            float current_angle = angle + cone_angle * (attack_percentage - 50) / 100;
            xbeg = min_range * std::sin(current_angle);
            ybeg = min_range * std::cos(current_angle);
            xend = range * std::sin(current_angle);
            yend = range * std::cos(current_angle);
        }
};
