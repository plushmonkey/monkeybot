#ifndef BOT_H_
#define BOT_H_

#include "api/Api.h"
#include "WindowFinder.h"
#include "Keyboard.h"
#include "State.h"
#include "Config.h"
#include "Level.h"
#include "LogReader.h"
#include "CommandHandler.h"
#include "Taunter.h"
#include "MemorySensor.h"
#include "Survivor.h"
#include "Revenge.h"
#include "plugin/PluginManager.h"
#include "Version.h"
#include "ShipEnforcer.h"

#include <memory>
#include <mutex>
#include <vector>
#include <Windows.h>

#include <WinUser.h>

class FileStream;

struct TargetInfo {
    int dx;
    int dy;
    double dist;

    TargetInfo(int dx, int dy, double dist) : dx(dx), dy(dy), dist(dist) { }
};

class Bot : public api::Bot, public MessageHandler<ChatMessage> {
private:
    WindowFinder m_Finder;
    HWND m_Window;
    api::StatePtr m_State;
    api::Ship m_Ship;
    api::PlayerPtr m_EnemyTarget;
    TargetInfo m_EnemyTargetInfo;
    api::PlayerPtr m_LastTarget;
    int m_MaxEnergy;
    int m_Energy;
    int m_LastEnergy;
    Level m_Level;
    Pathing::Grid<short> m_Grid;
    DWORD m_LastEnemy;
    DWORD m_RepelTimer;
    DWORD m_LancTimer;
    bool m_Paused;
    bool m_Flagging;
    PluginManager m_PluginManager;

    api::SelectorPtr m_EnemySelector;

    std::mutex m_SendMutex;
    std::string m_SendBuffer;
    
    shared_ptr<Revenge> m_Revenge;
    
    std::map<UpdateID, UpdateFunction> m_Updaters;
    
    Config m_Config;
    ClientPtr m_Client;
    SurvivorGame m_Survivor;
    Memory::MemorySensor m_MemorySensor;
    Taunter m_Taunter;
    shared_ptr<LogReader> m_LogReader;
    CommandHandler m_CommandHandler;
    Version m_Version;

    std::string m_AttachTarget;

    shared_ptr<ShipEnforcer> m_ShipEnforcer;
    shared_ptr<api::EnemySelectorFactory> m_EnemySelectors;

    HWND SelectWindow();
    void SelectShip();
    //bool EnforceShip();
    void FindEnemy(unsigned long dt);
    void AttachUpdate(unsigned long dt);
    void EnforceCenter(unsigned long dt);
    void HandleMessage(ChatMessage* mesg);
    bool WindowHasFocus() const;

public:
    Bot(int ship);

    api::Version& GetVersion() { return m_Version; }

    ClientPtr GetClient();
    Config& GetConfig() { return m_Config; }
    PluginManager& GetPluginManager() { return m_PluginManager; }
    api::CommandHandler& GetCommandHandler() { return m_CommandHandler; }

    Pathing::Grid<short>& GetGrid() { return m_Grid; }
    const Level& GetLevel() const { return m_Level; }

    api::PlayerPtr GetEnemyTarget() const { return m_EnemyTarget; }
    TargetInfo GetEnemyTargetInfo() const { return m_EnemyTargetInfo; }

    int GetEnergy() const { return m_Energy; }
    int GetLastEnergy() const { return m_LastEnergy; }
    int GetMaxEnergy() const { return m_MaxEnergy; }
    int GetEnergyPercent() const { 
        if (m_MaxEnergy == 0) return 0;
        return static_cast<int>((m_Energy / (float)m_MaxEnergy) * 100);
    }

    void SetEnemySelector(api::SelectorPtr selector);

    bool FullEnergy() const;

    std::string GetName() const;
    Vec2 GetPos() const { return m_MemorySensor.GetPosition() / 16; }
    Vec2 GetPixelPos() const { return m_MemorySensor.GetPosition(); }

    Vec2 GetVelocity() const;
    Vec2 GetHeading() const;
    double GetSpeed() const { return GetVelocity().Length(); }
    void SetSpeed(double target);
    
    const std::string& GetAttachTarget() const { return m_AttachTarget; }
    bool GetFlagging() const { return m_Flagging; }
    void SetFlagging(bool f) { m_Flagging = f; }
    void SetTaunt(bool b) { 
        m_Config.Taunt = b;
        m_Taunter.SetEnabled(b);
    }

    bool GetPaused() const { return m_Paused; }
    void SetPaused(bool paused) { m_Paused = paused; }

    void SendMessage(const std::string& str);

    bool IsInCenter() const {
        Vec2 pos = GetPos();
        Vec2 spawn((float)m_Config.SpawnX, (float)m_Config.SpawnY);

        return (pos - spawn).Length() < m_Config.CenterRadius;
    }

    void SetState(api::StatePtr state) { m_State = state; }
    void SetState(api::StateType type);

    api::StateType GetStateType() const {
        return m_State->GetType();
    }

    DWORD GetLastEnemy() const { return m_LastEnemy; }
    void SetLastEnemy(DWORD last) { m_LastEnemy = last; }

    void CheckLancs(const std::string& line);

    void Follow(const std::string& name);

    int Run();
    void Update(DWORD dt);

    void SetShip(api::Ship ship);

    shared_ptr<Revenge> GetRevenge() { return m_Revenge; }

    api::Ship GetShip() const { return m_Ship; }
    int GetShipNum() const { return (int)m_Ship + 1; }

    UpdateID RegisterUpdater(UpdateFunction func) {
        static UpdateID id;

        m_Updaters[id] = func;

        return id++;
    }

    void UnregisterUpdater(UpdateID id) {
        if (m_Updaters.find(id) != m_Updaters.end())
            m_Updaters.erase(id);
    }

    Memory::MemorySensor& GetMemorySensor() {
        return m_MemorySensor;
    }

    SurvivorGame* GetSurvivorGame() { return &m_Survivor; }

    void UpdateLog();

    bool IsInSafe() const {
        return m_Client->IsInSafe(GetPos(), m_Level);
    }

    bool IsInShip() const {
        return m_MemorySensor.GetBotPlayer()->GetShip() != api::Ship::Spectator;
    }

    unsigned int GetFreq() const;
    shared_ptr<api::EnemySelectorFactory> GetEnemySelectors() { return m_EnemySelectors; }

    api::StatePtr GetState() const { return m_State; }
};

#define RegisterBotUpdater(bot, function) (bot)->RegisterUpdater(std::bind(&function, this, std::placeholders::_1, std::placeholders::_2)); 

#endif
