#include "Steering.h"

#include "Util.h"
#include <iostream>

SteeringBehavior::SteeringBehavior(api::Bot* bot)
    : m_Bot(bot),
      m_WeightSeek(0.8),
      m_WeightArrive(1.0),
      m_WeightPursuit(1.0),
      m_WeightAvoid(2.5),
      m_TargetType(TargetNone)
{

}

Vec2 SteeringBehavior::Seek(Vec2 target) {
    double speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).MaximumSpeed / 10 / 16;

    Vec2 desired = Vec2Normalize(target - m_Bot->GetPos()) * speed;

    return desired - m_Bot->GetVelocity();
}

Vec2 SteeringBehavior::Arrive(Vec2 target, double deceleration) {
    Vec2 to_target = target - m_Bot->GetPos();
    double dist = to_target.Length();
    Vec2 desired(0, 0);

    if (dist >= 1) {
        double max_speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).MaximumSpeed / 10 / 16;
        double speed = std::min(dist / deceleration, max_speed);

        desired = to_target * speed / dist;
    }

    return desired - m_Bot->GetVelocity();
}

Vec2 SteeringBehavior::Pursuit(api::PlayerPtr target) {
    Vec2 target_pos = target->GetPosition() / 16;
    Vec2 target_vel = target->GetVelocity() / 16;
    Vec2 to_target = target_pos - m_Bot->GetPos();
    
    double rel_heading = m_Bot->GetHeading().Dot(target->GetHeading());

    if (to_target.Dot(m_Bot->GetHeading()) > 0 && rel_heading < -0.95)
        return Seek(target_pos);

    double max_speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).MaximumSpeed / 10 / 16;
    double lookAhead = to_target.Length() / (max_speed + target_vel.Length());
    return Seek(target_pos + target_vel * lookAhead);
}

Vec2 SteeringBehavior::AvoidWalls() {
    const double MaxSpeed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).MaximumSpeed / 10 / 16;
    const double SpeedPercent = (m_Bot->GetVelocity().Length() / MaxSpeed);
    const double MinLookAhead = 7.0;
    const double lookAhead = MinLookAhead + SpeedPercent * MinLookAhead;

    Vec2 ahead = m_Bot->GetPos() + Vec2Normalize(m_Bot->GetVelocity()) * lookAhead;
    //Vec2 ahead = m_Bot->GetPos() + m_Bot->GetHeading() * lookAhead;
    Vec2 collidable = Util::TraceVector(m_Bot->GetPos(), ahead, 1, m_Bot->GetLevel());

    ahead.x = std::floor(ahead.x);
    ahead.y = std::floor(ahead.y);

    if (collidable == ahead) return Vec2(0, 0);

    double dist = m_Bot->GetPos().Distance(collidable);
    Vec2 avoidance;
    if (dist != 0.0) {
        double multiplier = 1.0 + (lookAhead - dist) / lookAhead;
        avoidance = Vec2Normalize(collidable - ahead) * multiplier;
    }

    return avoidance;
}

Vec2 SteeringBehavior::CalculateWeighted() {
    Vec2 force(0, 0);
    Vec2 target = m_Target;

    if (m_TargetType == TargetNone) return force;

    if (m_TargetType == TargetPlayer)
        target = m_TargetPlayer->GetPosition() / 16;

    if (IsBehaviorOn(api::SteeringBehavior::BehaviorSeek))
        force = force + Seek(target) * m_WeightSeek;
    if (IsBehaviorOn(api::SteeringBehavior::BehaviorArrive))
        force = force + Arrive(target) * m_WeightArrive;
    if (IsBehaviorOn(api::SteeringBehavior::BehaviorPursuit) && m_TargetType == TargetPlayer)
        force = force + Pursuit(m_TargetPlayer) * m_WeightPursuit;
    if (IsBehaviorOn(api::SteeringBehavior::BehaviorAvoid))
        force = force + AvoidWalls() * m_WeightAvoid;

    return force;
}

void SteeringBehavior::Target(api::PlayerPtr target) {
    m_TargetPlayer = target;

    m_TargetType = TargetPlayer;

    if (!target)
        m_TargetType = TargetNone;
}

void SteeringBehavior::Target(Vec2 position) {
    m_Target = position;
    m_TargetPlayer = nullptr;
    m_TargetType = TargetPosition;
}

Vec2 SteeringBehavior::GetTargetPosition() const {
    if (m_TargetType == TargetPosition)
        return m_Target;
    else if (m_TargetType == TargetPlayer)
        return m_TargetPlayer->GetPosition() / 16;
    return Vec2(0, 0);
}

api::PlayerPtr SteeringBehavior::GetTarget() const {
    if (m_TargetType == TargetPlayer)
        return m_TargetPlayer;
    return nullptr;
}
