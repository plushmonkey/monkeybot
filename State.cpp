#include "State.h"
#include "Keyboard.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Bot.h"
#include <thread>
#include <iostream>

#define RUNENERGY 400
#define XRADARENERGY 600
const Coord NullCoord(0, 0);

State::State(Bot& bot)
    : m_Bot(bot) { }

AggressiveState::AggressiveState(Bot& bot)
    : State(bot), 
      m_LastEnemyPos(0,0),
      m_LastEnemyTimer(0),
      m_EnemyVelocity(0, 0) { }

void AggressiveState::Update() {
    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();
    ScreenAreaPtr& player = m_Bot.GetPlayer();
    Keyboard& keyboard = m_Bot.GetKeyboard();

    bool insafe = Util::PlayerInSafe(player);
    int tardist = 10, dx, dy;
    double dist;
    static bool keydown;

    int energy = Util::GetEnergy(m_Bot.GetEnergyAreas());

    if (energy < RUNENERGY && !insafe) {
        m_Bot.SetState(std::shared_ptr<RunState>(new RunState(m_Bot)));
        return;
    }

    if (energy < XRADARENERGY && Util::XRadarOn(grabber))
        keyboard.Send(VK_END);
    if (energy >= XRADARENERGY && !Util::XRadarOn(grabber))
        keyboard.Send(VK_END);

    try {
        std::vector<Coord> enemies = Util::GetEnemies(radar);

        Coord closest = Util::GetClosestEnemy(enemies, radar, &dx, &dy, &dist);

        if (timeGetTime() > m_LastEnemyTimer + 500) {
            m_EnemyVelocity.x = closest.x - m_LastEnemyPos.x;
            m_EnemyVelocity.y = closest.y - m_LastEnemyPos.y;
            m_LastEnemyTimer = timeGetTime();
            m_LastEnemyPos = closest;
        }

        // ignore velocity if it's probably retargeting
        if (std::abs(m_EnemyVelocity.x) < 15)
            dx += m_EnemyVelocity.x;
        if (std::abs(m_EnemyVelocity.y) < 15)
            dy += m_EnemyVelocity.y;

        m_Bot.SetLastEnemy(timeGetTime());

        int target = Util::GetTargetRotation(dx, dy);
        int rot = Util::GetRotation(ship);

        keydown = true;

        if (rot != target) {
            int away = std::abs(rot - target);
            int dir = 0;

            if (away < 20 && rot < target) dir = 1;
            if (away < 20 && rot > target) dir = 0;

            if (away > 20 && rot < target) dir = 0;
            if (away > 20 && rot > target) dir = 1;

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

        if (dist > tardist) {
            keyboard.Up(VK_DOWN);
            keyboard.Down(VK_UP);
        } else if (dist == tardist) {
            keyboard.Up(VK_UP);
            keyboard.Up(VK_DOWN);
        } else {
            keyboard.Up(VK_UP);
            keyboard.Down(VK_DOWN);
        }

        if (insafe)
            keyboard.Up(VK_CONTROL);
        else
            keyboard.Down(VK_CONTROL);

    } catch (const std::exception&) {
        if (keydown) {
            keyboard.Up(VK_LEFT);
            keyboard.Up(VK_RIGHT);
            keyboard.Up(VK_UP);
            keyboard.Up(VK_DOWN);
            keyboard.Up(VK_CONTROL);
            keydown = false;
        }

        if (timeGetTime() > m_Bot.GetLastEnemy() + 1000 * 60) {
            keyboard.Send(VK_END);
            keyboard.Send(VK_INSERT);
            keyboard.Send(VK_RIGHT);
            m_Bot.SetLastEnemy(timeGetTime());
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
    Keyboard& keyboard = m_Bot.GetKeyboard();
    ScreenGrabberPtr grabber = m_Bot.GetGrabber();

    int energy = Util::GetEnergy(m_Bot.GetEnergyAreas());

    if (Util::XRadarOn(grabber))
        keyboard.Send(VK_END);

    if (energy > RUNENERGY) {
        keyboard.Up(VK_DOWN);
        m_Bot.SetState(std::shared_ptr<AggressiveState>(new AggressiveState(m_Bot)));
        return;
    }

    keyboard.Down(VK_DOWN);
    keyboard.Up(VK_UP);
    keyboard.Up(VK_CONTROL);
}
