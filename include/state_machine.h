#pragma once
#include "types.h"

enum class SpiderState {
    IDLE,
    ALERT,
    STALK,
    CHARGE,
    RETREAT,
    OBSERVE
};

struct StateConfig {
    float moveSpeed;
    float turnSpeed;
    float alertRadius;
    float chargeRadius;
    float retreatRadius;
    float minStateDuration;
    float maxStateDuration;
};

class StateMachine {
public:
    StateMachine();

    void update(float dt, const Vec2& spiderPos, const Vec2& cursorPos);
    SpiderState getState() const { return currentState; }
    float getStateTime() const { return stateTime; }
    const StateConfig& getConfig() const;

    void setAggressiveMode(bool aggressive) { aggressiveMode = aggressive; }
    bool isAggressive() const { return aggressiveMode; }

private:
    void transitionTo(SpiderState newState);
    SpiderState evaluateTransition(const Vec2& spiderPos, const Vec2& cursorPos);

    SpiderState currentState = SpiderState::IDLE;
    float stateTime = 0.0f;
    float transitionCooldown = 0.0f;
    bool aggressiveMode = false;

    StateConfig idleConfig;
    StateConfig alertConfig;
    StateConfig stalkConfig;
    StateConfig chargeConfig;
    StateConfig retreatConfig;
    StateConfig observeConfig;
};
