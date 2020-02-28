#include "Client.h"

#include "Common.h"
#include "ScreenArea.h"
#include "ScreenGrabber.h"
#include "Util.h"
#include "Level.h"
#include "Memory.h"
#include "MemorySensor.h"

#include <iostream>
#include <algorithm>
#include <thread>

namespace {

const std::size_t MaxChatLength = 200;

} // ns

ScreenClient::ScreenClient(HWND hwnd, Config& config, Memory::MemorySensor& memsensor)
    : m_Window(hwnd),
      m_Config(config),
      m_LastBomb(0),
      m_LastBullet(0),
      m_EmpEnd(0),
      m_Thrusting(false),
      m_MemorySensor(memsensor),
      m_CurrentBulletDelay(0),
      m_MultiState(MultiState::None),
      m_Multi(true)
{ 
    m_Screen = std::make_shared<ScreenGrabber>(m_Window);
    m_Ship = m_Screen->GetArea(m_Screen->GetWidth() / 2 - 18, m_Screen->GetHeight() / 2 - 18, 36, 36);

    GrabRadar();

    m_Player = m_Radar->GetArea(m_Radar->GetWidth() / 2 - 1, m_Radar->GetWidth() / 2 - 1, 4, 4);

    int width = m_Screen->GetWidth();

    m_EnergyArea[0] = m_Screen->GetArea(width - 94, 0, 16, 21);
    m_EnergyArea[1] = m_Screen->GetArea(width - 78, 0, 16, 21);
    m_EnergyArea[2] = m_Screen->GetArea(width - 62, 0, 16, 21);
    m_EnergyArea[3] = m_Screen->GetArea(width - 46, 0, 16, 21);
    m_EnergyArea[4] = m_Screen->GetArea(width - 30, 0, 16, 21);
    m_PlayerWindow.SetScreenArea(m_Screen->GetArea(3, 3, 172, m_Screen->GetHeight() - 50));

    m_PixelHandlers[Pixel(255, 255, 255, 0)] = std::bind(&ScreenClient::EMPPixelHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void ScreenClient::Update(DWORD dt) {
    m_Screen->Update();
    m_Radar->Update();
    m_Ship->Update();
    m_Player->Update();
    m_PlayerWindow.Update(dt);

    m_MultiState = DetermineMultiState();

    if ((m_Multi && m_MultiState == MultiState::Off) ||
        (!m_Multi && m_MultiState == MultiState::On))
    {
        EnableMulti(m_Multi);
    }

    //Scan();
}

MultiState ScreenClient::DetermineMultiState() const {
    const int Width = m_Screen->GetWidth();
    const int Height = m_Screen->GetHeight();
    const int GunIndicator = Util::GetIndicatorTop(Util::Indicator::Gun, Height);
    const int GunPixelY = 6;

    int x = Width - 18;
    int y = GunIndicator + GunPixelY;

    Pixel pixel = m_Screen->GetPixel(x, y);

    if (pixel == Colors::MultiNone) return MultiState::None;
    if (pixel == Colors::MultiOff) return MultiState::Off;
    return MultiState::On;
}

bool ScreenClient::HasBouncingBullets() const {
    const int GunIndicator = Util::GetIndicatorTop(Util::Indicator::Gun, m_Screen->GetHeight());
    const int ScreenWidth = m_Screen->GetWidth();

    int x = ScreenWidth - 18;
    int y = GunIndicator + 11;

    return m_Screen->GetPixel(x, y) == Colors::BulletBounceColor;
}


int ScreenClient::GetFreq() {
    return m_MemorySensor.GetFrequency();
}

api::PlayerList ScreenClient::GetFreqPlayers(int freq) {
    auto players = m_MemorySensor.GetPlayers();
   api::PlayerList team;

    for (auto p : players) {
        if (freq == p->GetFreq())
            team.push_back(p);
    }
    return team;
}

api::PlayerList ScreenClient::GetPlayers() {
    return m_MemorySensor.GetPlayers();
}

bool ScreenClient::OnSoloFreq() {
    int freq = GetFreq();
    return GetFreqPlayers(freq).size() <= 1;
}

api::PlayerPtr ScreenClient::GetSelectedPlayer() {
    return m_PlayerWindow.GetSelectedPlayer();
}

void ScreenClient::MoveTicker(Direction dir) {
    if (dir != Direction::Up && dir != Direction::Down) return;

    int key = dir == Direction::Down ? VK_NEXT : VK_PRIOR;

    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_Keyboard.Send(key);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void ScreenClient::EnableMulti(bool enable) {
    if (m_MultiState == MultiState::None) return;

    if ((enable && m_MultiState == MultiState::Off) ||
        (!enable && m_MultiState == MultiState::On))
    {
        m_Keyboard.ToggleDown();

        int count = 0;
        bool set = false;
        while (!set && count < 50) {
            m_Keyboard.Down(VK_DELETE);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            m_Keyboard.Up(VK_DELETE);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            m_Screen->Update();

            m_MultiState = DetermineMultiState();

            set = (enable && m_MultiState == MultiState::On) || (!enable && m_MultiState == MultiState::Off);
            ++count;
        }

        m_Keyboard.ToggleDown();
    }

    m_Multi = enable;
}

void ScreenClient::Bomb() {
    if (!m_Config.FireBombs) return;
    if (Emped()) {
        m_Keyboard.Up(VK_TAB);
        return;
    }
    if (m_Config.BombDelay > 0 && (timeGetTime() < m_LastBomb + m_Config.BombDelay * 10)) return;

    if (m_Thrusting)
        SetThrust(false);

    if (m_Config.BombDelay == 0) {
        m_Keyboard.Down(VK_TAB);
    } else {
        m_Keyboard.ToggleDown();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_Keyboard.Down(VK_TAB);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_Keyboard.Up(VK_TAB);
        m_Keyboard.ToggleDown();
        m_LastBomb = timeGetTime();
    }
}

void ScreenClient::Gun(GunState state, int energy_percent) {
    if (!m_Config.FireGuns) state = GunState::Off;

    if (Emped()) {
        m_Keyboard.Up(VK_CONTROL);
        return;
    }

    if (m_Config.ScaleDelay)
        m_CurrentBulletDelay = static_cast<int>(std::ceil(m_Config.BulletDelay * 10 * (1.0f + (100.0f - energy_percent) / 100)));

    if (state == GunState::Tap && m_Config.BulletDelay == 0) state = GunState::Constant;
    if (state == GunState::Tap && timeGetTime() < m_LastBullet + m_CurrentBulletDelay) return;

    if (m_Thrusting)
        SetThrust(false);

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

void ScreenClient::Burst() {
    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    m_Keyboard.Down(VK_SHIFT);
    std::this_thread::sleep_for(std::chrono::milliseconds(105));
    m_Keyboard.Send(VK_DELETE);
    m_Keyboard.Up(VK_SHIFT);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    m_Keyboard.ToggleDown();
}

void ScreenClient::Repel() {
    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    m_Keyboard.Down(VK_SHIFT);
    std::this_thread::sleep_for(std::chrono::milliseconds(105));
    m_Keyboard.Send(VK_CONTROL);
    m_Keyboard.Up(VK_SHIFT);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    m_Keyboard.ToggleDown();
}

void ScreenClient::Rocket() {
    m_Keyboard.ToggleDown();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    m_Keyboard.Send(VK_F3);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    m_Keyboard.ToggleDown();
}

void ScreenClient::Decoy() {
	m_Keyboard.ToggleDown();
	std::this_thread::sleep_for(std::chrono::milliseconds(30));

    if (m_Thrusting)
        SetThrust(false);

	m_Keyboard.Send(VK_F5);

	m_Keyboard.ToggleDown();
}

void ScreenClient::SetThrust(bool on) {
#ifdef NO_THRUSTING
    m_Thrusting = false;
    return;
#endif

    m_Thrusting = on;

    if (on)
        m_Keyboard.Down(VK_SHIFT);
    else
        m_Keyboard.Up(VK_SHIFT);
}

void ScreenClient::SetXRadar(bool on) {
    /* Try to toggle xradar, timeout after 50 tries */
    try {
        int count = 0;
        while (Util::XRadarOn(m_Screen) != on && count < 50) {
            if (m_Thrusting)
                SetThrust(false);

            m_Keyboard.Down(VK_END);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            m_Keyboard.Up(VK_END);
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            m_Screen->Update();
        }
    } catch (...) {}
}

void ScreenClient::Warp() {
    SetXRadar(false);
    if (m_Thrusting)
        SetThrust(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
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
    if (m_Thrusting)
        SetThrust(false);

    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    m_Keyboard.Send(VK_F7);
}

bool ScreenClient::InShip() const {
    return Util::InShip(m_Screen);
}

void ScreenClient::EnterShip(int num) {
    m_Keyboard.ToggleDown();
    m_Keyboard.Up(VK_SHIFT);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_Keyboard.Send(VK_ESCAPE);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (num == 9)
        m_Keyboard.Send('s');
    else 
        m_Keyboard.Send(0x30 + num);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

}
void ScreenClient::Spec() {
    m_Keyboard.ReleaseAll();

    m_Keyboard.Send(VK_ESCAPE);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    short code = VkKeyScan('s');
    m_Keyboard.Down(code);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    m_Keyboard.Up(code);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

std::vector<api::PlayerPtr> ScreenClient::GetEnemies() {
    std::vector<api::PlayerPtr> enemies;
   api::PlayerList players = m_MemorySensor.GetPlayers();
    for (auto p : players) {
        if (p->GetFreq() != GetFreq() &&
            p->GetShip() != api::Ship::Spectator &&
            p->GetName().at(0) != '<') 
        {
            Vec2 pos = p->GetPosition() / 16;

            if (m_Config.CenterOnly && m_Config.Hyperspace) {
                if (pos.x < 320 || pos.x >= 703 || pos.y < 320 || pos.y >= 703)
                    continue;
            }

            enemies.push_back(p);
        }
    }

    return enemies;
}

void ScreenClient::SetTarget(const std::string& name) {
    m_Target = name;
    std::transform(m_Target.begin(), m_Target.end(), m_Target.begin(), tolower);
}

void ScreenClient::SetPriorityTarget(const std::string& name) {
    m_PriorityTarget = name;
    std::transform(m_PriorityTarget.begin(), m_PriorityTarget.end(), m_PriorityTarget.begin(), tolower);
}

api::PlayerPtr ScreenClient::GetClosestEnemy(Vec2 real_pos, Vec2 heading, const Level& level, int* dx, int* dy, double* dist) {
    std::vector<api::PlayerPtr> enemies = GetEnemies(); // Grab all of the players visible on radar. Returns position in world space

    if (enemies.size() == 0) return api::PlayerPtr(nullptr);

    *dist = std::numeric_limits<double>::max(); // Distance of closest enemy
    double closest_calc_dist = std::numeric_limits<double>::max(); // Distance of closest enemy with multiplier applied
    api::PlayerPtr& closest = enemies.at(0); // The closest enemy
    const double RotationMultiplier = 2.5; // Determines how much the rotation difference will increase distance by
    bool using_target = false;

    bool in_safe = IsInSafe(real_pos, level);
    

    for (unsigned int i = 0; i < enemies.size(); i++) {
        api::PlayerPtr& enemy = enemies.at(i);
        int cdx, cdy;
        double cdist;

        Vec2 pos = enemy->GetPosition() / 16;

        if (pos.x <= 0 && pos.y <= 0) continue;

        Util::GetDistance(pos, real_pos, &cdx, &cdy, &cdist);

        if (cdist < 15 && in_safe) continue;

        Vec2 to_target = Vec2Normalize(pos - real_pos); // Unit vector pointing towards this enemy
        double dot = heading.Dot(to_target);
        double multiplier = 1.0 + ((1.0 - dot) / RotationMultiplier);
        double calc_dist = cdist * multiplier;

        std::string enemy_name = enemy->GetName();

        std::transform(enemy_name.begin(), enemy_name.end(), enemy_name.begin(), tolower);
        
        if (enemy_name.compare(m_PriorityTarget) == 0) {
            *dist = cdist;
            *dx = cdx;
            *dy = cdy;
            closest = enemy;
            break;
        }

        bool is_target = enemy_name.compare(m_Target) == 0;

        if ((calc_dist < closest_calc_dist || is_target) && !using_target) {
            closest_calc_dist = calc_dist;
            *dist = cdist;
            *dx = cdx;
            *dy = cdy;
            closest = enemy;

            if (is_target) using_target = true;
        }
    }

    return closest;
}

Vec2 ScreenClient::GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level) {
    return Util::FindTargetPos(bot_pos, target, m_Screen, m_Radar, level, 9);
}

int ScreenClient::GetEnergy(api::Ship current_ship)  {
    const ShipSettings& settings = m_MemorySensor.GetShipSettings(current_ship);
    int max_energy = settings.MaximumEnergy;
    int digits = 4;

    if (max_energy >= 10000)
        digits = 5;

    return Util::GetEnergy(m_EnergyArea, digits);
}

int ScreenClient::GetRotation() {
    return m_MemorySensor.GetBotPlayer()->GetRotation();
}

bool ScreenClient::IsInSafe(Vec2 real_pos, const Level& level) const {
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

        if (radarend == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            tcout << "Finding radar location. (Screen dimensions: " << m_Screen->GetWidth() << "x" << m_Screen->GetHeight() << ")" << std::endl;
            tcout << "Saving screen to screen.bmp" << std::endl;
            m_Screen->Save("screen.bmp");
        }
    }

    int radarwidth = radarend - radarstart;

    tcout << "Creating radar screen with width of " << radarwidth << " at " << radarstart << ", " << radary << "." << std::endl;

    m_Radar = m_Screen->GetArea(radarstart, radary, radarwidth, radarwidth);

    if (!m_Radar.get()) {
        tcerr << "Resolution (" << m_Screen->GetWidth() << "x" << m_Screen->GetHeight() << ") not supported." << std::endl;
        std::abort();
    }
}

bool ScreenClient::Emped() {
    return m_EmpEnd > timeGetTime();
}

bool SetClipboard(const std::string& str) {
    if (OpenClipboard(NULL)) {
        // memory becomes owned by system so don't clean it up
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, str.length() + 1);

        if (!mem) {
            CloseClipboard();
            return false;
        }

        memcpy(GlobalLock(mem), str.c_str(), str.length());
        GlobalUnlock(mem);

        EmptyClipboard();
        SetClipboardData(CF_TEXT, mem);

        CloseClipboard();
        return true;
    }

    return false;
}

// Sends a string by pasting. Limits each paste to 200. Calls itself with the remaining string to paste again.
void ScreenClient::SendString(const std::string& str, bool paste) {
    using namespace std;

    lock_guard<mutex> lock(m_ChatMutex);

    std::string to_send = str.substr(0, min(MaxChatLength, str.length()));

    m_Keyboard.ReleaseAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    
    int maxEnergy = this->GetEnergy(m_MemorySensor.GetBotPlayer()->GetShip());
    // Clear anything that's in the chat already
    {
        m_Keyboard.Send(VK_SPACE);
        m_Keyboard.Down(VK_LCONTROL);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Down(VK_BACK);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Up(VK_BACK);
        m_Keyboard.Up(VK_LCONTROL);

        if (!paste) {
            int energy = 0;

            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                this->Update(100);
                energy = this->GetEnergy(m_MemorySensor.GetBotPlayer()->GetShip());
            } while (energy != maxEnergy);
        }
    }

    if (paste && SetClipboard(to_send)) {
        m_Keyboard.Down(VK_SPACE); 
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Up(VK_SPACE);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        m_Keyboard.Down(VK_LCONTROL);
        short code = VkKeyScan('v');
        m_Keyboard.Down(code);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Up(code);
        m_Keyboard.Up(VK_LCONTROL);
    } else {
        for (char c : to_send) {
            short code = VkKeyScan(c);
            u8 state = (code >> 8) & 0xFF;
            bool shift = state & 1;

            if (shift)
                m_Keyboard.Down(VK_LSHIFT);
            else
                m_Keyboard.Up(VK_LSHIFT);

            m_Keyboard.Down(code);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            m_Keyboard.Up(code);
            if (shift)
                m_Keyboard.Up(VK_LSHIFT);
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    m_Keyboard.Send(VK_RETURN);

    if (str.length() > MaxChatLength)
        SendString(str.substr(MaxChatLength));
}

void ScreenClient::SendPM(const std::string& target, const std::string& mesg) {
    std::string priv = ":" + target + ":";
    std::string str = priv + mesg;

    while (str.length() != 0) {
        std::size_t send_size = std::min(MaxChatLength, str.length());
        std::string to_send = str.substr(0, send_size);
        
        if (str.length() > MaxChatLength)
            str = priv + str.substr(send_size);
        else
            str.clear();

        SendString(to_send);
    }
}

void ScreenClient::UseMacro(short num) {
    if (m_Thrusting)
        SetThrust(false);

    m_Keyboard.Send(VK_F1 + num - 1);
}

void ScreenClient::EMPPixelHandler(ScreenGrabberPtr screen, int x, int y) {
    static const std::vector<Vec2> dirs = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    const Pixel emp_color(148, 148, 214, 0);

    // Only check near the ship
    if (std::abs(x - (screen->GetWidth() / 2)) > 100) return;
    if (std::abs(y - (screen->GetHeight() / 2)) > 100) return;

    for (const auto& dir : dirs) {
        if (x + dir.x < 0 || x + dir.x > screen->GetWidth() - 1) continue;
        if (y + dir.y < 0 || y + dir.y > screen->GetHeight() - 1) continue;
        Pixel pixel = screen->GetPixel(int(x + dir.x), int(y + dir.y));

        if (pixel != emp_color)
            return;
    }

    m_EmpEnd = timeGetTime() + 100;
}

void ScreenClient::Scan() {
    const int width = m_Screen->GetWidth();
    const int height = m_Screen->GetHeight();
    static const Pixel emp_pixel(255, 255, 255, 0);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Pixel pixel = m_Screen->GetPixel(x, y);

            if (pixel == emp_pixel)
                EMPPixelHandler(m_Screen, x, y);
            
            /*auto found = m_PixelHandlers.find(pixel);
            if (found != m_PixelHandlers.end())
                found->second(m_Screen, x, y);*/
        }
    }
}

std::vector<Vec2> ScreenClient::FindMines(Vec2 bot_pixel_pos) {
    const int width = m_Screen->GetWidth();
    const int height = m_Screen->GetHeight();

    const int mine_width = 14;
    const int mine_height = 16;
    const int start_x = 180;
    const int start_y = 75;
    const int end_x = width - 28;
    const int end_y = height - mine_height - 1;    
    const Pixel black(0, 0, 0, 0);

    // how many non-black pixels are in each row of a mine
    const int mine_counts[] = {3, 6, 9, 13, 14, 14, 14, 14, 14, 14, 14, 12, 12, 10, 7, 2};

    std::vector<Vec2> mines;

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            if (m_Screen->GetPixel(x, y) != black) continue;
            if (m_Screen->GetPixel(x + 1, y) == black) continue;

            // Start checking if it's a mine from 5 pixels back
            x -= 5; 
            
            bool possible = true;
            for (int i = 0; i < mine_height; ++i) {
                int count = 0;

                for (int j = 0; j < mine_width; ++j) {
                    Pixel pixel = m_Screen->GetPixel(x + j, y + i);
                    if (pixel != black) count++;
                }

                int diff = std::abs(count - mine_counts[i]);
                if (diff > 2) {
                    possible = false;
                    break;
                }
            }

            x += 5;

            if (possible) {
                // It's probably a mine if we made it this far
                // Calculate how far away the mine is from the player ship (center of screen)
                Vec2 diff(x + 1 - (width / 2.0f), y + 7 - (height / 2.0f));

                // Add the mine to the list of mines by calculating its position based on the bot's pixel position in the level.
                double mine_x = std::floor((bot_pixel_pos.x + diff.x) / 16.0f);
                double mine_y = std::floor((bot_pixel_pos.y + diff.y) / 16.0f);
                mines.emplace_back(mine_x, mine_y);
            }
        }
    }

    return mines;
}

