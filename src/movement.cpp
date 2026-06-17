#include "movement.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static float randfRange(float min, float max) {
    return min + randf() * (max - min);
}

Movement::Movement() {
    position = Vec2(400.0f, 300.0f);
    velocity = Vec2(0.0f, 0.0f);
    acceleration = Vec2(0.0f, 0.0f);
}

void Movement::update(float dt, SpiderState state, const Vec2& currentPos,
                      const Vec2& cursorPos, int screenW, int screenH) {
    position = currentPos;
    Vec2 steer(0.0f, 0.0f);

    float speed = 0.0f;

    switch (state) {
        case SpiderState::IDLE:
            speed = 40.0f;
            steer = wander(dt, speed);
            break;

        case SpiderState::ALERT:
            speed = 15.0f;
            steer = seek(cursorPos, speed) * 0.15f;
            break;

        case SpiderState::STALK:
            speed = 55.0f;
            steer = seek(cursorPos, speed) * 0.6f + wander(dt, speed) * 0.2f;
            break;

        case SpiderState::CHARGE:
            speed = 200.0f;
            steer = seek(cursorPos, speed);
            break;

        case SpiderState::RETREAT: {
            speed = 160.0f;
            steer = flee(cursorPos, speed);
            break;
        }

        case SpiderState::OBSERVE:
            speed = 8.0f;
            steer = wander(dt, speed) * 0.1f;
            break;
    }

    maxSpeed = speed;

    // Apply steering force
    acceleration = steer;

    // Limit acceleration
    float accelLen = acceleration.length();
    if (accelLen > maxForce) {
        acceleration = acceleration.normalized() * maxForce;
    }

    // Integrate
    velocity += acceleration * dt;

    // Apply friction
    velocity *= friction;

    // Limit velocity
    float velLen = velocity.length();
    if (velLen > maxSpeed) {
        velocity = velocity.normalized() * maxSpeed;
    }

    // Update position
    position += velocity * dt;

    // Clamp to screen
    clampToScreen(screenW, screenH);

    // Update heading smoothly
    if (velLen > 3.0f) {
        float targetHeading = std::atan2(velocity.y, velocity.x);
        float diff = targetHeading - heading;
        while (diff > M_PI) diff -= 2.0f * M_PI;
        while (diff < -M_PI) diff += 2.0f * M_PI;
        heading += diff * std::min(1.0f, 4.0f * dt);
    }
}

Vec2 Movement::seek(const Vec2& target, float speed) {
    Vec2 toTarget = target - position;
    float dist = toTarget.length();
    if (dist < 1.0f) return Vec2(0, 0) - velocity * 0.5f;

    // Arrive behavior — slow down near target
    float desiredSpeed = speed;
    if (dist < 100.0f) {
        desiredSpeed = speed * (dist / 100.0f);
    }

    Vec2 desired = toTarget.normalized() * desiredSpeed;
    return desired - velocity;
}

Vec2 Movement::flee(const Vec2& threat, float speed) {
    Vec2 desired = (position - threat).normalized() * speed;
    return desired - velocity;
}

Vec2 Movement::wander(float dt, float speed) {
    wanderTimer += dt;

    // Periodically pause during wander (check once per ~frame)
    if (!wanderPaused && wanderTimer > 0.5f && randf() < 0.02f) {
        wanderPaused = true;
        wanderPauseTimer = randfRange(1.0f, 3.5f);
        wanderTimer = 0.0f;
    }

    if (wanderPaused) {
        wanderPauseTimer -= dt;
        if (wanderPauseTimer <= 0.0f) {
            wanderPaused = false;
            wanderTimer = 0.0f;
        }
        // Brake gently
        return Vec2(0, 0) - velocity * 0.3f;
    }

    // Gentle wander angle drift
    wanderAngle += randfRange(-wanderJitter, wanderJitter) * dt;

    Vec2 circleCenter = velocity.normalized() * wanderDistance;
    if (velocity.length() < 2.0f) {
        circleCenter = Vec2(std::cos(heading), std::sin(heading)) * wanderDistance;
    }

    Vec2 displacement(
        std::cos(wanderAngle) * wanderRadius,
        std::sin(wanderAngle) * wanderRadius
    );

    Vec2 wanderForce = circleCenter + displacement;

    float len = wanderForce.length();
    if (len > speed) {
        wanderForce = wanderForce.normalized() * speed;
    }

    return wanderForce;
}

void Movement::clampToScreen(int screenW, int screenH) {
    float margin = 50.0f;
    float softMargin = 120.0f;

    // Soft boundary — steer away from edges
    if (position.x < softMargin) {
        velocity.x += (softMargin - position.x) * 0.05f;
    }
    if (position.x > screenW - softMargin) {
        velocity.x -= (position.x - (screenW - softMargin)) * 0.05f;
    }
    if (position.y < softMargin) {
        velocity.y += (softMargin - position.y) * 0.05f;
    }
    if (position.y > screenH - softMargin) {
        velocity.y -= (position.y - (screenH - softMargin)) * 0.05f;
    }

    // Hard clamp
    if (position.x < margin) {
        position.x = margin;
        velocity.x = std::abs(velocity.x) * 0.3f;
    }
    if (position.x > screenW - margin) {
        position.x = screenW - margin;
        velocity.x = -std::abs(velocity.x) * 0.3f;
    }
    if (position.y < margin) {
        position.y = margin;
        velocity.y = std::abs(velocity.y) * 0.3f;
    }
    if (position.y > screenH - margin) {
        position.y = screenH - margin;
        velocity.y = -std::abs(velocity.y) * 0.3f;
    }
}
