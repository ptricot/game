
#pragma once

#include <functional>
#include <cmath>

class Projectile {
    public:
        float x;
        float y;
        int radius;
        int frame = 0;
        float dx;
        float dy;
        char damage_type; // P : physical, F : fire, C : cold
        std::function<float(float, int)> acceleration_x;
        std::function<float(float, int)> acceleration_y;
        Projectile(
            float x,
            float y,
            float angle,
            int radius = 10,
            float speed = 2.0f,
            char damage_type = 'F',
            std::function<float(float, int)> acceleration_x = [](float dx, int frame){return 0.0f;},
            std::function<float(float, int)> acceleration_y = [](float dx, int frame){return 0.0f;}
        ) :
            x(x),
            y(y),
            radius(radius),
            dx(speed * std::sin(angle)),
            dy(speed * std::cos(angle)),
            damage_type(damage_type),
            acceleration_x(acceleration_x),
            acceleration_y(acceleration_y) {}

    void run_frame() {
        // Apply acceleration
        dx += acceleration_x(dx, frame);
        dy += acceleration_y(dy, frame);
        
        // Apply speed
        x += dx;
        y += dy;

        frame++;
    }
};