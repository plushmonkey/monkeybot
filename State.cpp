#include "State.h"
#include "Keyboard.h"
#include "ScreenArea.h"
#include "Util.h"
#include "Bot.h"
#include <thread>
#include <iostream>

State::State(Bot& bot)
    : m_Bot(bot) { }

AggressiveState::AggressiveState(Bot& bot)
    : State(bot) { }

void AggressiveState::Update() {
    std::shared_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();
    ScreenAreaPtr& player = m_Bot.GetPlayer();
    Keyboard& keyboard = m_Bot.GetKeyboard();

    bool insafe = Util::PlayerInSafe(player);
    int tardist = 15, dx, dy;
    double dist;
    static bool keydown;

    int energy = Util::GetEnergy(m_Bot.GetEnergyAreas());

    if (energy < 600 && !insafe) {
        tcout << "Switching to run state. Current energy: " << energy << std::endl;
        m_Bot.SetState(std::shared_ptr<RunState>(new RunState(m_Bot)));
        return;
    }

    try {
        std::vector<Coord> enemies = Util::GetEnemies(radar);

        Util::GetClosestEnemy(enemies, radar, &dx, &dy, &dist);
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
            keyboard.Send(VK_INSERT);
            keyboard.Send(VK_RIGHT);
            m_Bot.SetLastEnemy(timeGetTime());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

RunState::RunState(Bot& bot) : State(bot) { }

void RunState::Update() {
    Keyboard& keyboard = m_Bot.GetKeyboard();

    int energy = Util::GetEnergy(m_Bot.GetEnergyAreas());

    if (energy > 600) {
        tcout << "Switching to aggressive state. Current energy: " << energy << std::endl;
        keyboard.Up(VK_DOWN);
        m_Bot.SetState(std::shared_ptr<AggressiveState>(new AggressiveState(m_Bot)));
        return;
    }

    keyboard.Down(VK_DOWN);
    keyboard.Up(VK_UP);
    keyboard.Up(VK_CONTROL);
}
