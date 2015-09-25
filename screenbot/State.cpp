#include "State.h"

#include "Keyboard.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Bot.h"
#include "Client.h"
#include "Memory.h"
#include "Random.h"
#include "Tokenizer.h"
#include "MemorySensor.h"

#include <thread>
#include <iostream>
#include <tchar.h>
#include <regex>
#include <cmath>
#include <limits>

#define RADIUS 1

namespace {

const int PathUpdateDelay = 250;

} // ns

std::ostream& operator<<(std::ostream& out, api::StateType type) {
    static std::vector<std::string> states = { "Chase", "Patrol", "Aggressive", "Attach", "Baseduel", "Follow", "Plugin" };
    out << states.at(static_cast<int>(type));
    return out;
}

AttachState::AttachState(api::Bot* bot)
    : State(bot),
      m_Direction(Direction::Down),
      m_Count(0)
{
    bot->GetClient()->ReleaseKeys();
}

void AttachState::Update(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();
    Vec2 pos = m_Bot->GetPos();

    if (client->OnSoloFreq()) {
        m_Bot->SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    std::string bot_name = m_Bot->GetName();
    int bot_freq = client->GetFreq();
    auto selected = client->GetSelectedPlayer();
    std::string attach_target = m_Bot->GetAttachTarget();

    if (!m_Bot->IsInSafe()) {
        // Detach if previous attach was successful
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (attach_target.length() > 0)
            client->SelectPlayer(attach_target);
        client->Attach();
        m_Bot->SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    // Check if there is a specific person to attach to
    if (attach_target.length() == 0) {
        if (selected->GetName().compare(bot_name.substr(0, 12)) == 0) {
            // Ticker is at the top of the list, reverse direction
            m_Direction = Direction::Down;
            client->MoveTicker(m_Direction);
            return;
        }

        if (selected->GetFreq() != bot_freq) {
            // Ticker is below bot freq, reverse direction
            m_Direction = Direction::Up;
            client->MoveTicker(m_Direction);
            return;
        }
    } else {
        // Moves the ticker without continuing state loop
        client->SelectPlayer(attach_target);
        m_Direction = Direction::None;
    }

    client->SetXRadar(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (m_Bot->GetEnergyPercent() == 100) {
        client->Attach();

        double multiplier = 1.0 + (m_Count++ / 2.0);

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(std::ceil(200 * multiplier))));
    }

    client->MoveTicker(m_Direction);
}

Vec2 ClosestWall(const Level& level, Pathing::JPSNode* node) {
    static const std::vector<Vec2> directions = { Vec2(0, -1), Vec2(1, 0), Vec2(0, 1), Vec2(-1, 0) };
    const int SearchLength = 5;
    Vec2 closest;
    double closest_dist = std::numeric_limits<double>::max();

    Vec2 node_pos(node->x, node->y);

    for (const auto& dir : directions) {
        for (int i = 0; i < SearchLength; ++i) {
            Vec2 current = node_pos + dir * i;
            if (level.IsSolid((short)current.x, (short)current.y)) {
                double dist = node_pos.Distance(current);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    closest = current;
                }
                break;
            }
        }
    }

    return closest;
};

ChaseState::ChaseState(api::Bot* bot)
    : PathingState(bot), 
      m_LastRealEnemyCoord(0, 0),
      m_LastEnemySeen(0)
{
    m_Bot->GetClient()->ReleaseKeys();

    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorPursuit, false);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorSeek, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorArrive, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorAvoid, true);

    m_Bot->GetMovementManager()->SetEnabled(true);
}

ChaseState::~ChaseState() {
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorPursuit, false);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorSeek, false);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorArrive, false);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorAvoid, false);
}

