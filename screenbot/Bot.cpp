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
      m_Level(),
      m_AliveTime(0),
      m_Grid(1024, 1024),
      m_LastEnemy(0),
      m_Client(nullptr),
      m_RepelTimer(0),
      m_Taunter(this),
      m_LancTimer(20000),
      m_Flagging(false),
      m_CommandHandler(this),
      m_Paused(false),
      m_Survivor(this)
{ }

ClientPtr Bot::GetClient() {
    return m_Client;
}

std::string Bot::GetName() const {
    return m_MemorySensor.GetName();
}

Vec2 Bot::GetHeading() const {
    double rad = (((40 - (m_Client->GetRotation() + 30)) % 40) * 9) * (M_PI / 180);
    return Vec2((float)std::cos(rad), -(float)std::sin(rad));
}

Vec2 Bot::GetVelocity() const {
    return m_MemorySensor.GetVelocity() / 16;
}

void Bot::SetShip(Ship ship) {
    m_ShipNum = (int)ship + 1;

    if (m_Client->InShip()) {
        m_Client->ReleaseKeys();
        m_Client->SetXRadar(false);
        while (m_Client->GetEnergy() < GetMaxEnergy() || m_Client->GetEnergy() == 0) {
            m_Client->Update(100);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    m_Client->EnterShip(m_ShipNum);

    m_Config.LoadShip(GetShip());
}

void Bot::SetSpeed(double target) {
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

void Bot::ForceLogRead() {
    m_LogReader->Update(0);
    MQueue.Dispatch();
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
        std::cout << "Target: " << m_AttachTarget << std::endl;
        m_Client->SelectPlayer(m_AttachTarget);
    }
}

void Bot::HandleMessage(ChatMessage* mesg) {
    if (mesg->GetType() != ChatMessage::Type::Other) return;

    std::string line = mesg->GetMessage();

    if (m_Config.Hyperspace)
        CheckLancs(line);
}

void Bot::Update(DWORD dt) {
    m_LogReader->Update(dt);

    if (m_Paused) {
        MQueue.Dispatch();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }

    m_Client->Update(dt);

    if (!m_Client->InShip()) {
        m_Client->EnterShip(m_ShipNum);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        m_Client->Update(30);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (m_Client->InShip() && m_Config.Attach && m_Config.Hyperspace) {
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

    m_AliveTime += dt;

    if (!InCenter() && m_Config.CenterOnly && !m_Config.Attach) {
        m_Client->ReleaseKeys();

        if (GetEnergyPercent() == 100)
            tcout << "Warping because position is out of center " << GetPos() << "." << std::endl;
        m_Client->Warp();
    }

    m_LancTimer += dt;

    if (m_LancTimer >= 35000 && m_Config.Hyperspace) {
        if (m_Config.Attach)
            m_Client->SendString("?lancs");
        m_LancTimer = 0;
    }

    Vec2 pos = GetPos();

    // Attach to ticked player when in center safe
    if (m_Config.Attach && !m_Client->OnSoloFreq() && this->GetStateType() != StateType::AttachState) {
        if (InCenter())
            SetState(std::make_shared<AttachState>(*this));
    }

    
    try {
        m_EnemyTarget = m_Client->GetClosestEnemy(pos, GetHeading(), m_Level, &dx, &dy, &dist);

        if (!m_EnemyTarget->InArena())
            reset_target = true;

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;
        
        SetLastEnemy(timeGetTime());

        if (m_Config.CenterOnly) {
            Vec2 enemy_pos = m_EnemyTarget->GetPosition() / 16;

            if (enemy_pos.x < 320 || enemy_pos.x >= 703 || enemy_pos.y < 320 || enemy_pos.x >= 703)
                reset_target = true;
        }

        m_RepelTimer += dt;
        int epct = GetEnergyPercent();
        if (m_RepelTimer >= 1200 && epct > 0 && epct < m_Config.RepelPercent) {
            m_Client->Repel();
            m_RepelTimer = 0;
        }
    } catch (...) { 
        reset_target = true;
    }

    if (reset_target) {
        m_EnemyTarget = nullptr;
        m_EnemyTargetInfo.dist = 0.0;
    }

    static DWORD target_timer = 0;
    static DWORD alive_timer = 0;
    target_timer += dt;

    if (m_Config.Commander && m_Config.Survivor)
        m_Survivor.Update(dt);

    if (m_Config.Commander && target_timer >= 10000 && m_EnemyTarget != nullptr && m_EnemyTarget != m_LastTarget) {
        m_Client->SendString(";!target " + m_EnemyTarget->GetName());
        m_LastTarget = m_EnemyTarget;
        target_timer = 0;
    }

    m_MemorySensor.Update(dt);
    m_State->Update(dt);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    MQueue.Dispatch();
}

int Bot::Run() {
    bool ready = false;

    while (!ready) {
        try {
            if (!m_Window)
                m_Window = SelectWindow();

            SelectShip();

            ready = true;
        } catch (const std::exception& e) {
            tcerr << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    tcout << "Loading config file config.json" << std::endl;

    if (!m_Config.Load("config.json"))
        tcout << "Could not load config.json. Using default values." << std::endl;

    m_Config.LoadShip(GetShip());

    tcout << m_Config;

    m_LogReader = std::make_shared<LogReader>(m_Config.LogFile, 3000);
    m_LogReader->Clear();

    if (!m_Level.Load(m_Config.Level)) {
        tcerr << "Could not load level " << m_Config.Level << "\n";
    } else {
        tcout << "Creating grid for the level." << std::endl;
        for (int y = 0; y < 1024; ++y) {
            for (int x = 0; x < 1024; ++x) {
                if (m_Level.IsSolid(x, y))
                    m_Grid.SetSolid(x, y, true);
            }
        }
    }

    Memory::SensorError result = m_MemorySensor.Initialize(m_Window);

    if (result != Memory::SensorError::None) {
        tcerr << "Memory::SensorError::" << result << std::endl;
        std::cin.get();
        std::exit(1);
    }

    m_MemorySensor.Update(0);
    m_Taunter.SetEnabled(m_Config.Taunt);

    try {
        m_Client = std::make_shared<ScreenClient>(m_Window, m_Config, m_MemorySensor);
    } catch (const std::exception& e) {
        tcerr << e.what() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::exit(1);
    }

    this->SetState(std::make_shared<PatrolState>(*this));

    tcout << "Bot name: " << GetName() << std::endl;
    tcout << "Bot started." << std::endl;

    auto players = m_MemorySensor.GetPlayers();

    std::cout << "Count: " << players.size() << std::endl;
    for (auto p : players) {
        std::cout << p->GetName() << ": Pos: " << p->GetPosition() << " Vel: " << p->GetVelocity();
        std::cout << " Rot: " << p->GetRotation() << " Freq: " << p->GetFreq() << " pid: " << p->GetPid() << std::endl;
    }
    
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
