#include "State.h"
#include "Keyboard.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Bot.h"
#include <thread>
#include <iostream>
#include <tchar.h>

const Coord NullCoord(0, 0);

State::State(Bot& bot)
    : m_Bot(bot) { }

AggressiveState::AggressiveState(Bot& bot)
    : State(bot), 
      m_LastEnemyPos(0,0),
      m_LastEnemyTimer(0),
      m_EnemyVelocity(0, 0),
      m_LastBomb(0),
      m_LastNonSafeTime(0) { }

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

    if (distfactor < 1) distfactor = 10;

    bool insafe = Util::PlayerInSafe(player);
    int tardist = targetdist;
    static bool keydown;

    int energypct = m_Bot.GetEnergyPercent();

    if (energypct < runpercent && !insafe) {
        tardist = rundist;
        keyboard.ReleaseAll();
    }

    bool xon = Util::XRadarOn(grabber);

    if (energypct < xpercent && xon)
        keyboard.Send(VK_END);

    if (energypct > xpercent && !xon)
        keyboard.Send(VK_END);

    Coord target = m_Bot.GetEnemyTarget();

    DWORD cur_time = timeGetTime();

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

        if (saferesettime > 0 && insafe && cur_time >= m_LastNonSafeTime + saferesettime) {
            if (xon)
                keyboard.Send(VK_END);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
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

        if (firebombs && cur_time > m_LastBomb + bombtime && energypct >= stopbombing) {
            keyboard.ToggleDown();
            keyboard.Up(VK_CONTROL);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            keyboard.Down(VK_TAB);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            keyboard.Up(VK_TAB);
            m_LastBomb = cur_time;

            keyboard.ToggleDown();
        }

        if (dist > tardist) {
            keyboard.Up(VK_DOWN);
            keyboard.Down(VK_UP);
        } else {
            keyboard.Up(VK_UP);
            keyboard.Down(VK_DOWN);
        }

        if (energypct < runpercent) {
            keyboard.Up(VK_CONTROL);
        } else {
            if (insafe) {
                keyboard.Up(VK_CONTROL);
            } else {
                if (fireguns)
                    keyboard.Down(VK_CONTROL);
            }
        }
    } else {
        m_LastNonSafeTime = cur_time;
        if (keydown) {
            keyboard.Up(VK_LEFT);
            keyboard.Up(VK_RIGHT);
            keyboard.Up(VK_UP);
            keyboard.Up(VK_DOWN);
            keyboard.Up(VK_CONTROL);
            keydown = false;
        }

        if (cur_time > m_Bot.GetLastEnemy() + 1000 * 60) {
            if (xon)
                keyboard.Send(VK_END);

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
