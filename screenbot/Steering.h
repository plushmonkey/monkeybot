#ifndef STEERING_H_
#define STEERING_H_

#include "Vector.h"
#include "Player.h"
#include "api/Api.h"
#include <bitset>

class Bot;

class SteeringBehavior : public api::SteeringBehavior {
private:
    enum TargetType {
        TargetNone,
        TargetPlayer,
        TargetPosition
    };

    api::Bot* m_Bot;

    api::PlayerPtr m_TargetPlayer;
    Vec2 m_Target;
    TargetType m_TargetType;

    std::bitset<api::SteeringBehavior::BehaviorCount> m_BehaviorSet;

    double m_WeightSeek;
    double m_WeightArrive;
    double m_WeightPursuit;
    double m_WeightAvoid;

    Vec2 AvoidWalls();

public:
    SteeringBehavior(api::Bot* bot);

    Vec2 Seek(Vec2 target);
    Vec2 Arrive(Vec2 target, double deceleration = 0.75);
    Vec2 Pursuit(api::PlayerPtr target);

    bool IsBehaviorOn(api::SteeringBehavior::BehaviorType behavior) const {
        return m_BehaviorSet[behavior];
    }

    void SetBehavior(api::SteeringBehavior::BehaviorType behavior, bool on) {
        m_BehaviorSet[behavior] = on;
    }

    Vec2 CalculateWeighted();

    void Target(api::PlayerPtr target);
    void Target(Vec2 position);

    Vec2 GetTargetPosition() const;
    api::PlayerPtr GetTarget() const;
};

#endif
