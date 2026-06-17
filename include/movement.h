#pragma once
#include "types.h"
#include "state_machine.h"

class Movement {
public:
    Movement();

    void update(float dt, SpiderState state, const Vec2& currentPos,
                const Vec2& cursorPos, int screenW, int screenH);

    Vec2 getPosition() const { return position; }
    Vec2 getVelocity() const { return velocity; }
    float getHeading() const { return heading; }
    void setPosition(const Vec2& pos) { position = pos; }

private:
    Vec2 seek(const Vec2& target, float maxSpeed);
    Vec2 flee(const Vec2& threat, float maxSpeed);
    Vec2 wander(float dt, float maxSpeed);
    void clampToScreen(int screenW, int screenH);

    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    float heading = 0.0f;

    float maxSpeed = 200.0f;
    float maxForce = 150.0f;
    float friction = 0.96f;

    // Wander parameters
    float wanderAngle = 0.0f;
    float wanderRadius = 30.0f;
    float wanderDistance = 60.0f;
    float wanderJitter = 1.2f;
    float wanderTimer = 0.0f;
    float wanderPauseTimer = 0.0f;
    bool wanderPaused = false;
};
