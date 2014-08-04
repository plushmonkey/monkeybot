#ifndef BOT_H_
#define BOT_H_

#include <memory>
#include "WindowFinder.h"
#include "Keyboard.h"
#include "State.h"
#include <vector>
#include <Windows.h>

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

    HWND SelectWindow();

public:
    Bot();

    ScreenAreaPtr& GetRadar();
    ScreenAreaPtr& GetShip();
    ScreenAreaPtr& GetPlayer();
    std::weak_ptr<ScreenGrabber> GetGrabber();
    ScreenAreaPtr* GetEnergyAreas() { return m_EnergyArea; }
    Keyboard& GetKeyboard() { return m_Keyboard; }

    DWORD GetLastEnemy() const { return m_LastEnemy; }
    void SetLastEnemy(DWORD time) { m_LastEnemy = time; }

    void SetState(StatePtr state) { m_State = state; }
    int Run();
    void Update();
};


#endif