void ScreenClient::SelectPlayer(const std::string& name) {
    api::PlayerPtr target = m_PlayerWindow.Find(name);

    std::cout << "Selecting " << name << std::endl;

    if (!target.get()) {
        tcerr << "ScreenClient::SelectPlayer : "<< "Could not find " << name << " in player window. Scrolling up." << std::endl;;
        api::PlayerPtr botPlayer = m_PlayerWindow.Find(m_MemorySensor.GetBotPlayer()->GetName());

        int keycode = botPlayer ? VK_NEXT : VK_PRIOR;
        // Page up/down because the player wasn't found in the window
        m_Keyboard.ReleaseAll();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_Keyboard.Down(VK_LSHIFT);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Down(keycode);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        m_Keyboard.Up(VK_LSHIFT);
        m_Keyboard.Up(keycode);
        return;
    }

    if (target == m_PlayerWindow.GetSelectedPlayer()) return;

    Direction dir = m_PlayerWindow.GetDirection(target);

    // hopefully this doesn't happen
    if (dir == Direction::None) return;

    MoveTicker(dir);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    m_Screen->Update();
    m_PlayerWindow.Update(151);
    // Keep going until it's selected
    SelectPlayer(name);
}

Vec2 ScreenClient::GetResolution() {
    return Vec2(m_Screen->GetWidth(), m_Screen->GetHeight());
}
