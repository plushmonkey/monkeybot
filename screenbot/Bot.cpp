#include "Bot.h"

#include "Common.h"
#include "ScreenArea.h"
#include "Keyboard.h"
#include "WindowFinder.h"
#include "ScreenGrabber.h"
#include "State.h"
#include "Util.h"
#include "Memory.h"
#include "Client.h"
#include "Tokenizer.h"
#include "FileStream.h"
#include "Random.h"

#include <thread>
#include <tchar.h>
#include <iostream>
#include <map>
#include <string>
#include <queue>
#include <sstream>
#include <regex>

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
      m_Client(nullptr),
      m_Attach(false),
      m_CenterOnly(false),
      m_RepelTimer(0),
      m_Taunter(this),
      m_Taunt(false),
      m_LancTimer(20000),
      m_Flagging(false),
      m_CommandHandler(this),
      m_Hyperspace(false),
      m_BaseAddress(0)
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

unsigned int Bot::GetPixelX() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr);
}
unsigned int Bot::GetPixelY() const {
    if (m_PosAddr == 0) return 0;

    return Memory::GetU32(m_ProcessHandle, m_PosAddr + 4);
}

std::string Bot::GetName() const {
    return m_Config.Get<std::string>("Name");
    //return Memory::GetBotName(m_ProcessHandle, m_BaseAddress);
}

Vec2 Bot::GetHeading() const {
    double rad = (((40 - (m_Client->GetRotation() + 30)) % 40) * 9) * (M_PI / 180);
    return Vec2((float)std::cos(rad), -(float)std::sin(rad));
}

Vec2 Bot::GetVelocity() const {
    if (m_PosAddr == 0) return Vec2(0, 0);

    int xspeed = Memory::GetU32(m_ProcessHandle, m_PosAddr + 8);
    int yspeed = Memory::GetU32(m_ProcessHandle, m_PosAddr + 12);

    return Vec2(xspeed / 16.0, yspeed / 16.0);
}

