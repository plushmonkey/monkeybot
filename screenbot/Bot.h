#ifndef BOT_H_
#define BOT_H_

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

#include <memory>
#include <vector>
#include <Windows.h>

class FileStream;

struct TargetInfo {
    int dx;
    int dy;
    double dist;

    TargetInfo(int dx, int dy, double dist) : dx(dx), dy(dy), dist(dist) { }
};

class Bot : public MessageHandler<ChatMessage> {
private:
    WindowFinder m_Finder;
    HWND m_Window;
    StatePtr m_State;
    int m_ShipNum;
    PlayerPtr m_EnemyTarget;
    TargetInfo m_EnemyTargetInfo;
    PlayerPtr m_LastTarget;
    int m_MaxEnergy;
    int m_Energy;
    Level m_Level;
    DWORD m_AliveTime;
    Pathing::Grid<short> m_Grid;
    DWORD m_LastEnemy;
    DWORD m_RepelTimer;
    DWORD m_LancTimer;
    bool m_Paused;
    bool m_Flagging;

    Config m_Config;
    ClientPtr m_Client;
    SurvivorGame m_Survivor;
    Memory::MemorySensor m_MemorySensor;
    Taunter m_Taunter;
    std::shared_ptr<LogReader> m_LogReader;
    CommandHandler m_CommandHandler;

    std::string m_AttachTarget;

    HWND SelectWindow();
    void SelectShip();
    void HandleMessage(ChatMessage* mesg);
    
public:
    Bot(int ship);

    ClientPtr GetClient();
    Config& GetConfig() { return m_Config; }

    Pathing::Grid<short>& GetGrid() { return m_Grid; }
    const Level& GetLevel() const { return m_Level; }

    PlayerPtr GetEnemyTarget() const { return m_EnemyTarget; }
    TargetInfo GetEnemyTargetInfo() const { return m_EnemyTargetInfo; }

    int GetEnergy() const { return m_Energy; }
    int GetMaxEnergy() const { return m_MaxEnergy; }
    int GetEnergyPercent() const { 
        if (m_MaxEnergy == 0) return 0;
        return static_cast<int>((m_Energy / (float)m_MaxEnergy) * 100);
    }

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
    void SetPaused(bool b) { m_Paused = b; }

    bool InCenter() const {
        Vec2 pos = GetPos();
        Vec2 spawn((float)m_Config.SpawnX, (float)m_Config.SpawnY);

        return (pos - spawn).Length() < m_Config.CenterRadius;
    }

    void SetState(StatePtr state) { m_State = state; }
    StateType GetStateType() const {
        return m_State->GetType();
    }

    DWORD GetLastEnemy() const { return m_LastEnemy; }
    void SetLastEnemy(DWORD last) { m_LastEnemy = last; }

    void CheckLancs(const std::string& line);

    int Run();
    void Update(DWORD dt);

    void SetShip(Ship ship);

    Ship GetShip() const { return (Ship)(m_ShipNum - 1); }

    Memory::MemorySensor GetMemorySensor() {
        return m_MemorySensor;
    }

    SurvivorGame* GetSurvivorGame() { return &m_Survivor; }

    void ForceLogRead();
};

#endif
