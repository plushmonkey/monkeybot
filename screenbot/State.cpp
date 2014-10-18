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

const Coord NullCoord(0, 0);

MemoryState::MemoryState(Bot& bot) : State(bot) {

}

void MemoryState::Update() {
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
        std::vector<unsigned int> erase_list;

        // Loop through the found values and remove ones that don't match the movement
        for (auto& kv : m_FindSpace) {
            // Get the new value at this address
            unsigned int check = Memory::GetU32(m_Bot.GetProcessHandle(), kv.first);

            // Remove the address from the list if it doesn't match the movement
            if (check == kv.second)
                erase_list.push_back(kv.first);

            // Set the new value of the address
            m_FindSpace[kv.first] = check;
        }

        for (unsigned int i = 0; i < erase_list.size(); ++i)
            m_FindSpace.erase(erase_list[i]);
    }

    tcout << "Find space size: " << m_FindSpace.size() << std::endl;

    if (m_FindSpace.size() <= 5) {
        auto iter = m_FindSpace.begin();
        ++iter;
        m_Bot.SetPosAddress(iter->first - 4);
        
        //m_Bot.SetState(std::shared_ptr<AggressiveState>(new AggressiveState(m_Bot)));
        m_Bot.SetState(std::shared_ptr<FollowState>(new FollowState(m_Bot)));
    }
}


State::State(Bot& bot)
    : m_Bot(bot) { }

FollowState::FollowState(Bot& bot) : State(bot) { }

void FollowState::Update() {
    Keyboard& keyboard = m_Bot.GetKeyboard();

    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();

    std::vector<Coord> path = m_Bot.GetPath();

    if (path.size() > 0) {
        Coord next = path.at(path.size() - 1);
        Coord pos(m_Bot.GetX(), m_Bot.GetY());
        int dx, dy;
        double dist;

        Util::GetDistance(pos, next, &dx, &dy, &dist);

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

        keyboard.Up(VK_UP);
    }

}

AggressiveState::AggressiveState(Bot& bot)
    : State(bot), 
      m_LastEnemyPos(0,0),
      m_LastEnemyTimer(0),
      m_EnemyVelocity(0, 0),
      m_LastBomb(0),
      m_LastNonSafeTime(0),
      m_LastBullet(0) 
{
    
}

void AggressiveState::Update() {
    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();
    ScreenAreaPtr& player = m_Bot.GetPlayer();
    Keyboard& keyboard = m_Bot.GetKeyboard();

    const int runpercent        = m_Bot.GetConfig().Get<int>(_T("RunPercent"));
    const int xpercent          = m_Bot.GetConfig().Get<int>(_T("XPercent"));
    const int saferesettime     = m_Bot.GetConfig().Get<int>(_T("SafeResetTime"));
    const int targetdist        = m_Bot.GetConfig().Get<int>(_T("TargetDistance"));
    const int rundist           = m_Bot.GetConfig().Get<int>(_T("RunDistance"));
    const int stopbombing       = m_Bot.GetConfig().Get<int>(_T("StopBombing"));
    const int bombtime          = m_Bot.GetConfig().Get<int>(_T("BombTime"));
    const bool firebombs        = m_Bot.GetConfig().Get<bool>(_T("FireBombs"));
    const bool fireguns         = m_Bot.GetConfig().Get<bool>(_T("FireGuns"));
    int distfactor              = m_Bot.GetConfig().Get<int>(_T("DistanceFactor"));
    int bulletdelay             = m_Bot.GetConfig().Get<int>(_T("BulletDelay")) * 10;
    const bool scaledelay       = m_Bot.GetConfig().Get<bool>(_T("ScaleDelay"));

    if (distfactor < 1) distfactor = 10;

    bool insafe = Util::PlayerInSafe(player);
    int tardist = targetdist;
    static bool keydown;

    int energypct = m_Bot.GetEnergyPercent();

    if (energypct < runpercent && !insafe) {
        tardist = rundist;
        keyboard.ReleaseAll();
    }

    if (scaledelay)
        m_CurrentBulletDelay = static_cast<int>(std::ceil(bulletdelay * (1.0f + (100.0f - energypct) / 100)));
    else
        m_CurrentBulletDelay = bulletdelay;

    /* Turn off x if energy low, turn back on when high */
    m_Bot.SetXRadar(energypct > xpercent);

    Coord target = m_Bot.GetEnemyTarget();

    DWORD cur_time = timeGetTime();

    /* Only update if there is a target enemy */
    if (target != NullCoord) {
        TargetInfo info = m_Bot.GetEnemyTargetInfo();
        double dist = info.dist;
        int dx = info.dx;
        int dy = info.dy;
        
        if (cur_time > m_LastEnemyTimer + 500) {
            m_EnemyVelocity.x = target.x - m_LastEnemyPos.x;
            m_EnemyVelocity.y = target.y - m_LastEnemyPos.y;
            m_LastEnemyTimer = cur_time;
            m_LastEnemyPos = target;
        }

        if (!insafe)
            m_LastNonSafeTime = cur_time;

        /* Handle trying to get out of safe */
        if (saferesettime > 0 && insafe && cur_time >= m_LastNonSafeTime + saferesettime) {
            m_Bot.SetXRadar(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            keyboard.Send(VK_INSERT);
            m_LastNonSafeTime = cur_time;
        }

        m_Bot.SetLastEnemy(cur_time);

        int dxchange = static_cast<int>(m_EnemyVelocity.x * (dist / distfactor));
        int dychange = static_cast<int>(m_EnemyVelocity.y * (dist / distfactor));

        if (std::abs(m_EnemyVelocity.x) < 15)
            dx += dxchange;
        if (std::abs(m_EnemyVelocity.y) < 15)
            dy += dychange;

        TargetInfo move_info = m_Bot.GetTargetInfo();

        int target_rot = Util::GetTargetRotation(dx, dy);
        int rot = Util::GetRotation(ship);
        int away = std::abs(rot - target_rot);

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
        if (!insafe && firebombs && timeGetTime() > m_LastBomb + bombtime && energypct > stopbombing) {
            keyboard.ToggleDown();

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            keyboard.Down(VK_TAB);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            keyboard.Up(VK_TAB);

            keyboard.ToggleDown();

            m_LastBomb = timeGetTime();
        }

        /* Handle gunning */
        if (energypct < runpercent) {
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

RunState::RunState(Bot& bot) : State(bot) {
    Keyboard& keyboard = m_Bot.GetKeyboard();
    keyboard.Up(VK_LEFT);
    keyboard.Up(VK_RIGHT);
    keyboard.Up(VK_UP);
    keyboard.Up(VK_DOWN);
    keyboard.Up(VK_CONTROL);
}

void RunState::Update() {
    /*Keyboard& keyboard = m_Bot.GetKeyboard();
    ScreenGrabberPtr grabber = m_Bot.GetGrabber();

    int energy = m_Bot.GetEnergy();
    int energypct = m_Bot.GetEnergyPercent();

    if (Util::XRadarOn(grabber))
        keyboard.Send(VK_END);

    if (energypct > RUNPERCENT) {
        keyboard.Up(VK_DOWN);
        m_Bot.SetState(std::shared_ptr<AggressiveState>(new AggressiveState(m_Bot)));
        return;
    }

    keyboard.Down(VK_DOWN);
    keyboard.Up(VK_UP);
    keyboard.Up(VK_CONTROL);*/
}
