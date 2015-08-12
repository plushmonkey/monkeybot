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
#include "Selector.h"

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
      m_Ship(api::Ship(ship - 1)),
      m_EnemyTargetInfo(0, 0, 0),
      m_Energy(0),
      m_LastEnergy(0),
      m_MaxEnergy(0),
      m_Level(),
      m_Grid(1024, 1024),
      m_LastEnemy(0),
      m_Client(nullptr),
      m_RepelTimer(0),
      m_Taunter(this),
      m_LancTimer(20000),
      m_Flagging(false),
      m_CommandHandler(this),
      m_Paused(false),
      m_Survivor(this),
      m_MemorySensor(this),
      m_EnemySelector(new ClosestEnemySelector()),
      m_ShipEnforcer(new ShipEnforcer(this)),
      m_EnemySelectors(new EnemySelectorFactory()),
      m_Version()
{ }

ClientPtr Bot::GetClient() {
    return m_Client;
}

std::string Bot::GetName() const {
    return m_MemorySensor.GetName();
}

void Bot::SetState(api::StateType type) {
    switch (type) {
        case api::StateType::AggressiveState:
            SetState(std::make_shared<AggressiveState>(this));
            break;
        case api::StateType::PatrolState:
            SetState(std::make_shared<PatrolState>(this));
            break;
        case api::StateType::AttachState:
            SetState(std::make_shared<AttachState>(this));
            break;
        case api::StateType::ChaseState:
            SetState(std::make_shared<ChaseState>(this));
            break;
        case api::StateType::BaseduelState:
            SetState(std::make_shared<BaseduelState>(this));
            break;
    }
}

Vec2 Bot::GetHeading() const {
    double rad = (((40 - (m_Client->GetRotation() + 30)) % 40) * 9) * (M_PI / 180);
    return Vec2((float)std::cos(rad), -(float)std::sin(rad));
}

Vec2 Bot::GetVelocity() const {
    return m_MemorySensor.GetVelocity() / 16;
}

bool Bot::FullEnergy() const {
    return !m_Client->InShip() || (m_Client->GetEnergy(GetShip()) >= GetMaxEnergy() && m_Client->GetEnergy(GetShip()) != 0);
}

unsigned int Bot::GetFreq() const {
    return m_MemorySensor.GetFrequency();
}

void Bot::Follow(const std::string& name) {
    static bool centerOnly = GetConfig().CenterOnly;

    if (name.length() == 0) {
        std::cout << "No longer following" << std::endl;
        SetState(std::make_shared<PatrolState>(this));
    } else {
        SetState(std::make_shared<FollowState>(this, name));
        std::cout << "Following: " << name << std::endl;
    }

    centerOnly = !centerOnly;

    m_Config.CenterOnly = centerOnly;
}

void Bot::SetShip(api::Ship ship) {
    api::Ship current = m_MemorySensor.GetBotPlayer()->GetShip();
    if (ship == current) return;

    m_Ship = ship;

    if (m_Client->InShip()) {
        m_Client->ReleaseKeys();
        m_Client->SetXRadar(false);

        int timeout = 5000;
        //while (!FullEnergy() && (timeout > 0 || ship == api::Ship::Spectator)) {
        while (!FullEnergy() && timeout > 0) {
            m_Client->Update(100);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timeout -= 100;
        }
    }

    if (ship != api::Ship::Spectator) {
        m_Client->EnterShip(GetShipNum());

        m_Config.LoadShip(GetShip());
    } else {
        m_Client->Spec();
    }
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
}

