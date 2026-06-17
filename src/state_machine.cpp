#include "state_machine.h"
#include <cstdlib>
#include <cmath>

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

StateMachine::StateMachine() {
    // IDLE: slow wandering
    idleConfig = {60.0f, 2.0f, 250.0f, 100.0f, 50.0f, 2.0f, 6.0f};
    // ALERT: stopped, watching
    alertConfig = {20.0f, 5.0f, 300.0f, 150.0f, 40.0f, 0.5f, 2.0f};
    // STALK: slow approach
    stalkConfig = {80.0f, 3.0f, 350.0f, 120.0f, 60.0f, 2.0f, 5.0f};
    // CHARGE: fast rush
    chargeConfig = {280.0f, 6.0f, 400.0f, 200.0f, 80.0f, 0.3f, 1.5f};
    // RETREAT: fast escape
    retreatConfig = {220.0f, 4.0f, 200.0f, 100.0f, 30.0f, 1.0f, 3.0f};
    // OBSERVE: still, watching
    observeConfig = {10.0f, 4.0f, 280.0f, 130.0f, 50.0f, 1.5f, 4.0f};
}

void StateMachine::update(float dt, const Vec2& spiderPos, const Vec2& cursorPos) {
    stateTime += dt;
    transitionCooldown -= dt;
    if (transitionCooldown < 0.0f) transitionCooldown = 0.0f;

    if (transitionCooldown <= 0.0f) {
        SpiderState next = evaluateTransition(spiderPos, cursorPos);
        if (next != currentState) {
            transitionTo(next);
        }
    }
}

SpiderState StateMachine::evaluateTransition(const Vec2& spiderPos, const Vec2& cursorPos) {
    float dist = Vec2::distance(spiderPos, cursorPos);
    const StateConfig& cfg = getConfig();

    float alertDist = aggressiveMode ? cfg.alertRadius * 1.5f : cfg.alertRadius;
    float chargeDist = aggressiveMode ? cfg.chargeRadius * 1.3f : cfg.chargeRadius;

    switch (currentState) {
        case SpiderState::IDLE:
            if (dist < alertDist) {
                return SpiderState::ALERT;
            }
            if (stateTime > cfg.maxStateDuration) {
                return SpiderState::OBSERVE;
            }
            break;

        case SpiderState::ALERT:
            if (dist < chargeDist && aggressiveMode) {
                return SpiderState::CHARGE;
            }
            if (dist < chargeDist + 50.0f) {
                return SpiderState::STALK;
            }
            if (dist > alertDist * 1.5f) {
                return SpiderState::IDLE;
            }
            if (stateTime > cfg.maxStateDuration) {
                return SpiderState::STALK;
            }
            break;

        case SpiderState::STALK:
            if (dist < chargeDist * 0.6f) {
                if (aggressiveMode || randf() > 0.5f) {
                    return SpiderState::CHARGE;
                }
                return SpiderState::RETREAT;
            }
            if (dist > alertDist * 2.0f) {
                return SpiderState::IDLE;
            }
            if (stateTime > cfg.maxStateDuration) {
                return (randf() > 0.5f) ? SpiderState::OBSERVE : SpiderState::IDLE;
            }
            break;

        case SpiderState::CHARGE:
            if (dist < 30.0f || stateTime > cfg.maxStateDuration) {
                return SpiderState::RETREAT;
            }
            if (dist > alertDist * 2.5f) {
                return SpiderState::STALK;
            }
            break;

        case SpiderState::RETREAT:
            if (stateTime > cfg.maxStateDuration) {
                return SpiderState::OBSERVE;
            }
            if (dist > alertDist * 2.0f) {
                return SpiderState::IDLE;
            }
            break;

        case SpiderState::OBSERVE:
            if (dist < chargeDist) {
                return aggressiveMode ? SpiderState::CHARGE : SpiderState::ALERT;
            }
            if (stateTime > cfg.maxStateDuration) {
                return SpiderState::IDLE;
            }
            break;
    }

    return currentState;
}

void StateMachine::transitionTo(SpiderState newState) {
    currentState = newState;
    stateTime = 0.0f;
    transitionCooldown = 0.3f + randf() * 0.5f;
}

const StateConfig& StateMachine::getConfig() const {
    switch (currentState) {
        case SpiderState::IDLE: return idleConfig;
        case SpiderState::ALERT: return alertConfig;
        case SpiderState::STALK: return stalkConfig;
        case SpiderState::CHARGE: return chargeConfig;
        case SpiderState::RETREAT: return retreatConfig;
        case SpiderState::OBSERVE: return observeConfig;
    }
    return idleConfig;
}