void Bot::SetSpeed(float target) {
    Vec2 velocity = GetVelocity();
    
    double len = velocity.Length();

    Vec2 heading = GetHeading();
    velocity.Normalize();

    double dot = heading.Dot(velocity);

    if (dot < 0.0) {
        // Going backwards, so just press up key
        m_Client->Down(false);
        m_Client->Up(true);
    } else {
        if (len < target) {
            m_Client->Down(false);
            m_Client->Up(true);
        } else {
            m_Client->Up(false);
            m_Client->Down(true);
        }
    }

    //std::cout << "Speed: " << len << ", Target: " << target << std::endl;
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

void Bot::CheckLancs(const std::string& line) {
    std::string regex_str = R"::(^\s*\(S|E\) (.+)$)::";

    if (m_ShipNum > 6 || m_ShipNum == 3 || m_ShipNum == 4)
        regex_str = R"::(^\s*\(S\) (.+)$)::";

    std::regex summoner_regex(regex_str);
    Util::Tokenizer tokenizer(line);

    if (m_LancTimer > 5000) return;

    tokenizer(',');

    std::vector<std::string> summoners;
    std::vector<std::string> evokers;

    std::string target;

    for (const std::string& lanc : tokenizer) {
        std::sregex_iterator begin(lanc.begin(), lanc.end(), summoner_regex);
        std::sregex_iterator end;
        
        if (begin != end) {
            size_t pos = lanc.find("(S) ");
            if (pos != std::string::npos) {
                summoners.emplace_back(lanc.substr(lanc.find("(S) ") + 4));
                continue;
            }
            
            pos = lanc.find("(E)");
            if (pos == std::string::npos) continue;
            evokers.emplace_back(lanc.substr(lanc.find("(E) ") + 4));
            break;
        }
    }

    if (evokers.size() > 0)
        target = evokers.at(0);

    // Prioritize summoners over evokers
    if (summoners.size() > 0)
        target = summoners.at(0);

    m_AttachTarget = target.substr(0, 12);

    if (target.length() > 0) {
        m_LancTimer = 5001;
        std::cout << "Target: " << target << std::endl;
        m_Client->SelectPlayer(target);
    }
}

void Bot::HandleMessage(ChatMessage* mesg) {
    std::string line = mesg->GetLine();

    if (m_Hyperspace)
        CheckLancs(line);
}

void Bot::Update(DWORD dt) {
    m_Client->Update(dt);

    if (!m_Client->InShip()) {
        m_Client->EnterShip(m_ShipNum);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        m_Client->Update(30);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (m_Client->InShip() && m_Attach && m_Hyperspace) {
            if (m_Flagging) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                m_Client->SendString("?flag");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            m_Client->SendString("?lancs");
        }
    }
    
    m_Energy = m_Client->GetEnergy();

    // Reset maximum energy on every death
    if (m_Energy == 0) m_MaxEnergy = 0;
    if (m_Energy > m_MaxEnergy) m_MaxEnergy = m_Energy;

    int dx, dy;
    double dist;

    bool reset_target = false;

    if (m_PosAddr && GetStateType() != StateType::MemoryState) {
        m_AliveTime += dt;

        if (!InCenter() && m_CenterOnly && !m_Attach) {
            m_Client->ReleaseKeys();

            if (GetEnergyPercent() == 100)
                tcout << "Warping because position is out of center (" << GetX() << ", " << GetY() << ")." << std::endl;
            m_Client->Warp();
        }
    }

    m_LancTimer += dt;

    m_LogReader->Update(dt);

    if (m_LancTimer >= 35000 && m_Hyperspace) {
        if (m_Attach)
            m_Client->SendString("?lancs");
        m_LancTimer = 0;
    }

    Vec2 pos = GetPos();

    // Attach to ticked player when in center safe
    if (m_Attach && !m_Client->OnSoloFreq() && this->GetStateType() != StateType::AttachState) {
        //if (pos.x >= m_SpawnX - 20 && pos.x <= m_SpawnX + 20 && pos.y >= m_SpawnY - 20 && pos.y <= m_SpawnY + 20)
        if (InCenter())
            SetState(std::make_shared<AttachState>(*this));
    }
    
    try {
        m_EnemyTarget = m_Client->GetClosestEnemy(pos, GetHeading(), m_Level, &dx, &dy, &dist);

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;
        
        SetLastEnemy(timeGetTime());

        if (m_PosAddr && m_CenterOnly) {
            if (m_EnemyTarget.x < 320 || m_EnemyTarget.x >= 703 || m_EnemyTarget.y < 320 || m_EnemyTarget.x >= 703)
                reset_target = true;
        }

        m_RepelTimer += dt;
        int epct = GetEnergyPercent();
        if (m_RepelTimer >= 1200 && epct > 0 && epct < m_RepelPercent) {
            m_Client->Repel();
            m_RepelTimer = 0;
        }
    } catch (...) { 
        reset_target = true;
    }

    if (reset_target) {
        m_EnemyTarget = Vec2(0, 0);
        m_EnemyTargetInfo.dist = 0.0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    m_State->Update(dt);
    
    MQueue.Dispatch();
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
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
    m_Config.Set(_T("Baseduel"),        _T("False"));
    m_Config.Set(_T("ProjectileSpeed"), _T("3400"));
    m_Config.Set(_T("CenterRadius"),    _T("400"));
    m_Config.Set(_T("IgnoreCarriers"),  _T("False"));
    m_Config.Set(_T("IgnoreDelayDistance"), _T("10"));
    m_Config.Set(_T("RepelPercent"),    _T("25"));
    m_Config.Set(_T("UseBurst"),        _T("True"));
	m_Config.Set(_T("DecoyDelay"),		_T("0"));
    m_Config.Set(_T("LogFile"),         _T("C:\\Program Files (x86)\\Continuum\\logs\\bot.log"));
    m_Config.Set(_T("Taunt"),           _T("False"));
    m_Config.Set(_T("Name"),            _T(""));
    m_Config.Set(_T("Hyperspace"),      _T("false"));
    
    if (!m_Config.Load(_T("bot.conf")))
        tcout << "Could not load bot.conf. Using default values." << std::endl;

    std::string includes = m_Config.Get<std::string>("Include");
    if (includes.length() > 0) {
        Util::Tokenizer tokenizer(includes);

        tokenizer('|');

        for (auto it = tokenizer.begin(); it != tokenizer.end(); ++it) {
            tcout << "Loading include file " << *it << std::endl;
            m_Config.Load(*it);
        }
    }

    tstringstream ss;

    ss << _T("ship") << to_tstring(m_ShipNum) << _T(".conf");

    if (!m_Config.Load(ss.str()))
        tcout << "Could not load " << ss.str() << ". Not overriding any ship specific configurations." << std::endl;

    for (auto iter = m_Config.begin(); iter != m_Config.end(); ++iter)
        tcout << iter->first << ": " << iter->second << std::endl;

    m_LogReader = std::make_shared<LogReader>(m_Config.Get<std::string>("LogFile"), 3000);
    m_LogReader->Clear();

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

    if (Memory::GetDebugPrivileges()) {
        DWORD pid;
        GetWindowThreadProcessId(m_Window, &pid);

        if (!(m_ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid))) {
            tcerr << "Could not open process for reading.\n";
            std::cin.get();
            std::exit(1);
        }
        
        m_BaseAddress = Memory::GetModuleBase("Continuum.exe", pid);
        uintptr_t addr = Memory::GetPosAddress(m_ProcessHandle, m_BaseAddress);

        SetPosAddress(addr);
        tcout << "Position address: " << std::hex << addr << std::dec << std::endl;
    } else {
        tcerr << "Could not get Windows debug privileges." << std::endl;
        std::cin.get();
        std::exit(1);
    }

    m_Attach = m_Config.Get<bool>("Attach");
    m_CenterOnly = m_Config.Get<bool>("OnlyCenter");
    m_SpawnX = m_Config.Get<int>("SpawnX");
    m_SpawnY = m_Config.Get<int>("SpawnY");
    m_CenterRadius = m_Config.Get<int>("CenterRadius");
    m_RepelPercent = m_Config.Get<int>("RepelPercent");
    m_Taunt = m_Config.Get<bool>("Taunt");
    m_Hyperspace = m_Config.Get<bool>("Hyperspace");

    m_Taunter.SetEnabled(m_Taunt);

    if (m_PosAddr == 0)
        this->SetState(std::make_shared<MemoryState>(*this));
    else
        this->SetState(std::make_shared<PatrolState>(*this));

    tcout << "Bot name: " << GetName() << std::endl;

    tcout << "Bot started." << std::endl;
    DWORD last_update = timeGetTime();
    
    while (true) {
        DWORD dt = timeGetTime() - last_update;
        //std::cout << dt << " ";
        //std::cout << GetStateType() << std::endl;
        last_update = timeGetTime();
        Update(dt);
    }

    return 0;
}
