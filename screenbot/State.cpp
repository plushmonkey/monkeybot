#include "State.h"
#include "Keyboard.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Bot.h"
#include "Client.h"
#include <thread>
#include <iostream>
#include <tchar.h>
#include "Memory.h"
#include "Random.h"
#include "Tokenizer.h"
#include <regex>
#include <cmath>
#include <limits>

#define RADIUS 1

std::ostream& operator<<(std::ostream& out, StateType type) {
    static std::vector<std::string> states = {"Memory", "Chase", "Patrol", "Aggressive"};
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

        client->Gun(GunState::Off);

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

        if (m_Bot.GetConfig().Get<bool>("Patrol")) {
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

ChaseState::ChaseState(Bot& bot) 
    : State(bot), 
      m_StuckTimer(0), 
      m_LastCoord(0, 0),
      m_LastRealEnemyCoord(0, 0)
{
    m_Bot.GetClient()->ReleaseKeys();
}

bool Enclosed(Coord pos, int radius, Pathing::Grid<short>& grid) {
    Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

    static std::vector<Coord> directions = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
        { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
    };

    if (grid.IsSolid(pos.x, pos.y)) return true;

    for (Coord& dir : directions) {
        int solids = 0;
        for (int i = 0; i < radius; ++i) {
            if (!grid.IsOpen(pos.x + dir.x * i, pos.y + dir.y * i))
                solids++;
        }

        if (solids != radius) return false;
    }
    return true;
}

void ChaseState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    if (m_Bot.GetEnemyTarget() == Coord(0, 0)) {
        client->ReleaseKeys();
        if (m_LastRealEnemyCoord != Coord(0, 0)) {
            std::vector<Coord> waypoints = { m_LastRealEnemyCoord };
            m_Bot.SetState(std::make_shared<PatrolState>(m_Bot, waypoints));
        } else {
            m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));
        }
        return;
    }

    Coord pos(m_Bot.GetX(), m_Bot.GetY());
    Coord enemy = m_Bot.GetEnemyTarget();

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

    if (m_Bot.GetGrid().IsOpen(pos.x, pos.y) && m_Bot.GetGrid().IsOpen(enemy.x, enemy.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps(enemy.x, enemy.y, pos.x, pos.y, m_Bot.GetGrid());
    } else {
        if (m_Plan.size() == 0)
            m_Plan.push_back(m_Bot.GetGrid().GetNode(enemy.x, enemy.y));
    }

    if (m_Plan.size() == 0) return;

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Coord next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);
    while (dist < 3 && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Coord(next_node->x, next_node->y);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    int rot = client->GetRotation();
    int target_rot = Util::GetTargetRotation(dx, dy);

    int away = std::abs(rot - target_rot);

    if (dist < 20 && !client->InSafe(pos, m_Bot.GetLevel())) {
        client->Gun(GunState::Tap, m_Bot.GetEnergyPercent());
        client->Gun(GunState::Off);
    }

    bool go = true;

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
    //    if (dist < 7) go = false;
    } else {
        client->Right(false);
        client->Left(false);
    }

    client->Up(go);
}

