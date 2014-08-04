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
    std::weak_ptr<ScreenGrabber> grabber = m_Bot.GetGrabber();
    ScreenAreaPtr& radar = m_Bot.GetRadar();
    ScreenAreaPtr& ship = m_Bot.GetShip();
    ScreenAreaPtr& player = m_Bot.GetPlayer();
    Keyboard& keyboard = m_Bot.GetKeyboard();

    bool insafe = false;

    try {
        player->Find(Colors::SafeColor);
        insafe = true;
    } catch (const std::exception&) {
        insafe = false;
    }

    int tardist = 15;

    static bool keydown = false;

    if (radar->GetWidth() > 300)
        tardist = 30;

    int energy = Util::GetEnergy(m_Bot.GetEnergyAreas());

    try {
        std::vector<Coord> enemies = Util::GetEnemies(radar);

        m_Bot.SetLastEnemy(timeGetTime());

        int radar_center = static_cast<int>(std::ceil(radar->GetWidth() / 2.0));

        int closest_dx = 0;
        int closest_dy = 0;
        double closest_dist = std::numeric_limits<double>::max();

        for (unsigned int i = 0; i < enemies.size(); i++) {
            int dx = enemies.at(i).x - radar_center;
            int dy = enemies.at(i).y - radar_center;

            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist < closest_dist) {
                closest_dist = dist;
                closest_dx = dx;
                closest_dy = dy;
            }
        }

        double angle = std::atan2(closest_dy, closest_dx) * 180 / M_PI;
        int target = static_cast<int>(angle / 9) + 10;
        if (target < 0) target += 40;

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

        if (energy < 600) {
            keyboard.Down(VK_DOWN);
            keyboard.Up(VK_UP);
            keyboard.Up(VK_CONTROL);
        } else {
            if (closest_dist > tardist) {
                keyboard.Up(VK_DOWN);
                keyboard.Down(VK_UP);
            } else {
                keyboard.Up(VK_UP);
                keyboard.Down(VK_DOWN);
            }

            if (!insafe)
                keyboard.Down(VK_CONTROL);
        }

        if (insafe)
            keyboard.Up(VK_CONTROL);

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