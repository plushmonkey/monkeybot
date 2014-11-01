#include "Common.h"
#include "ScreenArea.h"
#include "Keyboard.h"
#include "WindowFinder.h"
#include "Bot.h"
#include "ScreenGrabber.h"
#include "State.h"
#include "Util.h"
#include "Memory.h"
#include <thread>
#include <tchar.h>
#include <iostream>
#include <map>
#include <string>
#include <queue>
#include <sstream>

Bot::Bot(int ship)
    : m_Finder(_T("Continuum")),
    m_Window(0),
    m_Keyboard(),
    m_LastEnemy(timeGetTime()),
    m_State(nullptr),
    m_ShipNum(ship),
    m_Target(0, 0),
    m_TargetInfo(0, 0, 0),
    m_EnemyTargetInfo(0, 0, 0),
    m_Energy(0),
    m_MaxEnergy(0),
    m_PosAddr(0),
    m_Level(),
    m_ProcessHandle(nullptr)
{ }

unsigned int Bot::GetX() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr) / 16;
}
unsigned int Bot::GetY() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr + 4) / 16;
}

ScreenAreaPtr& Bot::GetRadar() {
    return m_Radar;
}

ScreenAreaPtr& Bot::GetShip() {
    return m_Ship;
}

ScreenAreaPtr& Bot::GetPlayer() {
    return m_Player;
}

std::shared_ptr<ScreenGrabber> Bot::GetGrabber() {
    return m_Grabber;
}

HWND Bot::SelectWindow() {
    const WindowFinder::Matches& matches = m_Finder.GetMatches();
    std::map<int, const WindowFinder::Match*> select_map;

    int i = 1;
    for (const WindowFinder::Match& kv : matches) {
        select_map[i] = &kv;
        tcout << i++ << " " << kv.second << " (" << kv.first << ")" << std::endl;
    }

    if (i == 1)
        throw std::runtime_error("No windows found.");

    tcout << "> ";

    tstring input;
    tcin >> input;

    int selection = _tstoi(input.c_str());

    if (selection < 1 || selection >= i)
        throw std::runtime_error("Error with window selection.");

    tcout << "Running bot on window " << select_map[selection]->second << "." << std::endl;

    return select_map[selection]->first;
}

void Bot::SelectShip() {
    static bool selected;

    if (selected == true) return;

    selected = true;
    tcout << "Ship number: ";

    std::string input;
    std::cin >> input;

    if (input.length() != 1 || input[0] < 0x31 || input[0] > 0x38) {
        tcout << "Defaulting to ship 8." << std::endl;
        return;
    }

    m_ShipNum = input[0] - 0x30;
}

void Bot::SetXRadar(bool on) {
    int count = 0;
    /* Try to toggle xradar, timeout after 50 tries */
    while (Util::XRadarOn(m_Grabber) != on && count < 50) {
        m_Keyboard.Down(VK_END);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        m_Keyboard.Up(VK_END);
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        m_Grabber->Update();
    }
}

std::vector<Coord> GetNeighbors(Coord pos, int rwidth) {
    std::vector<Coord> neighbors;
    neighbors.resize(4);
    int x, y;

    for (int i = 0; i < 4; i++)
        neighbors[i] = Coord(0, 0);

    x = pos.x;
    y = pos.y - 1;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[0] = Coord(x, y);

    x = pos.x + 1;
    y = pos.y;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[1] = Coord(x, y);

    x = pos.x;
    y = pos.y + 1;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[2] = Coord(x, y);

    x = pos.x - 1;
    y = pos.y;
    if (x > 0 && x < rwidth && y > 0 && y < rwidth)
        neighbors[3] = Coord(x, y);

    return neighbors;
}

