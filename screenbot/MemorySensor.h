#ifndef MEMORY_SENSOR_H_
#define MEMORY_SENSOR_H_

#include "Vector.h"
#include "Player.h"
#include "ClientSettings.h"

#include <string>
#include <windows.h>
#include <map>
#include <iosfwd>

namespace Memory {

enum class SensorError {
    None, DebugPrivileges, Pid, Handle, BaseAddress
};

std::ostream& operator<<(std::ostream& os, SensorError error);

class MemorySensor {
public:
    typedef unsigned short PlayerID;

private:
    HANDLE m_ProcessHandle;
    uintptr_t m_ContBaseAddr;
    uintptr_t m_MenuBaseAddr;
    uintptr_t m_PositionAddr;
    DWORD m_Pid;
    bool m_Initialized;
    DWORD m_SettingsTimer;

    std::map<PlayerID, PlayerPtr> m_Players;
    Vec2 m_Position;
    Vec2 m_Velocity;
    unsigned short m_Freq;
    std::string m_Name;

    ShipSettings m_ShipSettings[8];

    void DetectSettings();
    void DetectName();
    void DetectPosition();
    void DetectFreq();
    void DetectPlayers();

public:
    MemorySensor();
    SensorError Initialize(HWND window);

    void Update(unsigned long dt);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetVelocity() const { return m_Velocity; }
    unsigned short GetFrequency() const { return m_Freq; }
    const std::string& GetName() const { return m_Name; }
    PlayerList GetPlayers();

    const ShipSettings& GetShipSettings(Ship ship) const { return m_ShipSettings[(int)ship]; }
};

} // ns Memory

#endif
