#ifndef BOT_H_
#define BOT_H_

#include <memory>
#include "WindowFinder.h"
#include "Keyboard.h"
#include "State.h"
#include <vector>
#include <Windows.h>
#include "Config.h"
#include "Level.h"

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
    Keyboard m_Keyboard;
    ScreenAreaPtr m_EnergyArea[4];
    DWORD m_LastEnemy;
    StatePtr m_State;
    ScreenAreaPtr m_Radar;
    ScreenAreaPtr m_Ship;
    ScreenAreaPtr m_Player;
    std::shared_ptr<ScreenGrabber> m_Grabber;
    int m_ShipNum;
    Coord m_EnemyTarget;
    Coord m_Target;
    TargetInfo m_TargetInfo;
    TargetInfo m_EnemyTargetInfo;
    std::vector<Coord> m_Path;
    int m_MaxEnergy;
    int m_Energy;
    Config m_Config;
    Level m_Level;

    HANDLE m_ProcessHandle;

    unsigned int m_PosAddr;

    HWND SelectWindow();
    void SelectShip();
    void GrabRadar();

public:
    Bot(int ship);

    ScreenAreaPtr& GetRadar();
    ScreenAreaPtr& GetShip();
    ScreenAreaPtr& GetPlayer();
    std::shared_ptr<ScreenGrabber> GetGrabber();
    ScreenAreaPtr* GetEnergyAreas() { return m_EnergyArea; }
    Keyboard& GetKeyboard() { return m_Keyboard; }

    DWORD GetLastEnemy() const { return m_LastEnemy; }
    void SetLastEnemy(DWORD time) { m_LastEnemy = time; }

    Coord GetEnemyTarget() const { return m_EnemyTarget; }
    TargetInfo GetEnemyTargetInfo() const { return m_EnemyTargetInfo; }

    Coord GetTarget() const { return m_Target; }
    TargetInfo GetTargetInfo() const { return m_TargetInfo; }

    const std::vector<Coord>& GetPath() const { return m_Path; }
    int GetEnergy() const { return m_Energy; }
    int GetMaxEnergy() const { return m_MaxEnergy; }
    int GetEnergyPercent() const { 
        if (m_MaxEnergy == 0) return 100;
        return static_cast<int>((m_Energy / (float)m_MaxEnergy) * 100);
    }

    unsigned int GetX() const;
    unsigned int GetY() const;

    const Level& GetLevel() const { return m_Level; }

    HANDLE GetProcessHandle() const { return m_ProcessHandle; }
    unsigned int GetPosAddress() const { return m_PosAddr; }

    void SetPosAddress(unsigned int addr) { m_PosAddr = addr; }

    const Config& GetConfig() const { return m_Config; }

    void SetState(StatePtr state) { m_State = state; }
    int Run();
    void Update();

    void SetXRadar(bool on);
};


#endif
