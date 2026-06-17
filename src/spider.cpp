#include "spider.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

Spider::Spider() {
    // Initialize leg rest positions (relative to body center)
    // 8 legs arranged symmetrically: 4 on left (i=0..3), 4 on right (i=4..7)
    // Each pair spreads evenly from front to back
    for (int i = 0; i < NUM_LEGS; i++) {
        int side = (i < 4) ? -1 : 1;   // left=-1, right=+1
        int idx = i % 4;               // 0=front, 3=rear

        // Spread legs evenly from -60deg to +60deg relative to side
        float baseAngle = (side > 0) ? 0.0f : M_PI;
        // Offset from perpendicular: front legs angle forward, rear legs angle back
        float spreadAngle = ((idx - 1.5f) / 1.5f) * 0.9f;
        float angle = baseAngle + spreadAngle;

        // Longer legs reach further out
        float restDist = 35.0f + idx * 3.0f;
        legs[i].restOffset = Vec2(
            std::cos(angle) * restDist,
            std::sin(angle) * restDist
        );
        legs[i].position = Vec2(0, 0);
        legs[i].target = Vec2(0, 0);
        legs[i].phase = static_cast<float>(i) / NUM_LEGS;
        legs[i].grounded = true;
        legs[i].stepProgress = 1.0f;
    }
}

void Spider::update(float dt, const Vec2& cursorPos, int screenW, int screenH) {
    // Update state machine
    stateMachine.update(dt, movement.getPosition(), cursorPos);

    // Update movement
    movement.update(dt, stateMachine.getState(), movement.getPosition(),
                    cursorPos, screenW, screenH);

    // Update procedural legs
    updateLegs(dt);

    // Update micro animations
    updateMicroAnimations(dt);
}

void Spider::updateLegs(float dt) {
    Vec2 pos = movement.getPosition();
    float head = movement.getHeading();
    float speed = movement.getVelocity().length();

    legCycleTime += dt * (speed * 0.04f + 0.5f);

    float cosH = std::cos(head);
    float sinH = std::sin(head);

    for (int i = 0; i < NUM_LEGS; i++) {
        // Calculate world-space rest position (rotated by heading)
        Vec2 rest(
            pos.x + legs[i].restOffset.x * cosH - legs[i].restOffset.y * sinH,
            pos.y + legs[i].restOffset.x * sinH + legs[i].restOffset.y * cosH
        );

        legs[i].target = rest;

        // Distance from current foot to where it should be
        float distFromTarget = Vec2::distance(legs[i].position, rest);

        // Step threshold scales with speed — faster means longer strides
        float stepThreshold = 18.0f + speed * 0.08f;

        if (legs[i].grounded && distFromTarget > stepThreshold) {
            // Alternating gait: only step if the opposite (diagonal) leg is grounded
            // This creates a natural 4+4 alternating pattern
            int oppositeIdx = (i + 4) % NUM_LEGS;

            if (legs[oppositeIdx].grounded) {
                legs[i].grounded = false;
                legs[i].stepProgress = 0.0f;
                // Set target slightly ahead of rest position (anticipation)
                Vec2 velDir = movement.getVelocity().normalized();
                legs[i].target = rest + velDir * (speed * 0.04f);
            }
        }

        // Animate stepping
        if (!legs[i].grounded) {
            float stepSpeed = 6.0f + speed * 0.03f;
            legs[i].stepProgress += dt * stepSpeed;

            if (legs[i].stepProgress >= 1.0f) {
                legs[i].stepProgress = 1.0f;
                legs[i].grounded = true;
                legs[i].position = legs[i].target;
            } else {
                // Smooth interpolation from old pos to target
                float t = legs[i].stepProgress;
                float smoothT = t * t * (3.0f - 2.0f * t);

                Vec2 startPos = legs[i].position;
                legs[i].position = Vec2::lerp(startPos, legs[i].target, smoothT * 0.3f);

                // Vertical arc (lift foot off ground)
                float arc = std::sin(t * M_PI) * 5.0f;
                legs[i].position.y -= arc;
            }
        } else {
            // Grounded feet stay planted (don't drift)
            // Only very slowly track if body rotates in place
            if (speed < 5.0f) {
                legs[i].position = Vec2::lerp(legs[i].position, legs[i].target, dt * 1.5f);
            }
        }
    }
}

void Spider::updateMicroAnimations(float dt) {
    SpiderState state = stateMachine.getState();

    // Body pulse (breathing)
    bodyPulse += dt * 2.5f;
    if (bodyPulse > 2.0f * M_PI) bodyPulse -= 2.0f * M_PI;

    // Micro twitches - more frequent in alert states
    twitchTimer -= dt;
    if (twitchTimer <= 0.0f) {
        float twitchRate = 3.0f;
        if (state == SpiderState::ALERT || state == SpiderState::OBSERVE) {
            twitchRate = 1.0f;
        } else if (state == SpiderState::STALK) {
            twitchRate = 2.0f;
        }
        twitchTimer = twitchRate + randf() * twitchRate;
        microTwitch = (randf() - 0.5f) * 0.3f;
    } else {
        microTwitch *= 0.95f; // Decay
    }

    // Body bob
    float targetBob = 0.0f;
    if (state == SpiderState::CHARGE) {
        targetBob = std::sin(legCycleTime * 3.0f) * 2.0f;
    } else if (state == SpiderState::IDLE || state == SpiderState::STALK) {
        targetBob = std::sin(legCycleTime) * 1.0f;
    }
    bodyBob += (targetBob - bodyBob) * dt * 5.0f;
}

void Spider::setAggressiveMode(bool aggressive) {
    stateMachine.setAggressiveMode(aggressive);
}