PatrolState::PatrolState(Bot& bot, std::vector<Coord> waypoints)
    : State(bot), 
      m_LastBullet(timeGetTime()),
      m_StuckTimer(0),
      m_LastCoord(0, 0)
{
    m_Bot.GetClient()->ReleaseKeys();

    m_Patrol = m_Bot.GetConfig().Get<bool>(_T("Patrol"));

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

        m_FullWaypoints.emplace_back(x, y);
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
    std::vector<Coord>::iterator closest = m_Waypoints.begin();

    Coord pos(m_Bot.GetX(), m_Bot.GetY());

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

    if (!m_Patrol || m_Bot.GetEnemyTarget() != Coord(0, 0)) {
        // Target found, switch to aggressive
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    client->SetXRadar(true);

    if (m_Waypoints.size() == 0) {
        ResetWaypoints();
        return;
    }

    if (timeGetTime() >= m_LastBullet + 60000) {
        client->Gun(GunState::Tap);
        client->Gun(GunState::Off);
        m_LastBullet = timeGetTime();
    }

    Coord target = m_Waypoints.front();
    Coord pos(m_Bot.GetX(), m_Bot.GetY());

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

    if (!m_Bot.GetGrid().IsSolid(pos.x, pos.y) && m_Bot.GetGrid().IsOpen(target.x, target.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps(target.x, target.y, pos.x, pos.y, m_Bot.GetGrid());
    }

    if (m_Plan.size() == 0) {
        ResetWaypoints();
        client->ReleaseKeys();
        return;
    }

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Coord next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    while (dist < 3 && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Coord(next_node->x, next_node->y);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    int rot = client->GetRotation();
    int target_rot = Util::GetTargetRotation(dx, dy);

    int away = std::abs(rot - target_rot);

    bool go = true;

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
        //if (dist < 7) go = false;
    } else {
        client->Right(false);
        client->Left(false);
    }

    client->Up(go);
}

bool NearWall(unsigned x, unsigned y, const Level& level) {
    bool nw = level.IsSolid(x - 1, y - 1);
    bool n = level.IsSolid(x, y - 1);
    bool ne = level.IsSolid(x + 1, y - 1);
    bool e = level.IsSolid(x + 1, y);
    bool se = level.IsSolid(x + 1, y + 1);
    bool s = level.IsSolid(x, y + 1);
    bool sw = level.IsSolid(x - 1, y + 1);
    bool w = level.IsSolid(x - 1, y);
    return nw || n || ne || e || se || s || sw || w;
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

    m_Bot.GetClient()->ReleaseKeys();
    
    if (m_DistFactor < 1) m_DistFactor = 10;
}

void AggressiveState::Update(DWORD dt) {
    ClientPtr client = m_Bot.GetClient();

    Coord pos(m_Bot.GetX(), m_Bot.GetY());

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

    Coord target = m_Bot.GetEnemyTarget();
    DWORD cur_time = timeGetTime();

    /* Only update if there is a target enemy */
    if (target != Coord(0, 0)) {
        int dx, dy;
        double dist;

        Util::GetDistance(pos, target, &dx, &dy, &dist);

        if (!Util::IsClearPath(pos, target, RADIUS, m_Bot.GetLevel())) {
            m_Bot.SetState(std::make_shared<ChaseState>(m_Bot));
            return;
        }
        
        if (cur_time > m_LastEnemyTimer + 500) {
            m_EnemyVelocity.x = target.x - m_LastEnemyPos.x;
            m_EnemyVelocity.y = target.y - m_LastEnemyPos.y;
            m_LastEnemyTimer = cur_time;
            m_LastEnemyPos = target;
        }

        if (!insafe)
            m_LastNonSafeTime = cur_time;

        /* Handle trying to get out of safe */
        if (m_SafeResetTime > 0 && insafe && cur_time >= m_LastNonSafeTime + m_SafeResetTime) {
            client->Warp();
            m_LastNonSafeTime = cur_time;
        }

        
        int dxchange = static_cast<int>(m_EnemyVelocity.x * (dist / m_DistFactor));
        int dychange = static_cast<int>(m_EnemyVelocity.y * (dist / m_DistFactor));

        if (std::abs(m_EnemyVelocity.x) < 15)
            dx += dxchange;
        if (std::abs(m_EnemyVelocity.y) < 15)
            dy += dychange;

        int target_rot = Util::GetTargetRotation(dx, dy);
        int rot = client->GetRotation();
        int away = std::abs(rot - target_rot);

        /* Move bot if it's stuck at a wall */
        if (m_MemoryScanning && PointingAtWall(rot, pos.x, pos.y, m_Bot.GetLevel())) {
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
        if (rot != target_rot) {
            client->Gun(GunState::Off);
            return;
        }

        /* Handle bombing */
        if (!insafe && energypct > m_StopBombing) {
            if (!PointingAtWall(rot, pos.x, pos.y, m_Bot.GetLevel()) && dist >= 7)
                client->Bomb();
        }

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

        if (m_Patrol) {
            if (m_LastEnemyPos != Coord(0, 0)) {
                std::vector<Coord> waypoints = { m_LastEnemyPos };
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
