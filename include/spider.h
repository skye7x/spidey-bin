#pragma once
#include "types.h"
#include "state_machine.h"
#include "movement.h"

struct LegSegment {
    Vec2  position;
    Vec2  stepStart;    // foot position captured at the start of each step
    Vec2  target;
    Vec2  restOffset;
    float phase;
    bool  grounded;
    float stepProgress;
};

class Spider {
public:
    Spider();

    void update(float dt, const Vec2& cursorPos, int screenW, int screenH);
    void setAggressiveMode(bool aggressive);
    bool isAggressive() const { return stateMachine.isAggressive(); }

    Vec2        getPosition()   const { return movement.getPosition(); }
    float       getHeading()    const { return movement.getHeading(); }
    SpiderState getState()      const { return stateMachine.getState(); }
    const LegSegment* getLegs() const { return legs; }
    float getBodyPulse()        const { return bodyPulse; }
    float getMicroTwitch()      const { return microTwitch; }
    float getBodyBob()          const { return bodyBob; }

    void setPosition(const Vec2& pos) { movement.setPosition(pos); }

    static constexpr int NUM_LEGS = 8;

private:
    void updateLegs(float dt);
    void updateMicroAnimations(float dt);

    StateMachine stateMachine;
    Movement     movement;

    LegSegment legs[NUM_LEGS];
    float legCycleTime = 0.0f;
    float bodyPulse    = 0.0f;
    float microTwitch  = 0.0f;
    float twitchTimer  = 0.0f;
    float bodyBob      = 0.0f;
};
