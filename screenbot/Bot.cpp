#include "Common.h"
#include "ScreenArea.h"
#include "Keyboard.h"
#include "WindowFinder.h"
#include "Bot.h"
#include "ScreenGrabber.h"
#include "State.h"
#include "Util.h"
#include "Memory.h"
#include "Client.h"
#include "Tokenizer.h"
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
      m_State(nullptr),
      m_ShipNum(ship),
      m_EnemyTargetInfo(0, 0, 0),
      m_Energy(0),
      m_MaxEnergy(0),
      m_PosAddr(0),
      m_Level(),
      m_ProcessHandle(nullptr),
      m_AliveTime(0),
      m_Grid(1024, 1024),
      m_LastEnemy(0),
      m_Client(nullptr)
{ }

ClientPtr Bot::GetClient() {
    return m_Client;
}

unsigned int Bot::GetX() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr) / 16;
}
unsigned int Bot::GetY() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr + 4) / 16;
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

void Bot::Update(DWORD dt) {
    m_Client->Update(dt);
    
    if (!m_Client->InShip())
        m_Client->EnterShip(m_ShipNum);
    
    m_Energy = m_Client->GetEnergy();

    if (m_Energy > m_MaxEnergy) m_MaxEnergy = m_Energy;

    int dx, dy;
    double dist;

    bool reset_target = false;

    if (m_PosAddr && GetStateType() != StateType::MemoryState) {
        m_AliveTime += dt;

        if (!InCenter() && m_Config.Get<bool>("OnlyCenter")) {
            if (m_AliveTime < 10000 && m_PossibleAddr.size() > 1) {
                tcout << "Bot appears to be out of center. The position address is probably wrong, trying next one." << std::endl;
                m_PossibleAddr.erase(m_PossibleAddr.begin());
                SetPosAddress(m_PossibleAddr.at(0));
                m_Client->Warp();
            } else {
                tcout << "Warping because position is out of center (" << GetX() << ", " << GetY() << ")." << std::endl;
                m_Client->Warp();
            }
        }
    }


    Coord pos(GetX(), GetY());

    // Attach to ticked player when in center safe
    if (m_AliveTime > 10000 && m_Config.Get<bool>("Attach")) {
        int spawnX = m_Config.Get<int>("SpawnX");
        int spawnY = m_Config.Get<int>("SpawnX");
        if (pos.x >= spawnX - 20 && pos.x <= spawnX + 20&& pos.y >= spawnY - 20 && pos.y <= spawnY + 20)
            m_Client->Attach();
    }

    try {
        std::vector<Coord> enemies = m_Client->GetEnemies();
        m_EnemyTarget = m_Client->GetClosestEnemy(&dx, &dy, &dist);

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;

        SetLastEnemy(timeGetTime());

        if (m_PosAddr && m_Config.Get<bool>("OnlyCenter")) {
            Coord enemy = m_Client->GetRealPosition(pos, m_EnemyTarget, m_Level);
            if (enemy.x < 320 || enemy.x >= 703 || enemy.y < 320 || enemy.x >= 703)
                reset_target = true;
        }

    } catch (...) { 
        reset_target = true;
    }

    if (reset_target)
        m_EnemyTarget = Coord(0, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    m_State->Update(dt);
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

            m_Client = std::make_shared<ScreenClient>(m_Window, m_Config);

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
    m_Config.Set(_T("Patrol"),          _T("True"));
    m_Config.Set(_T("RotationStore"),   _T("hyperspace.rot"));
    m_Config.Set(_T("Attach"),          _T("False"));
    m_Config.Set(_T("MapZoom"),         _T("9"));
    m_Config.Set(_T("MinGunRange"),     _T("0"));
    m_Config.Set(_T("SpawnX"),          _T("512"));
    m_Config.Set(_T("SpawnY"),          _T("512"));
    m_Config.Set(_T("Waypoints"),       _T("(400, 585), (565, 580), (600, 475), (512, 460), (425, 460), (385, 505)"));
    m_Config.Set(_T("Include"),         _T(""));

    if (!m_Config.Load(_T("bot.conf")))
        tcout << "Could not load bot.conf. Using default values." << std::endl;

    std::string includes = m_Config.Get<std::string>("Include");
    if (includes.length() > 0) {
        Util::Tokenizer tokenizer(includes);

        tokenizer('|');

        for (auto it = tokenizer.begin(); it != tokenizer.end(); ++it) {
            std::cout << "Loading include file " << *it << std::endl;
            m_Config.Load(*it);
        }
    }

    tstringstream ss;

    ss << _T("ship") << to_tstring(m_ShipNum) << _T(".conf");

    if (!m_Config.Load(ss.str()))
        tcout << "Could not load " << ss.str() << ". Not overriding any ship specific configurations." << std::endl;

    for (auto iter = m_Config.begin(); iter != m_Config.end(); ++iter)
        tcout << iter->first << ": " << iter->second << std::endl;

    if (!m_Level.Load(m_Config.Get<tstring>(_T("Level")))) {
        tcerr << "Could not load level " << m_Config.Get<tstring>(_T("Level")) << "\n";
    } else {
        tcout << "Creating grid for the level." << std::endl;
        for (int y = 0; y < 1024; ++y) {
            for (int x = 0; x < 1024; ++x) {
                if (m_Level.IsSolid(x, y))
                    m_Grid.SetSolid(x, y, true);
            }
        }
    }

    bool memory_enabled = false;

    if (m_Config.Get<bool>(_T("MemoryScanning"))) {
        if (GetDebugPrivileges()) {
            DWORD pid;
            GetWindowThreadProcessId(m_Window, &pid);

            m_ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

            if (!m_ProcessHandle)
                tcerr << "Could not open process for reading.\n";
            else
                memory_enabled = true;
        } else {
            tcerr << "Could not get Windows debug privileges." << std::endl;
        }
    }

    if (!memory_enabled) {
        m_Config.Set(_T("OnlyCenter"), _T("False"));
        m_Config.Set(_T("Patrol"), _T("False"));
    }

    if (m_ProcessHandle)
        this->SetState(std::make_shared<MemoryState>(*this));
    else
        this->SetState(std::make_shared<AggressiveState>(*this));

    DWORD last_update = timeGetTime();

    while (true) {
        DWORD dt = timeGetTime() - last_update;
    //    std::cout << dt << " ";
    //    std::cout << GetStateType() << std::endl;
        last_update = timeGetTime();
        Update(dt);
    }

    return 0;
}