void Bot::Update(DWORD dt) {
    m_Grabber->Update();
    m_Radar->Update();
    m_Ship->Update();
    m_Player->Update();

    if (!Util::InShip(m_Grabber)) {
        m_Keyboard.Up(VK_CONTROL);
        m_Keyboard.Send(VK_ESCAPE);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_Keyboard.Send(0x30 + m_ShipNum);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    m_Energy = Util::GetEnergy(GetEnergyAreas());

    if (m_Energy > m_MaxEnergy) m_MaxEnergy = m_Energy;

    static DWORD lastpf = 0;

    int rwidth = m_Radar->GetWidth();
    int dx, dy;
    double dist;

    try {
        std::vector<Coord> enemies = Util::GetEnemies(m_Radar);
        m_EnemyTarget = Util::GetClosestEnemy(enemies, m_Radar, &dx, &dy, &dist);

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;
    } catch (...) { 
        m_EnemyTarget = Coord(0, 0);
        m_Target = Coord(0, 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    m_State->Update(dt);
}

void Bot::GrabRadar() {
    int radarstart = 0;
    int radarend = 0;
    int radary = 0;

    while (radarend == 0) {
        m_Grabber->Update();
        for (int y = static_cast<int>(m_Grabber->GetHeight() / 1.5); y < m_Grabber->GetHeight(); y++) {
            for (int x = static_cast<int>(m_Grabber->GetWidth() / 1.5); x < m_Grabber->GetWidth(); x++) {
                Pixel pix = m_Grabber->GetPixel(x, y);

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
                    x = m_Grabber->GetWidth();
                    y = m_Grabber->GetHeight();
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        tcout << "Finding radar location." << std::endl;
    }

    int radarwidth = radarend - radarstart;

    tcout << "Creating radar screen with width of " << radarwidth << " at " << radarstart << ", " << radary << "." << std::endl;

    m_Radar = m_Grabber->GetArea(radarstart, radary, radarwidth, radarwidth);

    if (!m_Radar.get()) {
        tcerr << "Resolution (" << m_Grabber->GetWidth() << "x" << m_Grabber->GetHeight() << ") not supported." << std::endl;
        std::abort();
    }
}

bool GetDebugPrivileges() {
    HANDLE token = nullptr;
    bool success = false;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
        TOKEN_PRIVILEGES privileges;

        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0))
            success = true;

        CloseHandle(token);
    }

    return success;
}

int Bot::Run() {
    bool ready = false;

    while (!ready) {
        try {
            if (!m_Window)
                m_Window = SelectWindow();

            SelectShip();

            m_Grabber = std::make_shared<ScreenGrabber>(m_Window);
            m_Ship = m_Grabber->GetArea(m_Grabber->GetWidth() / 2 - 18, m_Grabber->GetHeight() / 2 - 18, 36, 36);

            GrabRadar();

            m_Player = m_Radar->GetArea(m_Radar->GetWidth() / 2 - 1, m_Radar->GetWidth() / 2 - 1, 4, 4);

            int width = m_Grabber->GetWidth();

            m_EnergyArea[0] = m_Grabber->GetArea(width - 78, 0, 16, 21);
            m_EnergyArea[1] = m_Grabber->GetArea(width - 62, 0, 16, 21);
            m_EnergyArea[2] = m_Grabber->GetArea(width - 46, 0, 16, 21);
            m_EnergyArea[3] = m_Grabber->GetArea(width - 30, 0, 16, 21);

            ready = true;
        } catch (const std::exception& e) {
            tcerr << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    tcout << "Loading config file bot.conf" << std::endl;

    m_Config.Set(_T("XPercent"),        _T("75"));
    m_Config.Set(_T("RunPercent"),      _T("30"));
    m_Config.Set(_T("SafeResetTime"),   _T("10000"));
    m_Config.Set(_T("TargetDistance"),  _T("10"));
    m_Config.Set(_T("RunDistance"),     _T("30"));
    m_Config.Set(_T("StopBombing"),     _T("90"));
    m_Config.Set(_T("BombTime"),        _T("5000"));
    m_Config.Set(_T("FireBombs"),       _T("True"));
    m_Config.Set(_T("FireGuns"),        _T("True"));
    m_Config.Set(_T("DistanceFactor"),  _T("10"));
    m_Config.Set(_T("Level"),           _T("C:\\Program Files (x86)\\Continuum\\zones\\SSCE Hyperspace\\pub20140727.lvl"));
    m_Config.Set(_T("BulletDelay"),     _T("20"));
    m_Config.Set(_T("ScaleDelay"),      _T("True"));
    m_Config.Set(_T("MemoryScanning"),  _T("True"));
    m_Config.Set(_T("OnlyCenter"),      _T("True"));

    if (!m_Config.Load(_T("bot.conf")))
        tcout << "Could not load bot.conf. Using default values." << std::endl;

    tstringstream ss;

    ss << _T("ship") << to_tstring(m_ShipNum) << _T(".conf");

    if (!m_Config.Load(ss.str()))
        tcout << "Could not load " << ss.str() << ". Not overriding any ship specific configurations." << std::endl;

    for (auto iter = m_Config.begin(); iter != m_Config.end(); ++iter)
        tcout << iter->first << ": " << iter->second << std::endl;

    if (!m_Level.Load(m_Config.Get<tstring>(_T("Level"))))
        tcerr << "Could not load level " << m_Config.Get<tstring>(_T("Level")) << "\n";

    if (m_Config.Get<bool>(_T("MemoryScanning"))) {
        if (GetDebugPrivileges()) {
            DWORD pid;
            GetWindowThreadProcessId(m_Window, &pid);

            m_ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

            if (!m_ProcessHandle)
                tcerr << "Could not open process for reading.\n";
        } else {
            std::cerr << "Could not get Windows debug privileges." << std::endl;
        }
    }

    if (m_ProcessHandle)
        this->SetState(std::make_shared<MemoryState>(*this));
    else
        this->SetState(std::make_shared<AggressiveState>(*this));

    DWORD last_update = timeGetTime();

    while (true) {
        DWORD dt = timeGetTime() - last_update;

        last_update = timeGetTime();
        Update(dt);
    }

    return 0;
}