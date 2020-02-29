#include "MovementManager.h"

#include <cmath>
#include <iostream>
#include <memory>

class ProportionalIntegralDerivativeController {
public:
  ProportionalIntegralDerivativeController(double p_coeff, double i_coeff, double d_coeff) :
    p_coeff_(p_coeff), i_coeff_(i_coeff), d_coeff_(d_coeff),
    p_error_(0), i_error_(0), d_error_(0),
    total_steps_(0) { }

  double Control(double cross_track_error, double dt) {
    UpdateError(cross_track_error, dt);
    total_steps_++;

    double v = p_coeff_ * p_error_ + i_coeff_ * i_error_ + d_coeff_ * d_error_;

    return std::max(-1.0, std::min(1.0, v));
  }

  void Reset() {
    p_error_ = i_error_ = d_error_ = 0;
  }

private:
  void UpdateError(double cross_track_error, double dt) {
    double prev_error = p_error_;

    p_error_ = cross_track_error;
    i_error_ += p_error_ * dt;
    d_error_ = (p_error_ - prev_error) / dt;
  }

  double p_coeff_;
  double i_coeff_;
  double d_coeff_;

  double p_error_;
  double i_error_;
  double d_error_;

  double total_steps_;
};

double sign(double v) {
  return std::signbit(v) ? -1 : 1;
}

// Prevent acos from turning x into nan because it handles 1.0 boundary poorly
double safe_acos(double x) {
  if (x < -1.0) x = -1.0;
  if (x > 1.0) x = 1.0;
  return std::acos(x);
}

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
