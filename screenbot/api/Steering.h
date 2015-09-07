#ifndef API_STEERING_H_
#define API_STEERING_H_

#include "Player.h"

namespace api {

class Bot;

class MovementManager {
public:
    virtual ~MovementManager() { }

    virtual void UseReverse(bool use) = 0;
    virtual bool IsUsingReverse() const = 0;

    virtual void SetEnabled(bool enable) = 0;
    virtual bool IsEnabled() const = 0;

    virtual void Update(api::Bot* bot, unsigned long dt) = 0;
};

class SteeringBehavior {
public:
    enum BehaviorType {
        BehaviorSeek,
        BehaviorArrive,
        BehaviorPursuit,
        BehaviorAvoid,

        BehaviorCount
    };

    virtual Vec2 Seek(Vec2 target) = 0;
    virtual Vec2 Arrive(Vec2 target, double deceleration = 0.75) = 0;
    virtual Vec2 Pursuit(api::PlayerPtr target) = 0;

    virtual bool IsBehaviorOn(api::SteeringBehavior::BehaviorType behavior) const = 0;
    virtual void SetBehavior(api::SteeringBehavior::BehaviorType behavior, bool on) = 0;

    virtual Vec2 CalculateWeighted() = 0;

    virtual void Target(api::PlayerPtr target) = 0;
    virtual void Target(Vec2 position) = 0;
    virtual Vec2 GetTargetPosition() const = 0;
    virtual api::PlayerPtr GetTarget() const = 0;
};

} // ns api

#endif
