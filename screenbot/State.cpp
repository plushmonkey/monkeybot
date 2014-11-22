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

#include <thread>
#include <iostream>
#include <tchar.h>
#include <regex>
#include <cmath>
#include <limits>

#define RADIUS 1

std::ostream& operator<<(std::ostream& out, StateType type) {
    static std::vector<std::string> states = {"Memory", "Chase", "Patrol", "Aggressive", "Attach"};
    out << states.at(static_cast<int>(type));
    return out;
}

MemoryState::MemoryState(Bot& bot) : State(bot) {
    m_SpawnX = bot.GetConfig().Get<int>("SpawnX");
    m_SpawnY = bot.GetConfig().Get<int>("SpawnY");
}

void MemoryState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    int rot = client->GetRotation();

    while (rot != 0) {
        client->Left(true);
        client->Update(dt);
        rot = client->GetRotation();
    }

    client->Left(false);

    m_Up = Random::GetU32(0, 1) == 0;

    if (m_Up)
        client->Up(true);
    else
        client->Down(true);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    client->Up(false);
    client->Down(false);

    client->Gun(GunState::Constant);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    client->Gun(GunState::Off);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    unsigned short minX = std::max((m_SpawnX * 16) - 300, 0);
    unsigned short maxX = std::max((m_SpawnX * 16) + 300, 0);
    unsigned short minY = std::max((m_SpawnY * 16) - 300, 0);
    unsigned short maxY = std::max((m_SpawnY * 16) + 300, 0);

    if (m_FindSpace.size() == 0) {
        std::vector<unsigned int> found = Memory::FindRange(m_Bot.GetProcessHandle(), 8000, 8500);

        for (unsigned int i = 0; i < found.size(); ++i) {
            unsigned int value = Memory::GetU32(m_Bot.GetProcessHandle(), found[i]);

            m_FindSpace[found[i]] = value;
        }
    } else {
        typedef std::map<unsigned, unsigned> FindMap;

        FindMap::iterator it = m_FindSpace.begin();
        FindMap::iterator end = m_FindSpace.end();

        // Loop through the found values and remove ones that don't match the movement
        while (it != end) {
            auto kv = *it;
            
            FindMap::iterator this_iter = it;
            ++it;
            // Get the new value at this address
            unsigned int x = Memory::GetU32(m_Bot.GetProcessHandle(), kv.first - 4);
            unsigned int y = Memory::GetU32(m_Bot.GetProcessHandle(), kv.first);

            // Remove the address from the list if it doesn't match the movement
            if (y == kv.second || y < minY|| y > maxY|| x < minX || x > maxX)
                m_FindSpace.erase(this_iter);
            else
                m_FindSpace[kv.first] = y; // Set the new value of the address
        }
    }

    tcout << "Possible memory locations: " << m_FindSpace.size() << std::endl;

    /* Restart the search and make sure it's in the safe zone */
    if (m_FindSpace.size() == 0) {
        client->Warp();
        return;
    }

    if (m_FindSpace.size() <= 5) {
        client->Gun(GunState::Constant);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        client->Gun(GunState::Off);

        for (int i = 0; i < 25; ++i) {
            for (auto it = m_FindSpace.begin(); it != m_FindSpace.end();) {
                unsigned int x = Memory::GetU32(m_Bot.GetProcessHandle(), it->first - 4);
                unsigned int y = Memory::GetU32(m_Bot.GetProcessHandle(), it->first);
                auto this_it = it;
                ++it;

                if (x < minX || x > maxX|| y < minX || y > maxX) {
                    // Remove this address because it's out of range
                    m_FindSpace.erase(this_it);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // All of the addresses were discarded in the check loop
        if (m_FindSpace.size() == 0) {
            client->Warp();
            return;
        }
        std::vector<unsigned> possible;
        
        possible.reserve(m_FindSpace.size());

        for (auto &kv : m_FindSpace)
            possible.push_back(kv.first);

        // Subtract 4 to get the x position address instead of y
        m_Bot.SetPosAddress(m_FindSpace.begin()->first - 4);
        
        tcout << "Position location found at " << std::hex << m_Bot.GetPosAddress() << std::dec << std::endl;

        m_Bot.SetPossibleAddr(possible);

        if (m_Bot.GetConfig().Get<bool>("Attach")) {
            auto state = std::make_shared<AttachState>(m_Bot);
            m_Bot.SetState(state);
        } else if (m_Bot.GetConfig().Get<bool>("Patrol")) {
            auto state = std::make_shared<PatrolState>(m_Bot);
            m_Bot.SetState(state);
        } else {
            auto state = std::make_shared<AggressiveState>(m_Bot);
            m_Bot.SetState(state);
            m_Bot.SetLastEnemy(timeGetTime());
        }
    }
}


State::State(Bot& bot)
    : m_Bot(bot) { }

AttachState::AttachState(Bot& bot)
    : State(bot)
{
    bot.GetClient()->ReleaseKeys();
    m_SpawnX = m_Bot.GetConfig().Get<int>(_T("SpawnX"));
    m_SpawnY = m_Bot.GetConfig().Get<int>(_T("SpawnY"));
}

void AttachState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();
    Vec2 pos = m_Bot.GetPos();

    if (client->OnSoloFreq()) {
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    std::string bot_name = client->GetName();
    int bot_freq = client->GetFreq();
    auto selected = client->GetSelectedPlayer();

    if (selected.get()) {
        // Move the ticker down to select teammate
        if (selected->GetName().compare(bot_name) == 0) {
            client->MoveTicker(true);
            return;
        }

        // Move the ticker up because it's targeting an enemy
        if (selected->GetFreq() != bot_freq) {
            client->MoveTicker(false);
            return;
        }
    }


    if (!(pos.x >= m_SpawnX - 20 && pos.x <= m_SpawnX + 20 &&
          pos.y >= m_SpawnY - 20 && pos.y <= m_SpawnY + 20))
    {
        // Detach if previous attach was successful
        client->Attach();
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    client->SetXRadar(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (m_Bot.GetEnergyPercent() == 100) {
        client->Attach();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


double GetPlanDistance(const Pathing::Plan& plan) {
    if (plan.size() < 2) return 0.0;

    Vec2 last(plan.at(0)->x, plan.at(0)->y);
    double total_dist = 0.0;
    
    for (const auto& node : plan) {
        Vec2 coord(node->x, node->y);
        double dist = 0.0;

        Util::GetDistance(last, coord, nullptr, nullptr, &dist);

        total_dist += dist;
        last = coord;
    }
    return total_dist;
}

ChaseState::ChaseState(Bot& bot) 
    : State(bot), 
      m_StuckTimer(0), 
      m_LastCoord(0, 0),
      m_LastRealEnemyCoord(0, 0)
{
    m_Bot.GetClient()->ReleaseKeys();

    m_MinGunRange = m_Bot.GetConfig().Get<int>(_T("MinGunRange"));
}

void ChaseState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    if (m_Bot.GetEnemyTarget() == Vec2(0, 0)) {
        client->ReleaseKeys();
        if (m_LastRealEnemyCoord != Vec2(0, 0)) {
            std::vector<Vec2> waypoints = { m_LastRealEnemyCoord };
            m_Bot.SetState(std::make_shared<PatrolState>(m_Bot, waypoints));
        } else {
            m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));
        }
        return;
    }

    Vec2 pos = m_Bot.GetPos();
    Vec2 enemy = m_Bot.GetEnemyTarget();

    m_LastRealEnemyCoord = enemy;

    if (Util::IsClearPath(pos, enemy, RADIUS, m_Bot.GetLevel())) {
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    m_StuckTimer += dt;

    // Check if stuck every 2.5 seconds
    if (m_StuckTimer >= 2500) {
        double stuckdist;

        Util::GetDistance(pos, m_LastCoord, nullptr, nullptr, &stuckdist);

        if (stuckdist <= 10) {
            // Stuck
            client->Up(false);
            client->Down(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            client->Down(false);
        }

        m_LastCoord = pos;
        m_StuckTimer = 0;
    }

    if (m_Bot.GetGrid().IsOpen((short)pos.x, (short)pos.y) && m_Bot.GetGrid().IsOpen((short)enemy.x, (short)enemy.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps((short)enemy.x, (short)enemy.y, (short)pos.x, (short)pos.y, m_Bot.GetGrid());
    } else {
        if (m_Plan.size() == 0)
            m_Plan.push_back(m_Bot.GetGrid().GetNode((short)enemy.x, (short)enemy.y));
    }

    if (m_Plan.size() == 0) return;

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Vec2 next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);
    while (dist < 3 && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Vec2(next_node->x, next_node->y);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    int rot = client->GetRotation();
    int target_rot = Util::GetTargetRotation(dx, dy);

    int away = std::abs(rot - target_rot);

    double total_dist = GetPlanDistance(m_Plan);

    bool fire = m_MinGunRange == 0 || total_dist < m_MinGunRange;

    if (fire && !client->InSafe(pos, m_Bot.GetLevel()))
        client->Gun(GunState::Tap, m_Bot.GetEnergyPercent());
    else
        client->Gun(GunState::Off);

    if (rot != -1 && rot != target_rot) {
        int dir = 0;

        if (away < 20 && rot < target_rot) dir = 1;
        if (away < 20 && rot > target_rot) dir = 0;

        if (away > 20 && rot < target_rot) dir = 0;
        if (away > 20 && rot > target_rot) dir = 1;

        if (dir == 0) {
            client->Right(false);
            client->Left(true);
        } else {
            client->Right(true);
            client->Left(false);
        } 
    } else {
        client->Right(false);
        client->Left(false);
    }

    client->Up(true);
}

PatrolState::PatrolState(Bot& bot, std::vector<Vec2> waypoints)
    : State(bot), 
      m_LastBullet(timeGetTime()),
      m_StuckTimer(0),
      m_LastCoord(0, 0)
{
    m_Bot.GetClient()->ReleaseKeys();

    m_Patrol        = m_Bot.GetConfig().Get<bool>(_T("Patrol"));

    std::string waypoints_str = m_Bot.GetConfig().Get<std::string>(_T("Waypoints"));
    std::regex coord_re(R"::(\(([0-9]*?),\s*?([0-9]*?)\))::");

    std::sregex_iterator begin(waypoints_str.begin(), waypoints_str.end(), coord_re);
    std::sregex_iterator end;

    for (std::sregex_iterator iter = begin; iter != end; ++iter) {
        std::smatch match = *iter;
        std::string xstr = match[1];
        std::string ystr = match[2];
        int x = strtol(xstr.c_str(), nullptr, 10);
        int y = strtol(ystr.c_str(), nullptr, 10);

        m_FullWaypoints.emplace_back((float)x, (float)y);
    }

    if (waypoints.size() > 0)
        m_Waypoints = waypoints;
    else
        ResetWaypoints(false);
}

void PatrolState::ResetWaypoints(bool full) {
    m_Waypoints = m_FullWaypoints;

    if (full) return;

    double closestdist = std::numeric_limits<double>::max();
    std::vector<Vec2>::iterator closest = m_Waypoints.begin();
    Vec2 pos = m_Bot.GetPos();
    

    // Find the closet waypoint that isn't the one just visited
    for (auto i = m_Waypoints.begin(); i != m_Waypoints.end(); ++i) {
        int dx, dy;
        double dist;

        Util::GetDistance(pos, *i, &dx, &dy, &dist);

        if (dist > 25 && dist < closestdist) {
            closestdist = dist;
            closest = i;
        }
    }

    // Remove all the waypoints before the closest
    if (closest != m_Waypoints.begin())
        m_Waypoints.erase(m_Waypoints.begin(), closest);
}

void PatrolState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    if (!m_Patrol || m_Bot.GetEnemyTarget() != Vec2(0, 0)) {
        // Target found, switch to aggressive
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    client->SetXRadar(true);

    if (m_Waypoints.size() == 0) {
        ResetWaypoints();
        client->Up(false);
        return;
    }

    if (timeGetTime() >= m_LastBullet + 60000) {
        client->Gun(GunState::Tap);
        client->Gun(GunState::Off);
        m_LastBullet = timeGetTime();
    }

    Vec2 target = m_Waypoints.front();
    Vec2 pos = m_Bot.GetPos();

    m_StuckTimer += dt;

    // Check if stuck every 3.5 seconds
    if (m_StuckTimer >= 3500) {
        double stuckdist;

        Util::GetDistance(pos, m_LastCoord, nullptr, nullptr, &stuckdist);

        if (stuckdist <= 10) {
            // Stuck
            client->Up(false);
            client->Down(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(750));
            client->Down(false);
        }

        m_LastCoord = pos;
        m_StuckTimer = 0;
    }

    double closedist;
    Util::GetDistance(pos, target, nullptr, nullptr, &closedist);

    if (closedist <= 25) {
        m_Waypoints.erase(m_Waypoints.begin());
        if (m_Waypoints.size() == 0) return;
        target = m_Waypoints.front();
    }

    if (!m_Bot.GetGrid().IsSolid((short)pos.x, (short)pos.y) && m_Bot.GetGrid().IsOpen((short)target.x, (short)target.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps((short)target.x, (short)target.y, (short)pos.x, (short)pos.y, m_Bot.GetGrid());
    }

    if (m_Plan.size() == 0) {
        m_Waypoints.erase(m_Waypoints.begin());

        if (m_Bot.GetConfig().Get<bool>("DevaBDB")) {
            // Find possible waypoints within a certain radius
            const int SearchRadius = 200;
            const int CenterRadius = 60;
            const int SafeRadius = 6;
            const int SpawnX = m_Bot.GetConfig().Get<int>("SpawnX");
            const int SpawnY = m_Bot.GetConfig().Get<int>("SpawnY");

            std::vector<Vec2> waypoints;

            // Only add safe tiles that aren't near other waypoints
            for (int y = int(pos.y - SearchRadius); y < int(pos.y + SearchRadius) && y < 1024; ++y) {
                for (int x = int(pos.x - SearchRadius); x < int(pos.x + SearchRadius) &&  x < 1024; ++x) {
                    int id = m_Bot.GetLevel().GetTileID(x, y);

                    if (id != 171) continue;
                    if (!m_Bot.GetGrid().IsOpen(x, y)) continue;
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

                auto plan = jps((short)waypoint.x, (short)waypoint.y, (short)pos.x, (short)pos.y, m_Bot.GetGrid());

                if (plan.size() != 0) { // A possible waypoint found
                    m_Waypoints.clear();
                    m_Waypoints.push_back(waypoint);
                    client->Up(false);
                    return;
                }
            }

            std::cout << "no waypoints\n";
        }
        
        //ResetWaypoints();
      //  client->Warp();
        client->Up(false);
        return;
    }

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Vec2 next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    while (dist < 3 && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Vec2(next_node->x, next_node->y);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    int rot = client->GetRotation();
    int target_rot = Util::GetTargetRotation(dx, dy);

    int away = std::abs(rot - target_rot);

    if (rot != -1 && rot != target_rot) {
        int dir = 0;

        if (away < 20 && rot < target_rot) dir = 1;
        if (away < 20 && rot > target_rot) dir = 0;

        if (away > 20 && rot < target_rot) dir = 0;
        if (away > 20 && rot > target_rot) dir = 1;

        if (dir == 0) {
            client->Right(false);
            client->Left(true);
        } else {
            client->Right(true);
            client->Left(false);
        }
    } else {
        client->Right(false);
        client->Left(false);
    }

    client->Up(true);
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

AggressiveState::AggressiveState(Bot& bot)
    : State(bot), 
      m_LastEnemyPos(0,0),
      m_LastEnemyTimer(0),
      m_EnemyVelocity(0, 0),
      m_LastNonSafeTime(timeGetTime()),
      m_NearWall(0)
{
    m_RunPercent     = m_Bot.GetConfig().Get<int>(_T("RunPercent"));
    m_XPercent       = m_Bot.GetConfig().Get<int>(_T("XPercent"));
    m_SafeResetTime  = m_Bot.GetConfig().Get<int>(_T("SafeResetTime"));
    m_TargetDist     = m_Bot.GetConfig().Get<int>(_T("TargetDistance"));
    m_RunDist        = m_Bot.GetConfig().Get<int>(_T("RunDistance"));
    m_StopBombing    = m_Bot.GetConfig().Get<int>(_T("StopBombing"));
    m_DistFactor     = m_Bot.GetConfig().Get<int>(_T("DistanceFactor"));
    m_MemoryScanning = m_Bot.GetConfig().Get<bool>(_T("MemoryScanning"));
    m_OnlyCenter     = m_Bot.GetConfig().Get<bool>(_T("OnlyCenter"));
    m_Patrol         = m_Bot.GetConfig().Get<bool>(_T("Patrol"));
    m_MinGunRange    = m_Bot.GetConfig().Get<int>(_T("MinGunRange"));
    m_Baseduel       = m_Bot.GetConfig().Get<bool>(_T("DevaBDB"));
    m_ProjectileSpeed = m_Bot.GetConfig().Get<int>(_T("ProjectileSpeed"));

    m_Bot.GetClient()->ReleaseKeys();
    
    if (m_DistFactor < 1) m_DistFactor = 10;
}

Vec2 CalculateShot(const Vec2& pShooter, const Vec2& pTarget, const Vec2& vTarget, double sProjectile) {
    Vec2 totarget = pTarget - pShooter;

    double a = Vec2::Dot(vTarget, vTarget) - sProjectile * sProjectile;
    double b = 2 * Vec2::Dot(vTarget, totarget);
    double c = Vec2::Dot(totarget, totarget);

    Vec2 solution;

    double disc = (b * b) - 4 * a * c;
    if (disc < 0)
        return pTarget;

    double t1 = (-b + std::sqrt(disc)) / (2 * a);
    double t2 = (-b - std::sqrt(disc)) / (2 * a);
    double t = 0.0;
    if (t1 < t2 && t1 >= 0)
        t = t1;
    else
        t = t2;

    // Only use the calculated shot if its collision is within acceptable timeframe
    if (t < 0 || t > 5) {
        float dist = totarget.Length();
        t = (float)std::min(dist / sProjectile, 5.0);
    }

    solution = pTarget + (vTarget * (float)t);

    return solution;
}

void AggressiveState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    Vec2 pos = m_Bot.GetPos();

    bool insafe = client->InSafe(pos, m_Bot.GetLevel());
    int tardist = m_TargetDist;
    int energypct = m_Bot.GetEnergyPercent();

    if (energypct < m_RunPercent && !insafe) {
        tardist = m_RunDist;
        client->ReleaseKeys();
    }

    /* Turn off x if energy low, turn back on when high */
    client->SetXRadar(energypct > m_XPercent);

    /* Wait in safe for energy */
    if (insafe && energypct < 50) {
        client->Gun(GunState::Tap);
        client->ReleaseKeys();
        return;
    }

    Vec2 target = m_Bot.GetEnemyTarget();
    DWORD cur_time = timeGetTime();

    /* Only update if there is a target enemy */
    if (target != Vec2(0, 0)) {
        int dx, dy;
        double dist;

        Util::GetDistance(pos, target, &dx, &dy, &dist);

        if (!Util::IsClearPath(pos, target, RADIUS, m_Bot.GetLevel())) {
            m_Bot.SetState(std::make_shared<ChaseState>(m_Bot));
            return;
        }
        
        m_LastEnemyTimer += dt;
        if (m_LastEnemyTimer >= 250) {
            m_EnemyVelocity = (target - m_LastEnemyPos) / (m_LastEnemyTimer / 250.0f);
            m_LastEnemyPos = target;
            m_LastEnemyTimer = 0;
        }

        if (!insafe)
            m_LastNonSafeTime = cur_time;

        /* Handle trying to get out of safe */
        if (m_SafeResetTime > 0 && insafe && cur_time >= m_LastNonSafeTime + m_SafeResetTime) {
            client->Warp();
            m_LastNonSafeTime = cur_time;
        }

        Vec2 vEnemy = m_EnemyVelocity * 4;
        Vec2 vBot = m_Bot.GetVelocity();

        Vec2 solution = CalculateShot(pos + vBot, target, vEnemy - vBot, m_ProjectileSpeed / 16.0 / 10.0);

        Util::GetDistance(pos, solution, &dx, &dy, nullptr);

        int target_rot = Util::GetTargetRotation(dx, dy);
        int rot = client->GetRotation();
        int away = std::abs(rot - target_rot);
        
        /* Move bot if it's stuck at a wall */
        if (m_MemoryScanning && PointingAtWall(rot, (unsigned)pos.x, (unsigned)pos.y, m_Bot.GetLevel())) {
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

        /* Handle rotation */
        if (rot != -1 && rot != target_rot) {
            int dir = 0;

            if (away < 20 && rot < target_rot) dir = 1;
            if (away < 20 && rot > target_rot) dir = 0;

            if (away > 20 && rot < target_rot) dir = 0;
            if (away > 20 && rot > target_rot) dir = 1;

            if (dir == 0) {
                client->Right(false);
                client->Left(true);
            } else {
                client->Right(true);
                client->Left(false);
            }
        } else {
            client->Right(false);
            client->Left(false);
        }

        /* Handle distance movement */
        if (dist > tardist) {
            client->Down(false);
            client->Up(true);
        } else {
            client->Down(true);
            client->Up(false);
        }

        /* Only fire weapons if pointing at enemy */
        if (rot != target_rot && dist > 5 && !m_Baseduel) {
            client->Gun(GunState::Off);
            return;
        }

        /* Handle bombing */
        if (!insafe && energypct > m_StopBombing) {
            if (!PointingAtWall(rot, (unsigned)pos.x, (unsigned)pos.y, m_Bot.GetLevel()) && dist >= 7)
                client->Bomb();
        }

        // Only fire guns if the enemy is within range
        if (m_MinGunRange != 0 && dist > m_MinGunRange) {
            client->Gun(GunState::Off);
            return;
        }

        /* Handle gunning */
        if (energypct < m_RunPercent) {
            if (dist <= 7)
                client->Gun(GunState::Constant);
            else
                client->Gun(GunState::Off);
        } else {
            if (insafe) {
                client->Gun(GunState::Off);
            } else {
                // Do bullet delay if the closest enemy isn't close, ignore otherwise
                if (dist > 7)
                    client->Gun(GunState::Tap, energypct);
                else
                    client->Gun(GunState::Constant);
            }
        }

    } else {
        /* Clear input when there is no enemy */
        m_LastNonSafeTime = cur_time;

        /* Switch to patrol state when there is no enemy to fight */
        if (m_Patrol) {
            if (m_LastEnemyPos != Vec2(0, 0)) {
                std::vector<Vec2> waypoints = { m_LastEnemyPos };
                m_Bot.SetState(std::make_shared<PatrolState>(m_Bot, waypoints));
            } else {
                m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));
            }
            client->ReleaseKeys();
            return;
        }

        client->ReleaseKeys();
        
        /* Warp to keep the bot in game */
        if (cur_time > m_Bot.GetLastEnemy() + 1000 * 60) {
            client->Warp();
            m_Bot.SetLastEnemy(cur_time);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
