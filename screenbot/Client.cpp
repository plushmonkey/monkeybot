#include "Client.h"

#include "Common.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "RotationStore.h"
#include "Level.h"

#include <iostream>
#include <iostream>
#include <thread>

ScreenClient::ScreenClient(HWND hwnd, Config& config)
    : m_Window(hwnd),
      m_Config(config),
      m_LastBomb(0),
      m_LastBullet(0),
      m_CurrentBulletDelay(0),
      m_ConfigLoaded(false),
      m_Rotations(nullptr),
      m_MapZoom(9)
{ 
    m_Screen = std::make_shared<ScreenGrabber>(m_Window);
    m_Ship = m_Screen->GetArea(m_Screen->GetWidth() / 2 - 18, m_Screen->GetHeight() / 2 - 18, 36, 36);

    GrabRadar();

    m_Player = m_Radar->GetArea(m_Radar->GetWidth() / 2 - 1, m_Radar->GetWidth() / 2 - 1, 4, 4);

    int width = m_Screen->GetWidth();

    m_EnergyArea[0] = m_Screen->GetArea(width - 78, 0, 16, 21);
    m_EnergyArea[1] = m_Screen->GetArea(width - 62, 0, 16, 21);
    m_EnergyArea[2] = m_Screen->GetArea(width - 46, 0, 16, 21);
    m_EnergyArea[3] = m_Screen->GetArea(width - 30, 0, 16, 21);
    m_PlayerList.SetScreenArea(m_Screen->GetArea(3, 3, 172, m_Screen->GetHeight() - 50));
}


void ScreenClient::Update(DWORD dt) {
    m_Screen->Update();
    m_Radar->Update();
    m_Ship->Update();
    m_Player->Update();
    //m_PlayerList.Update(dt);

    if (!m_ConfigLoaded) {
        m_BombDelay = m_Config.Get<int>(_T("BombTime"));
        m_BulletDelay = m_Config.Get<int>(_T("BulletDelay")) * 10;
        m_ScaleDelay = m_Config.Get<bool>(_T("ScaleDelay"));
        m_FireBombs = m_Config.Get<bool>(_T("FireBombs"));
        m_FireGuns = m_Config.Get<bool>(_T("FireGuns"));
        m_MapZoom = m_Config.Get<int>("MapZoom");
        m_CurrentBulletDelay = m_BulletDelay;
        m_ConfigLoaded = true;
        m_Rotations = new Ships::RotationStore(m_Config);
    }
}

void ScreenClient::Bomb() {
    if (!m_FireBombs) return;
    if (timeGetTime() < m_LastBomb + m_BombDelay) return;

    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_Keyboard.Down(VK_TAB);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_Keyboard.Up(VK_TAB);
    m_Keyboard.ToggleDown();

    m_LastBomb = timeGetTime();
}

void ScreenClient::Gun(GunState state, int energy_percent) {
    if (!m_FireGuns) state = GunState::Off;

    if (m_ScaleDelay)
        m_CurrentBulletDelay = static_cast<int>(std::ceil(m_BulletDelay * (1.0f + (100.0f - energy_percent) / 100)));

    if (state == GunState::Tap && m_BulletDelay == 0) state = GunState::Constant;

    if (state == GunState::Tap && timeGetTime() < m_LastBullet + m_CurrentBulletDelay) return;

    switch (state) {
        case GunState::Constant:
            m_Keyboard.Down(VK_CONTROL);
        break;
        case GunState::Off:
            m_Keyboard.Up(VK_CONTROL);
            return;
        case GunState::Tap:
            m_Keyboard.Down(VK_CONTROL);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            m_Keyboard.Up(VK_CONTROL);
        break;
    }
    
    m_LastBullet = timeGetTime();
}

void ScreenClient::SetXRadar(bool on) {
    int count = 0;
    /* Try to toggle xradar, timeout after 50 tries */
    while (Util::XRadarOn(m_Screen) != on && count < 50) {
        m_Keyboard.Down(VK_END);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        m_Keyboard.Up(VK_END);
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        m_Screen->Update();
    }
}

