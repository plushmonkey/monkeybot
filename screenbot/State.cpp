#include "State.h"
#include "Keyboard.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Bot.h"
#include <thread>
#include <iostream>
#include <tchar.h>
#include "Memory.h"
#include "Random.h"
#include <cmath>
#include <limits>

#define RADIUS 0

const Coord NullCoord(0, 0);

PosFindState::PosFindState(Bot& bot) : State(bot) {

}

void PosFindState::Update(DWORD dt) {
    Coord target = m_Bot.GetEnemyTarget();
    
    if (target == Coord(0, 0)) return;

    TargetInfo info = m_Bot.GetEnemyTargetInfo();
    int dx = info.dx;
    int dy = info.dy;

    Coord pos(m_Bot.GetX(), m_Bot.GetY());

    Coord guess = Util::FindTargetPos(pos, target, m_Bot.GetGrabber(), m_Bot.GetRadar(), m_Bot.GetLevel());
  //  Coord guess(pos.x + dx * 2, pos.y + dy * 2);

    std::cout << "Pos: " << pos << " Guess: " << guess << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

MemoryState::MemoryState(Bot& bot) : State(bot) {

}

void MemoryState::Update(DWORD dt) {
    Keyboard& keyboard = m_Bot.GetKeyboard();

    int rot = Util::GetRotation(m_Bot.GetShip());

    while (rot != 0) {
        keyboard.Down(VK_LEFT);

        m_Bot.GetGrabber()->Update();
        m_Bot.GetShip()->Update();

        rot = Util::GetRotation(m_Bot.GetShip());
    }

    keyboard.Up(VK_LEFT);

    m_Up = Random::GetU32(0, 1) == 0;

    if (m_Up)
        keyboard.Down(VK_UP);
    else
        keyboard.Down(VK_DOWN);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    keyboard.Up(VK_DOWN);
    keyboard.Up(VK_UP);

    keyboard.Send(VK_CONTROL);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
            if (y == kv.second || y < 8000 || y > 8500 || x < 8000 || x > 8500)
                m_FindSpace.erase(this_iter);
            else
                m_FindSpace[kv.first] = y; // Set the new value of the address
        }
    }

    tcout << "Possible memory locations: " << m_FindSpace.size() << std::endl;

    /* Restart the search and make sure it's in the safe zone */
    if (m_FindSpace.size() == 0) {
        m_Bot.SetXRadar(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        keyboard.Send(VK_INSERT);
        return;
    }

    if (m_FindSpace.size() <= 5) {
        keyboard.Down(VK_CONTROL); // Stop moving
        for (int i = 0; i < 25; ++i) {
            for (auto it = m_FindSpace.begin(); it != m_FindSpace.end();) {
                unsigned int x = Memory::GetU32(m_Bot.GetProcessHandle(), it->first - 4);
                unsigned int y = Memory::GetU32(m_Bot.GetProcessHandle(), it->first);
                auto this_it = it;
                ++it;

                if (x < 8000 || x > 8500 || y < 8000 || y > 8500) {
                    // Remove this address because it's out of range
                    m_FindSpace.erase(this_it);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        keyboard.Up(VK_CONTROL);

        // All of the addresses were discarded in the check loop
        if (m_FindSpace.size() == 0) {
            m_Bot.SetXRadar(false);
            keyboard.Send(VK_INSERT);
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
        }

        //auto state = std::make_shared<PosFindState>(m_Bot);
        //m_Bot.SetState(state);
    }
}


State::State(Bot& bot)
    : m_Bot(bot) { }

FollowState::FollowState(Bot& bot) : State(bot) { 
    m_Bot.GetKeyboard().ReleaseAll();
}

void FollowState::Update(DWORD dt) {
    Keyboard& keyboard = m_Bot.GetKeyboard();

    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();

    if (m_Bot.GetEnemyTarget() == Coord(0, 0)) {
        keyboard.ReleaseAll();
        m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));
        return;
    }

    Coord pos(m_Bot.GetX(), m_Bot.GetY());
    int edx = m_Bot.GetEnemyTargetInfo().dx;
    int edy = m_Bot.GetEnemyTargetInfo().dy;
    int x = pos.x;
    int y = pos.y;
    Coord enemy = Util::FindTargetPos(pos, m_Bot.GetEnemyTarget(), m_Bot.GetGrabber(), m_Bot.GetRadar(), m_Bot.GetLevel());
    //Coord enemy(x + edx * 2, y + edy * 2);

    if (Util::IsClearPath(pos, enemy, RADIUS, m_Bot.GetLevel())) {
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    if (!m_Bot.GetLevel().IsSolid(enemy.x, enemy.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps(enemy.x, enemy.y, pos.x, pos.y, m_Bot.GetGrid());
    }

    if (m_Plan.size() == 0) return;

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Coord next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    while (next == pos && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Coord(next_node->x, next_node->y);
    }

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    dx = -dx;
    dy = -dy;

    int rot = Util::GetRotation(ship);
    int target_rot = Util::GetTargetRotation(dx, dy);

    int away = std::abs(rot - target_rot);

    if (rot != -1 && rot != target_rot) {
        int dir = 0;

        if (away < 20 && rot < target_rot) dir = 1;
        if (away < 20 && rot > target_rot) dir = 0;

        if (away > 20 && rot < target_rot) dir = 0;
        if (away > 20 && rot > target_rot) dir = 1;

        if (dir == 0) {
            keyboard.Up(VK_RIGHT);
            keyboard.Down(VK_LEFT);
        } else {
            keyboard.Up(VK_LEFT);
            keyboard.Down(VK_RIGHT);
        }
    } else {
        keyboard.Up(VK_LEFT);
        keyboard.Up(VK_RIGHT);
    }

    keyboard.Down(VK_UP);
}

PatrolState::PatrolState(Bot& bot, std::vector<Coord> waypoints) 
    : State(bot), 
      m_Waypoints(waypoints),
      m_LastBullet(timeGetTime()),
      m_StuckTimer(0),
      m_LastCoord(0, 0)
{
    if (m_Waypoints.size() == 0)
        ResetWaypoints(false);

    m_Bot.GetKeyboard().ReleaseAll();

    m_Patrol = m_Bot.GetConfig().Get<bool>(_T("Patrol"));
}

void PatrolState::ResetWaypoints(bool full) {
    m_Waypoints.emplace_back(400, 585);
    m_Waypoints.emplace_back(565, 580);
    m_Waypoints.emplace_back(600, 475);
    m_Waypoints.emplace_back(512, 460);
    m_Waypoints.emplace_back(385, 505);
    m_Waypoints.emplace_back(425, 460);

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
    Keyboard& keyboard = m_Bot.GetKeyboard();

    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();

    if (!m_Patrol || m_Bot.GetEnemyTarget() != Coord(0, 0)) {
        // Target found, switch to aggressive
        m_Bot.SetState(std::make_shared<AggressiveState>(m_Bot));
        return;
    }

    m_Bot.SetXRadar(true);

    if (m_Waypoints.size() == 0) {
        ResetWaypoints();
        return;
    }

    if (timeGetTime() >= m_LastBullet + 60000) {
        keyboard.Send(VK_CONTROL);
        m_LastBullet = timeGetTime();
    }

    Coord target = m_Waypoints.front();
    Coord pos(m_Bot.GetX(), m_Bot.GetY());
    int closedx, closedy;
    double closedist;
    Util::GetDistance(pos, target, &closedx, &closedy, &closedist);

    m_StuckTimer += dt;

    // Check if stuck every 2.5 seconds
    if (false && m_StuckTimer >= 2500) {
        int stuckdx, stuckdy;
        double stuckdist;

        Util::GetDistance(pos, m_LastCoord, &stuckdx, &stuckdy, &stuckdist);

        if (stuckdist <= 10) {
            // Stuck
            keyboard.Up(VK_UP);
            keyboard.Down(VK_DOWN);
            std::this_thread::sleep_for(std::chrono::milliseconds(750));
            keyboard.Up(VK_DOWN);
        }

        m_LastCoord = pos;
        m_StuckTimer = 0;
    }

    if (closedist <= 25) {
        m_Waypoints.erase(m_Waypoints.begin());
        if (m_Waypoints.size() == 0) return;
    }

    if (!m_Bot.GetLevel().IsSolid(target.x, target.y)) {
        Pathing::JumpPointSearch jps(Pathing::Heuristic::Manhattan<short>);

        m_Plan = jps(target.x, target.y, pos.x, pos.y, m_Bot.GetGrid());
    }

    if (m_Plan.size() == 0)
        return;

    Pathing::JPSNode* next_node = m_Plan.at(0);
    Coord next(next_node->x, next_node->y);

    int dx, dy;
    double dist = 0.0;

    Util::GetDistance(pos, next, &dx, &dy, &dist);

    while (dist < 5 && m_Plan.size() > 1) {
        m_Plan.erase(m_Plan.begin());
        next_node = m_Plan.at(0);
        next = Coord(next_node->x, next_node->y);
        Util::GetDistance(pos, next, &dx, &dy, &dist);
    }

    dx = -dx;
    dy = -dy;

    int rot = Util::GetRotation(ship);
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
            keyboard.Up(VK_RIGHT);
            keyboard.Down(VK_LEFT);
        } else {
            keyboard.Up(VK_LEFT);
            keyboard.Down(VK_RIGHT);
        }
        if (dist < 7) go = false;
    } else {
        keyboard.Up(VK_LEFT);
        keyboard.Up(VK_RIGHT);
    }

    if (go)
        keyboard.Down(VK_UP);
    else
        keyboard.Up(VK_UP);
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
      m_LastBomb(timeGetTime()),
      m_LastNonSafeTime(timeGetTime()),
      m_LastBullet(0),
      m_NearWall(0)
{
    m_RunPercent     = m_Bot.GetConfig().Get<int>(_T("RunPercent"));
    m_XPercent       = m_Bot.GetConfig().Get<int>(_T("XPercent"));
    m_SafeResetTime  = m_Bot.GetConfig().Get<int>(_T("SafeResetTime"));
    m_TargetDist     = m_Bot.GetConfig().Get<int>(_T("TargetDistance"));
    m_RunDist        = m_Bot.GetConfig().Get<int>(_T("RunDistance"));
    m_StopBombing    = m_Bot.GetConfig().Get<int>(_T("StopBombing"));
    m_BombTime       = m_Bot.GetConfig().Get<int>(_T("BombTime"));
    m_FireBombs      = m_Bot.GetConfig().Get<bool>(_T("FireBombs"));
    m_FireGuns       = m_Bot.GetConfig().Get<bool>(_T("FireGuns"));
    m_DistFactor     = m_Bot.GetConfig().Get<int>(_T("DistanceFactor"));
    m_BulletDelay    = m_Bot.GetConfig().Get<int>(_T("BulletDelay")) * 10;
    m_ScaleDelay     = m_Bot.GetConfig().Get<bool>(_T("ScaleDelay"));
    m_MemoryScanning = m_Bot.GetConfig().Get<bool>(_T("MemoryScanning"));
    m_OnlyCenter     = m_Bot.GetConfig().Get<bool>(_T("OnlyCenter"));
    m_Patrol         = m_Bot.GetConfig().Get<bool>(_T("Patrol"));

    m_Bot.GetKeyboard().ReleaseAll();
    if (m_DistFactor < 1) m_DistFactor = 10;
}

void AggressiveState::Update(DWORD dt) {
    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();
    ScreenAreaPtr& player = m_Bot.GetPlayer();
    Keyboard& keyboard = m_Bot.GetKeyboard();

    unsigned int x = m_Bot.GetX();
    unsigned int y = m_Bot.GetY();

    bool insafe = Util::PlayerInSafe(player);
    int tardist = m_TargetDist;
    static bool keydown;

    int energypct = m_Bot.GetEnergyPercent();

    if (energypct < m_RunPercent && !insafe) {
        tardist = m_RunDist;
        keyboard.ReleaseAll();
    }

    if (m_ScaleDelay)
        m_CurrentBulletDelay = static_cast<int>(std::ceil(m_BulletDelay * (1.0f + (100.0f - energypct) / 100)));
    else
        m_CurrentBulletDelay = m_BulletDelay;

    /* Turn off x if energy low, turn back on when high */
    m_Bot.SetXRadar(energypct > m_XPercent);

    /* Wait in safe for energy */
    if (insafe && energypct < 50) {
        keyboard.Send(VK_CONTROL);
        keyboard.ReleaseAll();
        return;
    }

    Coord target = m_Bot.GetEnemyTarget();

    if (target == Coord(0, 0)) {
        m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));
        keyboard.ReleaseAll();
        return;
    }

    DWORD cur_time = timeGetTime();

    /* Only update if there is a target enemy */
    if (target != NullCoord) {
        TargetInfo info = m_Bot.GetEnemyTargetInfo();
        double dist = info.dist;
        int dx = info.dx;
        int dy = info.dy;
        Coord pos(x, y);
        Coord enemy(x + dx * 2, y + dy * 2);

        if (!Util::IsClearPath(pos, enemy, RADIUS, m_Bot.GetLevel())) {
            m_Bot.SetState(std::make_shared<FollowState>(m_Bot));
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
            m_Bot.SetXRadar(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            keyboard.Send(VK_INSERT);
            m_LastNonSafeTime = cur_time;
        }

        m_Bot.SetLastEnemy(cur_time);

        int dxchange = static_cast<int>(m_EnemyVelocity.x * (dist / m_DistFactor));
        int dychange = static_cast<int>(m_EnemyVelocity.y * (dist / m_DistFactor));

        if (std::abs(m_EnemyVelocity.x) < 15)
            dx += dxchange;
        if (std::abs(m_EnemyVelocity.y) < 15)
            dy += dychange;

        int target_rot = Util::GetTargetRotation(dx, dy);
        int rot = Util::GetRotation(ship);
        int away = std::abs(rot - target_rot);

        /* Move bot if it's stuck at a wall */
        if (m_MemoryScanning && PointingAtWall(rot, x, y, m_Bot.GetLevel())) {
            m_NearWall += dt;
            if (m_NearWall >= 1000) {
                // Bot has been near a wall for too long
                keyboard.Down(VK_DOWN);

                int dir = Random::GetU32(0, 1) ? VK_LEFT : VK_RIGHT;
                keyboard.Down(dir);

                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                keyboard.Up(dir);
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                keyboard.Down(dir);
                std::this_thread::sleep_for(std::chrono::milliseconds(250));

                keyboard.Up(VK_DOWN);
                keyboard.Up(dir);

                m_NearWall = 0;
            }
        } else {
            m_NearWall = 0;
        }

        keydown = true;
        /* Handle rotation */
        if (rot != -1 && rot != target_rot) {
            int dir = 0;

            if (away < 20 && rot < target_rot) dir = 1;
            if (away < 20 && rot > target_rot) dir = 0;

            if (away > 20 && rot < target_rot) dir = 0;
            if (away > 20 && rot > target_rot) dir = 1;

            if (dir == 0) {
                keyboard.Up(VK_RIGHT);
                keyboard.Down(VK_LEFT);
            } else {
                keyboard.Up(VK_LEFT);
                keyboard.Down(VK_RIGHT);
            }
        } else {
            keyboard.Up(VK_LEFT);
            keyboard.Up(VK_RIGHT);
        }

        /* Handle distance movement */
        if (dist > tardist) {
            keyboard.Up(VK_DOWN);
            keyboard.Down(VK_UP);
        } else {
            keyboard.Up(VK_UP);
            keyboard.Down(VK_DOWN);
        }

        /* Only fire weapons if pointing at enemy*/
        if (rot != target_rot) {
            keyboard.Up(VK_CONTROL);
            return;
        }

        /* Handle bombing */
        if (!insafe && m_FireBombs && timeGetTime() > m_LastBomb + m_BombTime && energypct > m_StopBombing) {
            if (!PointingAtWall(rot, x, y, m_Bot.GetLevel()) && dist >= 7) {
                keyboard.ToggleDown();

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                keyboard.Down(VK_TAB);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                keyboard.Up(VK_TAB);

                keyboard.ToggleDown();

                m_LastBomb = timeGetTime();
            }
        }

        /* Handle gunning */
        if (energypct < m_RunPercent) {
            if (dist <= 7)
                keyboard.Down(VK_CONTROL);
            else
                keyboard.Up(VK_CONTROL);
        } else {
            if (insafe) {
                keyboard.Up(VK_CONTROL);
            } else {
                // Do bullet delay if the closest enemy isn't close, ignore otherwise
                if (dist > 7) {
                    if (timeGetTime() > m_LastBullet + m_CurrentBulletDelay) {
                        keyboard.Down(VK_CONTROL);
                        std::this_thread::sleep_for(std::chrono::milliseconds(30));
                        keyboard.Up(VK_CONTROL);
                        m_LastBullet = timeGetTime();
                    } else {
                        keyboard.Up(VK_CONTROL);
                    }
                } else {
                    keyboard.Down(VK_CONTROL);
                }
            }
        }

    } else {
        /* Clear input when there is no enemy */
        m_LastNonSafeTime = cur_time;
        if (keydown) {
            keyboard.Up(VK_LEFT);
            keyboard.Up(VK_RIGHT);
            keyboard.Up(VK_UP);
            keyboard.Up(VK_DOWN);
            keyboard.Up(VK_CONTROL);
            keydown = false;
        }
        
        if (m_Patrol)
            m_Bot.SetState(std::make_shared<PatrolState>(m_Bot));

        /* Warp to keep the bot in game */
        if (cur_time > m_Bot.GetLastEnemy() + 1000 * 60) {
            m_Bot.SetXRadar(false);

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            keyboard.Send(VK_INSERT);
            keyboard.Send(VK_RIGHT);
            m_Bot.SetLastEnemy(cur_time);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
