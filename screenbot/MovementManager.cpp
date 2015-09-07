#include "MovementManager.h"

MovementManager::MovementManager(ClientPtr client, api::SteeringBehavior* steering)
    : m_Client(client), m_Steering(steering), m_Enabled(true), m_Reverse(false)
{

}

void MovementManager::UseReverse(bool use) {
    m_Reverse = use;
}
bool MovementManager::IsUsingReverse() const {
    return m_Reverse;
}

void MovementManager::SetEnabled(bool enable) {
    m_Enabled = enable;
}

bool MovementManager::IsEnabled() const {
    return m_Enabled;
}

void MovementManager::Update(api::Bot* bot, unsigned long dt) {
    if (!m_Enabled) return;

    Vec2 target = m_Steering->GetTargetPosition();
    Vec2 to_target = target - bot->GetPos();
    Vec2 heading = bot->GetHeading();

    Vec2 seek_vector = m_Steering->CalculateWeighted();
    if (seek_vector != Vec2(0, 0)) {
        Vec2 seek_normal = Vec2Normalize(seek_vector);
        Vec2 perp = heading.Perpendicular();
        bool clockwise = perp.Dot(seek_normal) >= 0.0;
        double dot = heading.Dot(seek_normal);

        if (seek_normal.Dot(Vec2Normalize(to_target)) < 0.0 && !m_Reverse) {
            clockwise = !clockwise;
            m_Client->SetThrust(true);
        } else {
            m_Client->SetThrust(false);
        }

        if (dot < 0.05) {
            m_Client->Up(false);
            m_Client->Down(true);
        } else if (dot > 0.05) {
            m_Client->Up(true);
            m_Client->Down(false);
        } else {
            m_Client->Up(false);
            m_Client->Down(false);
        }

        if (dot < 0.95) {
            m_Client->Right(clockwise);
            m_Client->Left(!clockwise);
        } else {
            m_Client->Left(false);
            m_Client->Right(false);
        }
    } else {
        m_Client->Left(false);
        m_Client->Right(false);
        m_Client->Up(false);
        m_Client->Down(false);
    }
}
