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
    Vec2 m_EnemyTarget;
    TargetInfo m_EnemyTargetInfo;
    int m_MaxEnergy;
    int m_Energy;
    Level m_Level;
    DWORD m_AliveTime;
    Pathing::Grid<short> m_Grid;
    DWORD m_LastEnemy;
    DWORD m_RepelTimer;
    int m_RepelPercent;

    Config m_Config;
    bool m_Attach;
    bool m_CenterOnly;
    int m_SpawnX;
    int m_SpawnY;
    int m_CenterRadius;
    bool m_Taunt;
    bool m_Hyperspace;

    ClientPtr m_Client;

    HANDLE m_ProcessHandle;
    std::vector<unsigned> m_PossibleAddr;
    unsigned int m_PosAddr;

    DWORD m_LancTimer;
    Taunter m_Taunter;

    std::shared_ptr<LogReader> m_LogReader;
    CommandHandler m_CommandHandler;

    std::string m_AttachTarget;

    bool m_Flagging;

    HWND SelectWindow();
    void SelectShip();

    void HandleMessage(ChatMessage* mesg);
    
public:
    Bot(int ship);

    ClientPtr GetClient();
    Config& GetConfig() { return m_Config; }

    Pathing::Grid<short>& GetGrid() { return m_Grid; }
    const Level& GetLevel() const { return m_Level; }

    Vec2 GetEnemyTarget() const { return m_EnemyTarget; }
    TargetInfo GetEnemyTargetInfo() const { return m_EnemyTargetInfo; }

    int GetEnergy() const { return m_Energy; }
    int GetMaxEnergy() const { return m_MaxEnergy; }
    int GetEnergyPercent() const { 
        if (m_MaxEnergy == 0) return 100;
        return static_cast<int>((m_Energy / (float)m_MaxEnergy) * 100);
    }

    HANDLE GetProcessHandle() const { return m_ProcessHandle; }

    unsigned int GetX() const;
    unsigned int GetY() const;
    unsigned int GetPixelX() const;
    unsigned int GetPixelY() const;
    Vec2 GetPos() const { return Vec2(static_cast<float>(GetX()), static_cast<float>(GetY())); }
    Vec2 GetPixelPos() const { return Vec2(static_cast<float>(GetPixelX()), static_cast<float>(GetPixelY())); }

    Vec2 GetVelocity() const;
    Vec2 GetHeading() const;
    double GetSpeed() const { return GetVelocity().Length(); }
    void SetSpeed(float target);
    
    unsigned int GetPosAddress() const { return m_PosAddr; }
    void SetPosAddress(unsigned int addr) { m_PosAddr = addr; }
    void SetPossibleAddr(std::vector<unsigned> addr) { m_PossibleAddr = addr; }
    
    const std::string& GetAttachTarget() const { return m_AttachTarget; }
    bool GetFlagging() const { return m_Flagging; }
    void SetFlagging(bool f) { m_Flagging = f; }
    bool GetAttaching() const { return m_Attach; }
    void SetAttaching(bool b) { m_Attach = b; }
    void SetCenterOnly(bool b) { m_CenterOnly = b; }
    bool GetTaunt() const { return m_Taunt; }
    void SetTaunt(bool b) { 
        m_Taunt = b;
        m_Taunter.SetEnabled(b);
    }

    bool InCenter() const {
        if (m_PosAddr == 0) return true;
        Vec2 pos = GetPos();
        Vec2 spawn((float)m_SpawnX, (float)m_SpawnY);

        return (pos - spawn).Length() < m_CenterRadius;
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
};

#endif