void ChaseState::Update(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();

    if (!m_Bot->GetEnemyTarget().get()) {
        client->ReleaseKeys();
        m_Bot->SetState(std::make_shared<PatrolState>(m_Bot));
        return;
    }

    api::SteeringBehavior& steering = m_Bot->GetSteering();
    Vec2 pos = m_Bot->GetPos();
    Vec2 enemy_pos = steering.GetTargetPosition();

    if (Util::IsClearPath(pos, enemy_pos, RADIUS, m_Bot->GetLevel())) {
        m_Bot->SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    UpdateStuckCheck(dt);

    UpdatePath(enemy_pos, dt);

    if (m_Plan.size() == 0) {
        m_LastEnemySeen += dt;

        if (m_LastEnemySeen >= 45 * 1000) {
            m_LastEnemySeen = 0;
            client->ReleaseKeys();
            client->Warp();
            return;
        }

        client->ReleaseKeys();
        return;
    } else {
        m_LastEnemySeen = 0;
    }

    Vec2 next = m_Plan.at(0);
    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    // Grab the next node in the plan that isn't right beside the bot. NOTE: This can mess things up so the bot gets stuck on corners.
    while (((dist <= 1 && m_Bot->GetGrid().GetNode((short)next.x, (short)next.y)->near_wall) || (dist < 2 && !m_Bot->GetGrid().GetNode((short)next.x, (short)next.y)->near_wall)) && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next = m_Plan.at(0);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    steering.Target(next);

    UpdateWeapons(dt);
}

void ChaseState::UpdateWeapons(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();

    if (m_Bot->IsInSafe()) {
        client->Gun(GunState::Off);
        return;
    }
    
    double total_dist = GetPlanDistance();
    bool bouncing_bullets = client->HasBouncingBullets();
    bool fire = (m_Bot->GetConfig().MinGunRange == 0 || total_dist <= m_Bot->GetConfig().MinGunRange * .8) && bouncing_bullets;

    if (fire && !m_Bot->IsInSafe())
        client->Gun(GunState::Tap, m_Bot->GetEnergyPercent());
    else
        client->Gun(GunState::Off);
}

BaseduelState::BaseduelState(api::Bot* bot)
    : State(bot) 
{

}

void BaseduelState::Update(DWORD dt) {
    // Find possible waypoints within a certain radius
    const int SearchRadius = 200;
    const int CenterRadius = 60;
    const int SafeRadius = 6;
    const int SpawnX = m_Bot->GetConfig().SpawnX;
    const int SpawnY = m_Bot->GetConfig().SpawnY;

    Vec2 pos = m_Bot->GetPos();

    std::vector<Vec2> waypoints;

    // Only add safe tiles that aren't near other waypoints
    for (int y = int(pos.y - SearchRadius); y < int(pos.y + SearchRadius) && y < 1024; ++y) {
        for (int x = int(pos.x - SearchRadius); x < int(pos.x + SearchRadius) && x < 1024; ++x) {
            int id = m_Bot->GetLevel().GetTileID(x, y);

            if (id != 171) continue;
            if (!m_Bot->GetGrid().IsOpen(x, y)) continue;
            if (x > SpawnX - CenterRadius && x < SpawnX + CenterRadius && y > SpawnY - CenterRadius && y < SpawnY + CenterRadius)
                continue;

            {
                int dx = static_cast<int>(pos.x - x);
                int dy = static_cast<int>(pos.y - y);
                double dist = std::sqrt(dx * dx + dy * dy);

                if (dist < SafeRadius) continue;
            }

            bool add = true;
            for (Vec2& waypoint : waypoints) {
                int dx = static_cast<int>(waypoint.x - x);
                int dy = static_cast<int>(waypoint.y - y);
                double dist = std::sqrt(dx * dx + dy * dy);

                if (dist <= SafeRadius) {
                    add = false;
                    break;
                }
            }

            if (add)
                waypoints.emplace_back((float)x, (float)y);
        }
    }

    // Try to find a path to each of the possible waypoints
    for (Vec2& waypoint : waypoints) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        auto plan = jps((short)waypoint.x, (short)waypoint.y, (short)pos.x, (short)pos.y, m_Bot->GetGrid());

        if (plan.size() != 0) { // A possible waypoint found
            std::vector<Vec2> patrol_waypoint;
            patrol_waypoint.push_back(waypoint);
            m_Bot->SetState(std::make_shared<PatrolState>(m_Bot, patrol_waypoint));
            break;
        }
    }
}

FollowState::FollowState(api::Bot* bot, std::string follow)
    : PathingState(bot)
{
   api::PlayerList players = m_Bot->GetMemorySensor().GetPlayers();

    follow = Util::strtolower(follow);

   api::PlayerList::iterator it = std::find_if(players.begin(), players.end(), [&](api::PlayerPtr player) {
        const std::string& name = Util::strtolower(player->GetName());
        return name.compare(follow) == 0;
    });

    if (it == players.end()) {
        std::cerr << "Could not find player to follow. Switching to PatrolState." << std::endl;
        m_Bot->Follow("");
        return;
    }

    m_FollowPlayer = std::shared_ptr<Player>((Player*)it->get());
}

void FollowState::Update(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();
    api::PlayerPtr player = m_FollowPlayer.lock();

    if (!player) {
        std::cerr << "Could not find player to follow. Switching to PatrolState." << std::endl;
        m_Bot->Follow("");
        return;
    }

    Vec2 target = player->GetPosition() / 16;
    Vec2 pos = m_Bot->GetPos();
    bool near_wall = Util::NearWall(pos, m_Bot->GetGrid());

    this->UpdatePath(target, dt);

    if (m_Plan.size() == 0)
        m_Plan.emplace_back(target.x, target.y);

    Vec2 next = m_Plan.at(0);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    while (dist < 3 && m_Plan.size() > 1 && !near_wall) {
        m_Plan.erase(m_Plan.begin());
        next = m_Plan.at(0);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorSeek, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorArrive, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorAvoid, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorPursuit, false);

    m_Bot->GetSteering().Target(next);
}

void PathingState::SmoothPath(const Level& level, const Pathing::Plan& plan, std::vector<Vec2>& result) {
    const double Intensity = 2.0;

    // todo: use IsClearPath to remove unneeded nodes

    result.resize(plan.size());
    for (std::size_t i = 0; i < plan.size(); ++i) {
        Pathing::JPSNode* node = plan[i];

        //Vec2 closest = ClosestWall(level, node);
        Vec2 current(node->x, node->y);
        Vec2 new_pos(plan[i]->x, plan[i]->y);

        /*if (closest != Vec2(0, 0))
        new_pos = current + Vec2Normalize(current - closest) * Intensity;

        if (current != new_pos) {
        if (!Util::IsClearPath(current, new_pos, 1, level))
        new_pos = current;
        }*/

        result[i] = new_pos;
    }
}

// Returns the distance of an entire plan
double PathingState::GetPlanDistance() {
    if (m_Plan.size() < 2) return 0.0;

    Vec2 last = m_Plan.at(0);
    double total_dist = 0.0;

    for (const Vec2& coord : m_Plan) {
        double dist = 0.0;

        Util::GetDistance(last, coord, nullptr, nullptr, &dist);

        total_dist += dist;
        last = coord;
    }
    return total_dist + last.Distance(m_Bot->GetPos());
}

void PathingState::UpdateStuckCheck(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();
    Vec2 pos = m_Bot->GetPos();
    bool near_wall = Util::NearWall(pos, m_Bot->GetGrid());

    m_StuckTimer += dt;

    // Check if stuck every 2.5 seconds
    if (m_StuckTimer >= 2500) {
        if (m_Bot->GetSpeed() < 1.0 && near_wall) {
            // Stuck
            client->Up(false);
            client->Down(true);

            int dir = Random::GetU32(0, 1) ? VK_LEFT : VK_RIGHT;

            dir == VK_LEFT ? client->Left(true) : client->Right(true);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            dir == VK_LEFT ? client->Left(false) : client->Right(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            dir == VK_LEFT ? client->Left(true) : client->Right(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            client->Down(false);
            dir == VK_LEFT ? client->Left(false) : client->Right(false);
        }

        m_StuckTimer = 0;
    }
}

void PathingState::UpdatePath(Vec2 target, DWORD dt) {
    Vec2 pos = m_Bot->GetPos();

    if (!m_Bot->GetGrid().IsSolid((short)pos.x, (short)pos.y) && m_Bot->GetGrid().IsOpen((short)target.x, (short)target.y)) {
        static DWORD path_timer = 0;

        path_timer += dt;

        if (path_timer >= PathUpdateDelay || m_Plan.size() == 0) {
            Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

            SmoothPath(m_Bot->GetLevel(), jps((short)target.x, (short)target.y, (short)pos.x, (short)pos.y, m_Bot->GetGrid()), m_Plan);

            path_timer = 0;
        }
    }
}

PathingState::PathingState(api::Bot* bot) 
    : State(bot)
{

}

PatrolState::PatrolState(api::Bot* bot, std::vector<Vec2> waypoints, unsigned int close_distance)
    : PathingState(bot), 
      m_LastBullet(timeGetTime()),
      m_CloseDistance(close_distance)
{
    m_Bot->GetClient()->ReleaseKeys();

    m_FullWaypoints = m_Bot->GetConfig().Waypoints;

    if (waypoints.size() > 0)
        m_Waypoints = waypoints;
    else
        ResetWaypoints(false);
}

PatrolState::~PatrolState() {
    
}

void PatrolState::ResetWaypoints(bool full) {
    m_Waypoints = m_FullWaypoints;

    if (full) return;

    double closestdist = std::numeric_limits<double>::max();
    std::vector<Vec2>::iterator closest = m_Waypoints.begin();
    Vec2 pos = m_Bot->GetPos();

    // Find the closet waypoint that isn't the one just visited
    for (auto i = m_Waypoints.begin(); i != m_Waypoints.end(); ++i) {
        int dx, dy;
        double dist;

        Util::GetDistance(pos, *i, &dx, &dy, &dist);

        if (dist > m_CloseDistance && dist < closestdist) {
            closestdist = dist;
            closest = i;
        }
    }

    // Remove all the waypoints before the closest
    if (closest != m_Waypoints.begin())
        m_Waypoints.erase(m_Waypoints.begin(), closest);
}

bool PatrolState::UpdateWaypoints() {
    if (m_Waypoints.size() == 0) {
        ResetWaypoints();
        return true;
    }

    Vec2 target = m_Waypoints.front();
    Vec2 pos = m_Bot->GetPos();

    double closedist;
    Util::GetDistance(pos, target, nullptr, nullptr, &closedist);

    if (closedist <= m_CloseDistance) {
        m_Waypoints.erase(m_Waypoints.begin());
        if (m_Waypoints.size() == 0) return true;
        target = m_Waypoints.front();
    }

    if (!m_Bot->GetGrid().IsOpen((short)target.x, (short)target.y)) {
        m_Waypoints.erase(m_Waypoints.begin());
        return true;
    }

    return false;
}

void PatrolState::Update(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();

    if (!m_Bot->GetConfig().Patrol || m_Bot->GetEnemyTarget().get()) {
        // Target found, switch to aggressive
        m_Bot->SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    m_Bot->GetMovementManager()->SetEnabled(true);

    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorSeek, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorArrive, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorAvoid, true);
    m_Bot->GetSteering().SetBehavior(api::SteeringBehavior::BehaviorPursuit, false);

    int energypct = m_Bot->GetEnergyPercent();
    client->SetXRadar(energypct > m_Bot->GetConfig().XPercent);

    if (timeGetTime() >= m_LastBullet + 60000) {
        client->Gun(GunState::Tap);
        client->Gun(GunState::Off);
        m_LastBullet = timeGetTime();
    }

    UpdateStuckCheck(dt);

    if (UpdateWaypoints())
        return;

    UpdatePath(m_Waypoints.at(0), dt);

    if (m_Plan.size() == 0) {
        m_Waypoints.erase(m_Waypoints.begin());

        if (m_Bot->GetConfig().Baseduel && !m_Bot->IsInCenter()) {
            m_Bot->SetState(std::make_shared<BaseduelState>(m_Bot));
        } else {
            static DWORD warp_timer = 0;
            warp_timer += dt;
            if (warp_timer >= 60000) {
                // Warp if inactive for 60 seconds
                if (m_Bot->GetConfig().Attach && !m_Bot->GetClient()->OnSoloFreq()) {
                    client->SetXRadar(false);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    client->Attach();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    client->Attach();
                } else {
                    ResetWaypoints();
                    client->Warp();
                }
                warp_timer = 0;
            }
        }
        client->Up(false);
        return;
    }

    Vec2 next = m_Plan.at(0);

    int dx, dy;
    double dist = 0.0;
    Vec2 pos = m_Bot->GetPos();

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    while (dist < 3 && m_Plan.size() > 1 && !Util::NearWall(pos, m_Bot->GetGrid())) {
        m_Plan.erase(m_Plan.begin());
        next = m_Plan.at(0);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    m_Bot->GetSteering().Target(next);
}

bool PointingAtWall(int rot, unsigned x, unsigned y, const Level& level) {
    bool solid = false;

    auto e = [&]() -> bool { return level.IsSolid(x + 1, y + 0); };
    auto se = [&]() -> bool { return level.IsSolid(x + 1, y + 1); };
    auto s = [&]() -> bool { return level.IsSolid(x + 0, y + 1); };
    auto sw = [&]() -> bool { return level.IsSolid(x - 1, y + 1); };
    auto w = [&]() -> bool { return level.IsSolid(x - 1, y + 0); };
    auto nw = [&]() -> bool { return level.IsSolid(x - 1, y - 1); };
    auto ne = [&]() -> bool { return level.IsSolid(x + 1, y - 1); };
    auto n = [&]() -> bool { return level.IsSolid(x + 0, y - 1); };

    if (rot >= 7 && rot <= 12) {
        // East
        solid = ne() || e() || se();
    } else if (rot >= 13 && rot <= 17) {
        // South-east
        solid = e() || se() || s();
    } else if (rot >= 18 && rot <= 22) {
        // South
        solid = se() || s() || sw();
    } else if (rot >= 23 && rot <= 27) {
        // South-west
        solid = s() || sw() || w();
    } else if (rot >= 28 && rot <= 32) {
        // West
        solid = sw() || w() || nw();
    } else if (rot >= 33 && rot <= 37) {
        // North-west
        solid = w() || nw() || n();
    } else if (rot >= 3 && rot <= 6) {
        // North-east
        solid = n() || ne() || e();
    } else {
        // North
        solid = nw() || n() || nw();
    }
    return solid;
}

AggressiveState::AggressiveState(api::Bot* bot)
    : State(bot), 
      m_LastEnemyPos(0,0),
      m_NearWall(0),
      m_BurstTimer(100000),
      m_RocketTimer(10000),
      m_LastSafeTime(0)
{
	m_DecoyTimer = 0;

    m_Bot->GetClient()->ReleaseKeys();

    SteeringBehavior& steering = ((Bot*)m_Bot)->GetSteering();

    steering.SetBehavior(SteeringBehavior::BehaviorPursuit, true);
    steering.SetBehavior(SteeringBehavior::BehaviorArrive, false);
    steering.SetBehavior(SteeringBehavior::BehaviorSeek, false);
}

AggressiveState::~AggressiveState() {
    m_Bot->GetMovementManager()->SetEnabled(true);
}

Vec2 CalculateShot(const Vec2& pShooter, const Vec2& pTarget, const Vec2& vShooter, const Vec2& vTarget, double sProjectile) {
    Vec2 totarget = pTarget - pShooter;
    Vec2 v = vTarget - vShooter;

    double a = v.Dot(v) - sProjectile * sProjectile;
    double b = 2 * v.Dot(totarget);
    double c = totarget.Dot(totarget);

    Vec2 solution;

    double disc = (b * b) - 4 * a * c;
    double t = -1.0;

    if (disc >= 0.0) {
        double t1 = (-b + std::sqrt(disc)) / (2 * a);
        double t2 = (-b - std::sqrt(disc)) / (2 * a);
        if (t1 < t2 && t1 >= 0)
            t = t1;
        else
            t = t2;
    }

    // Only use the calculated shot if its collision is within acceptable timeframe
    if (t < 0 || t > 5) {
        Vec2 hShooter = Vec2Normalize(vShooter);
        Vec2 hTarget = Vec2Normalize(vTarget);

        int sign = hShooter.Dot(hTarget) < 0.0 ? -1 : 1;
        
        double speed = vShooter.Length() + (sign * vTarget.Length()) + sProjectile;
        double look_ahead = totarget.Length() / speed;
        return Vec2(pTarget + vTarget * look_ahead);
    }

    solution = pTarget + (v * t);

    return solution;
}

// Checks for surrounding walls. Returns true if the bot should use a burst.
bool InBurstArea(const Vec2& pBot, Pathing::Grid<short>& grid) {
    static const std::vector<Vec2> directions = { Vec2(0, -1), Vec2(1, 0), Vec2(0, 1), Vec2(-1, 0) };
    const int SearchLength = 15;
    const int WallsNeeded = 2;
    int wall_count = 0;

    for (auto dir : directions) {
        for (int i = 0; i < SearchLength; ++i) {
            if (grid.IsSolid(short(pBot.x + dir.x * i), short(pBot.y + dir.y * i))) {
                wall_count++;
                break;
            }
        }
    }

    return wall_count >= WallsNeeded;
}

void AggressiveState::Update(DWORD dt) {
    ClientPtr client = m_Bot->GetClient();

    if (!m_Bot->GetEnemyTarget() || m_Bot->GetEnemyTarget()->GetPosition() == Vec2(0, 0)) {
        m_Bot->SetState(std::make_shared<PatrolState>(m_Bot));
        return;
    }

    SteeringBehavior& steering = ((Bot*)m_Bot)->GetSteering();
    Vec2 target = m_Bot->GetEnemyTarget()->GetPosition() / 16;
    Vec2 to_target = target - m_Bot->GetPos();
    Vec2 heading = m_Bot->GetHeading();

    if (!Util::IsClearPath(m_Bot->GetPos(), target, RADIUS, m_Bot->GetLevel())) {
        m_Bot->SetState(std::make_shared<ChaseState>(m_Bot));
        return;
    }

    /*Vec2 enemy_heading = m_Bot->GetEnemyTarget()->GetHeading();

    double heading_diff = heading.Dot(enemy_heading);

    if (1.0 - heading_diff < 0.15) {
        Vec2 from_target = m_Bot->GetPos() - target;
        Vec2 norm = Vec2Normalize(from_target);
        bool use_new = true;
        Vec2 new_pos = target + Vec2Rotate(norm, M_PI / 180 * 25) * 10;
        if (!Util::IsClearPath(m_Bot->GetPos(), new_pos, 1, m_Bot->GetLevel())) {
            use_new = false;
            new_pos = target + Vec2Rotate(norm, M_PI / 180 * -25) * 10;
            if (Util::IsClearPath(m_Bot->GetPos(), new_pos, 1, m_Bot->GetLevel())) 
                use_new = true;
        }

        if (use_new)
            steering.Target(new_pos);
        
    } else {
        steering.Target(m_Bot->GetEnemyTarget());
    }
    */
    
    Vec2 pos = m_Bot->GetPos();

    bool insafe = client->IsInSafe(pos, m_Bot->GetLevel());
    int tardist = m_Bot->GetConfig().TargetDistance;
    int energypct = m_Bot->GetEnergyPercent();

    if ((client->Emped() || energypct < m_Bot->GetConfig().RunPercent) && !insafe) {
        tardist = m_Bot->GetConfig().RunDistance;
        client->ReleaseKeys();
    }

    bool pursuit = to_target.Length() > tardist;
    steering.SetBehavior(SteeringBehavior::BehaviorPursuit, pursuit);
    m_Bot->GetMovementManager()->SetEnabled(pursuit);
    steering.Target(m_Bot->GetEnemyTarget());

    int dx, dy;
    double dist = to_target.Length();
    int target_rot;
    Vec2 target_heading;
    int rot = m_Bot->GetClient()->GetRotation();
    DWORD cur_time = timeGetTime();
    Vec2 vBot;

    const int ChaseFromSafeTime = 3000;
    if (insafe)
        m_LastSafeTime = timeGetTime();

    /* Turn off x if energy low, turn back on when high */
    client->SetXRadar(energypct > m_Bot->GetConfig().XPercent);

    /* Wait in safe for energy */
    if (insafe && energypct < 50) {
        client->Gun(GunState::Tap);
        client->ReleaseKeys();
        return;
    }

    Util::GetDistance(pos, target, &dx, &dy, &dist);

    if (!Util::IsClearPath(pos, target, RADIUS, m_Bot->GetLevel())) {
        m_Bot->SetState(std::make_shared<ChaseState>(m_Bot));
        return;
    }

    m_LastEnemyPos = target;

    Vec2 vEnemy = m_Bot->GetEnemyTarget()->GetVelocity() / 16;
    vBot = m_Bot->GetVelocity();
    double proj_speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).BulletSpeed;

    Vec2 solution = CalculateShot(pos, target, vBot, vEnemy, proj_speed / 16.0 / 10.0);

    Util::GetDistance(pos, solution, &dx, &dy, nullptr);

    /* Move bot if it's stuck at a wall */
    if (PointingAtWall(rot, (unsigned)pos.x, (unsigned)pos.y, m_Bot->GetLevel())) {
        m_NearWall += dt;
        if (m_NearWall >= 1000) {
            // Bot has been near a wall for too long
            client->Down(true);

            int dir = Random::GetU32(0, 1) ? VK_LEFT : VK_RIGHT;

            dir == VK_LEFT ? client->Left(true) : client->Right(true);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            dir == VK_LEFT ? client->Left(false) : client->Right(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            dir == VK_LEFT ? client->Left(true) : client->Right(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            client->Down(false);
            dir == VK_LEFT ? client->Left(false) : client->Right(false);

            m_NearWall = 0;
        }
    } else {
        m_NearWall = 0;
    }

    /* Scale target distance by energy */
    tardist -= (int)std::floor(tardist * ((energypct / 100.0f) * .33f));

    if (timeGetTime() - m_LastSafeTime < ChaseFromSafeTime)
        tardist = 0;

    /* Handle rotation */
    target_rot = Util::GetTargetRotation(dx, dy);
    target_heading = Util::ContRotToVec(target_rot);

    if (!pursuit) {
        bool clockwise = Util::GetRotationDirection(heading, target_heading) == Direction::Right;

        if (rot != target_rot) {
            client->Left(!clockwise);
            client->Right(clockwise);
        } else {
            client->Left(false);
            client->Right(false);
        }

        if (dist > tardist && dist <= tardist * 1.5) {
            int max_speed = m_Bot->GetMemorySensor().GetShipSettings(m_Bot->GetShip()).InitialSpeed / 16 / 10;
            double target_speed = dist / (tardist * 1.5) * max_speed;

            m_Bot->SetSpeed(target_speed);
        } else {
            if (dist > tardist) {
                client->Down(false);
                client->Up(true);
            } else {
                client->Down(true);
                client->Up(false);
            }
        }

        if (dist > 20 && m_Bot->GetEnergyPercent() > 80)
            client->SetThrust(true);
        else
            client->SetThrust(false);
    }

    /* Fire bursts */
    m_BurstTimer += dt;
    if (m_BurstTimer >= 1000 && m_Bot->GetConfig().UseBurst) {
        if (!insafe && dist < 15 && InBurstArea(pos, m_Bot->GetGrid())) {
            client->Burst();

            m_BurstTimer = 0;
        }
    }

    /* Fire decoys */
    m_DecoyTimer += dt;
    if (!insafe && m_Bot->GetConfig().DecoyDelay > 0 && m_DecoyTimer >= m_Bot->GetConfig().DecoyDelay) {
        client->Decoy();
        m_DecoyTimer = 0;
    }

    /* Fire rockets */
    api::Ship ship = m_Bot->GetShip();
    const ShipSettings& settings = m_Bot->GetMemorySensor().GetShipSettings(ship);
    float rocket_duration_ms = settings.RocketTime * 10.0f;
    float tiles_per_ms = (settings.InitialSpeed / 10.0f / 16.0f) / 1000.0f;
    const int RocketThrust = 37;
    float speed_per_ms = RocketThrust / 100.0f / 16.0f / 1000.0f;

    m_RocketTimer += dt;
    if (m_RocketTimer > rocket_duration_ms) {
        Vec2 direction = Vec2Normalize(pos - target);
        float current_speed = (float)vBot.Length();
        float max_speed = settings.InitialSpeed / 10.0f / 16.0f;
        float ms_to_max = ((max_speed - current_speed) / 1000.0f) / speed_per_ms;
        Vec2 estimated_pos = pos + direction * (tiles_per_ms * (rocket_duration_ms - ms_to_max));

        if ((dist * dist) > (estimated_pos - pos).LengthSquared() * .75) {
            m_Bot->GetClient()->Rocket();
            m_RocketTimer = 0;
        }
    }
        
    double heading_dot = heading.Dot(target_heading);
    const double MaxGunDotDiff = 0.2;
    const double MaxBombDotDiff = 0.01;
    /* Only fire weapons if pointing at enemy */
    if ((1.0 - heading_dot) > MaxGunDotDiff && dist > 5 && (!m_Bot->GetConfig().Baseduel || m_Bot->IsInCenter())) {
        client->Gun(GunState::Off);
        return;
    }

    /* Calculate the dot product to determine if the bot is moving backwards */
    Vec2 norm_vel = m_Bot->GetVelocity();
    norm_vel.Normalize();
    double dot = heading.Dot(norm_vel);

    /* Handle bombing. Don't bomb if moving backwards unless energy is high. Don't close bomb. */
    if (!insafe && energypct > m_Bot->GetConfig().StopBombing && (1.0 - heading_dot) <= MaxBombDotDiff) {
        if ((dot >= 0.0 || energypct >= 50) && !PointingAtWall(rot, (unsigned)pos.x, (unsigned)pos.y, m_Bot->GetLevel()) && dist >= 7)
            client->Bomb();
    }

    // Only fire guns if the enemy is within range
    if (m_Bot->GetConfig().MinGunRange != 0 && dist > m_Bot->GetConfig().MinGunRange) {
        client->Gun(GunState::Off);
        return;
    }

    /* Handle gunning */
    if (energypct < m_Bot->GetConfig().RunPercent) {
        if (dist <= m_Bot->GetConfig().IgnoreDelayDistance)
            client->Gun(GunState::Constant);
        else
            client->Gun(GunState::Off);
    } else {
        if (insafe) {
            client->Gun(GunState::Off);
        } else {
            // Do bullet delay if the closest enemy isn't close, ignore otherwise
            if (dist > m_Bot->GetConfig().IgnoreDelayDistance)
                client->Gun(GunState::Tap, energypct);
            else
                client->Gun(GunState::Constant);
        }
    }
}