void ScreenClient::Warp() {
    SetXRadar(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_Keyboard.Send(VK_INSERT);
}


void ScreenClient::Up(bool val) {
    val ? m_Keyboard.Down(VK_UP) : m_Keyboard.Up(VK_UP);
}

void ScreenClient::Down(bool val) {
    val ? m_Keyboard.Down(VK_DOWN) : m_Keyboard.Up(VK_DOWN);
}

void ScreenClient::Left(bool val) {
    val ? m_Keyboard.Down(VK_LEFT) : m_Keyboard.Up(VK_LEFT);
}

void ScreenClient::Right(bool val) {
    val ? m_Keyboard.Down(VK_RIGHT) : m_Keyboard.Up(VK_RIGHT);
}

void ScreenClient::Attach() {
    m_Keyboard.Send(VK_F7);
}

bool ScreenClient::InShip() const {
    return Util::InShip(m_Screen);
}

void ScreenClient::EnterShip(int num) {
    m_Keyboard.Up(VK_CONTROL);
    m_Keyboard.Send(VK_ESCAPE);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_Keyboard.Send(0x30 + num);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

std::vector<Vec2> ScreenClient::GetEnemies(Vec2 real_pos, const Level& level) {
    std::vector<Vec2> enemies;

    for (int y = 0; y < m_Radar->GetWidth(); y++) {
        for (int x = 0; x < m_Radar->GetWidth(); x++) {
            Pixel pix = m_Radar->GetPixel(x, y);
            if (pix == Colors::EnemyColor[0] || pix == Colors::EnemyColor[1] || pix == Colors::EnemyBallColor) {
                int count = 0;
                try {
                    Pixel pixel;

                    // right
                    pixel = m_Radar->GetPixel(x + 1, y);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom-right
                    pixel = m_Radar->GetPixel(x + 1, y + 1);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                    // bottom
                    pixel = m_Radar->GetPixel(x, y + 1);
                    if (pixel == Colors::EnemyColor[0] || pixel == Colors::EnemyColor[1] || pixel == Colors::EnemyBallColor)
                        count++;
                } catch (std::exception&) {}

                if (count >= 3) {
                    Vec2 coord(static_cast<float>(x), static_cast<float>(y));
                    if (!Util::InSafe(m_Radar, coord))
                        enemies.push_back(this->GetRealPosition(real_pos, coord, level));
                }
            }
        }
    }

    if (enemies.size() == 0)
        throw std::runtime_error("No enemies near.");
    return enemies;
}

Vec2 ScreenClient::GetClosestEnemy(Vec2 real_pos, const Level& level, int* dx, int* dy, double* dist) {
    std::vector<Vec2> enemies = GetEnemies(real_pos, level);
    *dist = std::numeric_limits<double>::max();
    Vec2 closest = enemies.at(0);

    for (unsigned int i = 0; i < enemies.size(); i++) {
        int cdx, cdy;
        double cdist;

        Util::GetDistance(enemies.at(i), real_pos, &cdx, &cdy, &cdist);

        if (cdist < *dist) {
            *dist = cdist;
            *dx = cdx;
            *dy = cdy;
            closest = enemies.at(i);
        }
    }

    return closest;
}

Vec2 ScreenClient::GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level) {
    return Util::FindTargetPos(bot_pos, target, m_Screen, m_Radar, level, m_MapZoom);
}

int ScreenClient::GetEnergy()  {
    return Util::GetEnergy(m_EnergyArea);
}

int ScreenClient::GetRotation() {
    u64 val = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++)
            val = m_Ship->GetPixel(16 + j, 16 + i) + val;
    }

    if (m_Rotations)
        return m_Rotations->GetRotation(val);

    return -1;
}

bool ScreenClient::InSafe(Vec2 real_pos, const Level& level) {
    return level.GetTileID(static_cast<int>(real_pos.x), static_cast<int>(real_pos.y)) == 171;
}

void ScreenClient::ReleaseKeys() {
    m_Keyboard.ReleaseAll();
}

void ScreenClient::ToggleKeys() {
    m_Keyboard.ToggleDown();
}

void ScreenClient::GrabRadar() {
    int radarstart = 0;
    int radarend = 0;
    int radary = 0;

    while (radarend == 0) {
        m_Screen->Update();
        for (int y = static_cast<int>(m_Screen->GetHeight() / 1.5); y < m_Screen->GetHeight(); y++) {
            for (int x = static_cast<int>(m_Screen->GetWidth() / 1.5); x < m_Screen->GetWidth(); x++) {
                Pixel pix = m_Screen->GetPixel(x, y);

                if (radarstart == 0 && pix == Colors::RadarBorder)
                    radarstart = x;

                if (radarstart != 0 && pix != Colors::RadarBorder) {
                    radarend = x - 1;
                    if (radarend - radarstart < 104) {
                        radarstart = 0;
                        radarend = 0;
                        break;
                    }
                    radarstart++;
                    radary = y + 1;
                    x = m_Screen->GetWidth();
                    y = m_Screen->GetHeight();
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        tcout << "Finding radar location." << std::endl;
    }

    int radarwidth = radarend - radarstart;

    tcout << "Creating radar screen with width of " << radarwidth << " at " << radarstart << ", " << radary << "." << std::endl;

    m_Radar = m_Screen->GetArea(radarstart, radary, radarwidth, radarwidth);

    if (!m_Radar.get()) {
        tcerr << "Resolution (" << m_Screen->GetWidth() << "x" << m_Screen->GetHeight() << ") not supported." << std::endl;
        std::abort();
    }
}