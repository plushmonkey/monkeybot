#include "Steering.h"

#include "Util.h"
#include "RayCaster.h"
#include "Level.h"
#include <iostream>

SteeringBehavior::SteeringBehavior(api::Bot* bot)
    : m_Bot(bot),
      m_WeightSeek(0.8),
      m_WeightArrive(1.0),
      m_WeightPursuit(1.0),
      m_WeightAvoid(3.5),
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
    const double kDegToRad = M_PI / 180.0;
    const double kMinLookAhead = 7.0;
    const double kMaxLookAhead = 25.0;
    const double kImmediateMultiplier = 1.5;

    const double max_speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).MaximumSpeed / 10.0 / 16.0;
    const double speed_percent = (m_Bot->GetVelocity().Length() / max_speed);
    const double lookAhead = kMinLookAhead + speed_percent * (kMaxLookAhead - kMinLookAhead);
    const Level& level = m_Bot->GetLevel();

    Vec2 pos = m_Bot->GetPos();
    int ship_num = (int)m_Bot->GetShip();
    const ShipSettings& ship_settings = m_Bot->GetMemorySensor().GetClientSettings().ShipSettings[ship_num];
    // Radius in tiles
    double radius = ship_settings.Radius / 16.0;

    Vec2 avoidance;

#if 0
    // Search around the ship for immediate collisions and push back against them
    int check_radius = (int)std::ceil(radius + 1);
    for (int x = -check_radius; x < check_radius; ++x) {
      for (int y = -check_radius; y < check_radius; ++y) {
        Vec2 check = pos + Vec2(x, y);

        if (level.IsSolid((unsigned short)check.x, (unsigned short)check.y)) {
          Vec2 tile_center = Vec2(x + 0.5, y + 0.5);
          Vec2 push_direction = Vec2Normalize(-tile_center);
          avoidance = avoidance + push_direction * kImmediateMultiplier;
        }
      }
    }
#endif

    Vec2 velocity = m_Bot->GetVelocity();
    Vec2 direction = Vec2Normalize(velocity);

    std::vector<Vec2> rays;

    // Create look-ahead rays in front of the bot and two rotated away
    rays.push_back(direction * lookAhead);
    rays.push_back(Vec2Rotate(rays[0], -30.0 * kDegToRad) * 0.75);
    rays.push_back(Vec2Rotate(rays[0], 30.0 * kDegToRad) * 0.75);

    for (Vec2& ray : rays) {
      double length = ray.Length();
      CastResult result = RayCast(level, pos, Vec2Normalize(ray), length);

      if (result.hit) {
        double dist = result.position.Distance(pos);

        if (dist != 0.0) {
          double multiplier = 1.0 + (lookAhead - dist) / lookAhead;

          avoidance = avoidance + Vec2Normalize(pos - result.hit) * multiplier;
          //avoidance = avoidance + result.normal * multiplier;
        }
      }
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
