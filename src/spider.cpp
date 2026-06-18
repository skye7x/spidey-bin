#include "spider.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

Spider::Spider() {
    for (int i = 0; i < NUM_LEGS; i++) {
        int side = (i < 4) ? -1 : 1;
        int idx  = i % 4;

        float baseAngle   = (side > 0) ? 0.0f : M_PI;
        float spreadAngle = ((idx - 1.5f) / 1.5f) * 0.9f;
        float angle       = baseAngle + spreadAngle;

        float restDist = 35.0f + idx * 3.0f;
        legs[i].restOffset = Vec2(
            std::cos(angle) * restDist,
            std::sin(angle) * restDist
        );
        legs[i].position     = Vec2(0, 0);
        legs[i].stepStart    = Vec2(0, 0);   // initialise new field
        legs[i].target       = Vec2(0, 0);
        legs[i].phase        = static_cast<float>(i) / NUM_LEGS;
        legs[i].grounded     = true;
        legs[i].stepProgress = 1.0f;
    }
}

void Spider::update(float dt, const Vec2& cursorPos, int screenW, int screenH) {
    stateMachine.update(dt, movement.getPosition(), cursorPos);

    // Bug fix: no longer passes movement.getPosition() back as currentPos —
    // Movement::update now integrates its own position directly.
    movement.update(dt, stateMachine.getState(), cursorPos, screenW, screenH);

    updateLegs(dt);
    updateMicroAnimations(dt);
}

void Spider::updateLegs(float dt) {
    Vec2  pos   = movement.getPosition();
    float head  = movement.getHeading();
    float speed = movement.getVelocity().length();

    legCycleTime += dt * (speed * 0.04f + 0.5f);

    float cosH = std::cos(head);
    float sinH = std::sin(head);

    for (int i = 0; i < NUM_LEGS; i++) {
        Vec2 rest(
            pos.x + legs[i].restOffset.x * cosH - legs[i].restOffset.y * sinH,
            pos.y + legs[i].restOffset.x * sinH + legs[i].restOffset.y * cosH
        );

        legs[i].target = rest;

        float distFromTarget = Vec2::distance(legs[i].position, rest);
        float stepThreshold  = 18.0f + speed * 0.08f;

        if (legs[i].grounded && distFromTarget > stepThreshold) {
            int oppositeIdx = (i + 4) % NUM_LEGS;
            if (legs[oppositeIdx].grounded) {
                legs[i].grounded     = false;
                legs[i].stepProgress = 0.0f;
                // Bug fix: capture the foot's position at the moment the step
                // begins so the lerp always interpolates from the correct
                // origin, preventing the snap/pop at landing.
                legs[i].stepStart = legs[i].position;

                Vec2 velDir = movement.getVelocity().normalized();
                legs[i].target = rest + velDir * (speed * 0.04f);
            }
        }

        if (!legs[i].grounded) {
            float stepSpeed = 6.0f + speed * 0.03f;
            legs[i].stepProgress += dt * stepSpeed;

            if (legs[i].stepProgress >= 1.0f) {
                legs[i].stepProgress = 1.0f;
                legs[i].grounded     = true;
                legs[i].position     = legs[i].target;
            } else {
                float t       = legs[i].stepProgress;
                float smoothT = t * t * (3.0f - 2.0f * t);

                // Bug fix: lerp from stepStart (fixed at step-begin), not from
                // the current position each frame. Old code re-read position as
                // startPos every tick so it only ever moved 30% of the remaining
                // gap, never converging — causing a visible snap on landing.
                legs[i].position = Vec2::lerp(legs[i].stepStart,
                                              legs[i].target, smoothT);

                // Vertical arc (lift foot off ground)
                float arc = std::sin(t * M_PI) * 5.0f;
                legs[i].position.y -= arc;
            }
        } else {
            if (speed < 5.0f) {
                legs[i].position = Vec2::lerp(legs[i].position, legs[i].target,
                                              dt * 1.5f);
            }
        }
    }
}

void Spider::updateMicroAnimations(float dt) {
    SpiderState state = stateMachine.getState();

    bodyPulse += dt * 2.5f;
    if (bodyPulse > 2.0f * M_PI) bodyPulse -= 2.0f * M_PI;

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
        microTwitch *= 0.95f;
    }

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
