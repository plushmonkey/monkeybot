#ifndef BOT_H_
#define BOT_H_

#include "WindowFinder.h"
#include "Keyboard.h"
#include "State.h"
#include "Config.h"
#include "Level.h"

#include <memory>
#include <vector>
#include <Windows.h>

struct TargetInfo {
    int dx;
    int dy;
    double dist;

    TargetInfo(int dx, int dy, double dist) : dx(dx), dy(dy), dist(dist) { }
};

class Bot {
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

    Vec2 m_LastPos;
    DWORD m_VelocityTimer;
    Vec2 m_Velocity;

    Config m_Config;
    bool m_Attach;
    bool m_CenterOnly;
    int m_SpawnX;
    int m_SpawnY;

    ClientPtr m_Client;

    HANDLE m_ProcessHandle;
    std::vector<unsigned> m_PossibleAddr;
    unsigned int m_PosAddr;

    HWND SelectWindow();
    void SelectShip();

public:
    Bot(int ship);

    ClientPtr GetClient();
    const Config& GetConfig() const { return m_Config; }

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
    Vec2 GetPos() const { return Vec2(static_cast<float>(GetX()), static_cast<float>(GetY())); }
    Vec2 GetVelocity() const { return m_Velocity; }
    float GetSpeed() const { return std::abs((m_Velocity * 16 * 10).Length()); }
    
    unsigned int GetPosAddress() const { return m_PosAddr; }
    void SetPosAddress(unsigned int addr) { m_PosAddr = addr; }
    void SetPossibleAddr(std::vector<unsigned> addr) { m_PossibleAddr = addr; }

    int Run();
    void Update(DWORD dt);
    void SetState(StatePtr state) { m_State = state; }

    bool InCenter() const {
        if (m_PosAddr == 0) return true;
        unsigned x = GetX();
        unsigned y = GetY();

        return x > 320 && x < 703 && y > 320 && y < 703;
    }

    StateType GetStateType() const {
        return m_State->GetType();
    }

    DWORD GetLastEnemy() const { return m_LastEnemy; }
    void SetLastEnemy(DWORD last) { m_LastEnemy = last; }

};

#endif