void Bot::SendMessage(const std::string& str) {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_SendMutex);
            if (m_SendBuffer.length() == 0) {
                m_SendBuffer = str;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void Bot::UpdateLog() {
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

bool Bot::WindowHasFocus() const {
    HWND focus = GetForegroundWindow();

    return focus == m_Window;
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

    m_Ship = (api::Ship)(input[0] - 0x30 - 1);
}

void Bot::CheckLancs(const std::string& line) {
    // Only check the lancs if ?lancs was sent recently.
    if (m_LancTimer > 5000) return;

    std::string regex_str = R"::(^\s*\(S|E\) (.+)$)::";

    int ship_num = GetShipNum();
    if (ship_num > 6 || ship_num == 3 || ship_num == 4)
        regex_str = R"::(^\s*\(S\) (.+)$)::";

    std::regex summoner_regex(regex_str);
    Util::Tokenizer tokenizer(line);

    tokenizer(',');

    std::vector<std::string> summoners;
    std::vector<std::string> evokers;

    std::string target;

    for (const std::string& lanc : tokenizer) {
        std::sregex_iterator begin(lanc.begin(), lanc.end(), summoner_regex);
        std::sregex_iterator end;
        
        if (begin != end) {
            size_t pos = lanc.find("(S)");
            if (pos != std::string::npos) {
                summoners.emplace_back(lanc.substr(lanc.find("(S) ") + 4));
                continue;
            }
            
            pos = lanc.find("(E)");
            if (pos == std::string::npos) continue;
            evokers.emplace_back(lanc.substr(lanc.find("(E) ") + 4));
        }
    }

    if (evokers.size() == 0 && summoners.size() == 0) return;

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

void Bot::SetEnemySelector(api::SelectorPtr selector) {
    m_EnemySelector = selector;
}

void Bot::FindEnemy(unsigned long dt) {
    int dx, dy;
    double dist;

    bool reset_target = false;
    //m_EnemyTarget = m_Client->GetClosestEnemy(pos, GetHeading(), m_Level, &dx, &dy, &dist);
    m_EnemyTarget = m_EnemySelector->Select(this);
    if (m_EnemyTarget) {
        if (!m_EnemyTarget->InArena())
            reset_target = true;

        Util::GetDistance(m_EnemyTarget->GetPosition() / 16, GetPos(), &dx, &dy, &dist);

        m_EnemyTargetInfo.dx = dx;
        m_EnemyTargetInfo.dy = dy;
        m_EnemyTargetInfo.dist = dist;

        SetLastEnemy(timeGetTime());

        if (m_Config.CenterOnly) {
            Vec2 enemy_pos = m_EnemyTarget->GetPosition() / 16;

            ///// todo: this is hyperspace specific, fix it
            if (enemy_pos.x < 320 || enemy_pos.x >= 703 || enemy_pos.y < 320 || enemy_pos.y >= 703)
                reset_target = true;
        }

        // todo: move repel somewhere els
        m_RepelTimer += dt;
        int epct = GetEnergyPercent();

        bool delay_over = m_RepelTimer >(unsigned int)m_MemorySensor.GetShipSettings(GetShip()).BombFireDelay * 10;
        bool lost_energy = GetLastEnergy() > GetEnergy() && GetLastEnergy() - GetEnergy() < 300;
        if (delay_over && epct > 0 && lost_energy && epct < m_Config.RepelPercent) {
            m_Client->Repel();
            m_RepelTimer = 0;
        }
    } else {
        reset_target = true;
    }

    if (reset_target) {
        m_EnemyTarget = nullptr;
        m_EnemyTargetInfo.dist = 0.0;
    }
}

void Bot::AttachUpdate(unsigned long dt) {
    Vec2 pos = GetPos();
    // Attach to ticked player when in center safe
    if (m_Config.Attach && !m_Client->OnSoloFreq() && this->GetStateType() != api::StateType::AttachState) {
        if (IsInCenter()) {
            unsigned int freq = m_MemorySensor.GetBotPlayer()->GetFreq();
            api::PlayerList players = m_MemorySensor.GetPlayers();
            bool should_attach = false;
            Vec2 spawn(GetConfig().SpawnX, GetConfig().SpawnY);

            for (api::PlayerPtr&player : players) {
                if (player->GetFreq() == freq) {
                    if ((player->GetPosition() / 16 - spawn).Length() > GetConfig().CenterRadius) {
                        should_attach = true;
                        break;
                    }
                }
            }
            if (should_attach)
                SetState(std::make_shared<AttachState>(this));
        }
    }
}

void Bot::EnforceCenter(unsigned long dt) {
    if (!IsInCenter() && m_Config.CenterOnly && !m_Config.Attach && IsInShip()) {
        m_Client->ReleaseKeys();

        if (GetEnergyPercent() == 100)
            tcout << "Warping because position is out of center " << GetPos() << "." << std::endl;
        m_Client->Warp();
    }
}

void Bot::Update(DWORD dt) {
    static bool paused_for_focus = false;

    bool has_focus = WindowHasFocus();

    if (!has_focus) {
        if (!GetPaused()) {
            SetPaused(true);

            paused_for_focus = true;
        }
    } else {
        if (paused_for_focus) {
            paused_for_focus = false;
            SetPaused(false);
        }
    }

    m_LogReader->Update(dt);

    {
        std::lock_guard<std::mutex> lock(m_SendMutex);
        if (m_SendBuffer.length() > 0) {
            m_Client->SendString(m_SendBuffer);
            m_SendBuffer.clear();
        }
    }

    if (m_Paused) {
        m_PluginManager.OnUpdate(this, dt);
        m_MemorySensor.OnUpdate(this, dt);
        if (IsInShip())
            m_ShipEnforcer->OnUpdate(this, dt);
        MQueue.Dispatch();
        m_Client->ReleaseKeys();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        return;
    }

    m_Client->Update(dt);

    m_PluginManager.OnUpdate(this, dt);
    m_MemorySensor.OnUpdate(this, dt);

    // Update all of the updaters
    for (auto kv : m_Updaters) {
        if (!kv.second(this, dt)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            MQueue.Dispatch();
            return;
        }
    }

    if (!IsInShip()) {
        MQueue.Dispatch();
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        return;
    }

    m_LastEnergy = m_Energy;
    m_Energy = m_Client->GetEnergy(GetShip());

    // Reset maximum energy on every death
    if (m_Energy == 0) m_MaxEnergy = 0;
    if (m_Energy > m_MaxEnergy) m_MaxEnergy = m_Energy;

    EnforceCenter(dt);

    // todo: pull into hyperspace plugin
    m_LancTimer += dt;

    if (m_LancTimer >= 35000 && m_Config.Hyperspace) {
        if (m_Config.Attach)
            m_Client->SendString("?lancs");
        m_LancTimer = 0;
    }

    AttachUpdate(dt);
    FindEnemy(dt);

    static DWORD target_timer = 0;
    target_timer += dt;

    // todo: move commander stuff into plugin probably
    if (m_Config.Commander && m_Config.Survivor)
        m_Survivor.Update(dt);

    if (m_Config.Commander && target_timer >= 10000 && m_EnemyTarget != nullptr && m_EnemyTarget != m_LastTarget) {
        m_Client->SendString(";!target " + m_EnemyTarget->GetName());
        m_LastTarget = m_EnemyTarget;
        target_timer = 0;
    }

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
        Util::ExitWithError("Could not load config.json");

    m_Config.LoadShip(GetShip());
    
    tcout << m_Config;

    m_CommandHandler.InitPermissions();

    m_LogReader = std::make_shared<LogReader>(m_Config.LogFile, 500);
    m_LogReader->Clear();

    if (!m_Level.Load(m_Config.Level)) {
        Util::ExitWithError("Could not load level " + m_Config.Level);
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
        std::stringstream ss;

        ss << "Memory::SensorError::" << result;
        Util::ExitWithError(ss.str());
    }

    m_Taunter.SetEnabled(m_Config.Taunt);
    
    try {
        m_Client = std::make_shared<ScreenClient>(m_Window, m_Config, m_MemorySensor);
    } catch (const std::exception& e) {
        Util::ExitWithError(e.what());
    }

    this->SetState(std::make_shared<PatrolState>(this));

    tcout << "Bot name: " << GetName() << std::endl;
    tcout << "Bot started." << std::endl;

    const int MaxDeaths = 4;
    m_Revenge = std::make_shared<Revenge>(*this, GetShip(), api::Ship::Shark, MaxDeaths);
    m_Revenge->SetEnabled(m_Config.Revenge);

    int plugins = m_PluginManager.LoadPlugins(this, "plugins");

    PluginManager::const_iterator iter = m_PluginManager.begin();
    while (iter != m_PluginManager.end()) {
        std::cout << (*iter)->GetName() << " loaded" << std::endl;
        
        ++iter;
    }

    tcout << plugins << " plugin" << (plugins == 1 ? "" : "s") << " loaded." << std::endl;

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
