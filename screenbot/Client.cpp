#include "Client.h"
#include "Common.h"
#include <iostream>
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include <thread>

ScreenClient::ScreenClient(HWND hwnd, Config& config)
    : m_Window(hwnd),
      m_Config(config),
      m_LastBomb(0),
      m_LastBullet(0),
      m_CurrentBulletDelay(0),
      m_ConfigLoaded(false)
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
}


void ScreenClient::Update(DWORD dt) {
    m_Screen->Update();
    m_Radar->Update();
    m_Ship->Update();
    m_Player->Update();

    if (!m_ConfigLoaded) {
        m_BombDelay = m_Config.Get<int>(_T("BombTime"));
        m_BulletDelay = m_Config.Get<int>(_T("BulletDelay")) * 10;
        m_ScaleDelay = m_Config.Get<bool>(_T("ScaleDelay"));
        m_FireBombs = m_Config.Get<bool>(_T("FireBombs"));
        m_FireGuns = m_Config.Get<bool>(_T("FireGuns"));
        m_CurrentBulletDelay = m_BulletDelay;
        m_ConfigLoaded = true;
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

    if (state == GunState::Constant) m_CurrentBulletDelay = 0;

    if (state != GunState::Off && timeGetTime() < m_LastBullet + m_CurrentBulletDelay) return;

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

std::vector<Coord> ScreenClient::GetEnemies() {
    return Util::GetEnemies(m_Radar);
}

Coord ScreenClient::GetClosestEnemy(int* dx, int* dy, double* dist) {
    std::vector<Coord> enemies = GetEnemies();
    return Util::GetClosestEnemy(enemies, m_Radar, dx, dy, dist);
}

Coord ScreenClient::GetRealPosition(Coord bot_pos, Coord target, const Level& level) {
    return Util::FindTargetPos(bot_pos, target, m_Screen, m_Radar, level);
}

int ScreenClient::GetEnergy()  {
    return Util::GetEnergy(m_EnergyArea);
}

int ScreenClient::GetRotation() {
    return Util::GetRotation(m_Ship);
}

bool ScreenClient::InSafe() {
    return Util::PlayerInSafe(m_Player);
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