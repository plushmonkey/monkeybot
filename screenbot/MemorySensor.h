#ifndef MEMORY_SENSOR_H_
#define MEMORY_SENSOR_H_

#include "Vector.h"
#include "Player.h"
#include "ClientSettings.h"

#include <string>
#include <windows.h>
#include <map>
#include <iosfwd>

class Bot;

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
    int m_SelectedIndex;

    unsigned int m_UpdateID;

    std::map<PlayerID, PlayerPtr> m_Players;
    Vec2 m_Position;
    Vec2 m_Velocity;
    unsigned int m_Freq;
    std::string m_Name;

    ShipSettings m_ShipSettings[8];
    PlayerPtr m_BotPlayer;

    void DetectSelected();
    void DetectSettings();
    void DetectName();
    void DetectPosition();
    void DetectFreq();
    void DetectPlayers();

    bool OnUpdate(Bot* bot, unsigned long dt);
public:
    MemorySensor(Bot* bot);
    SensorError Initialize(HWND window);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetVelocity() const { return m_Velocity; }
    unsigned int GetFrequency() const { return m_Freq; }
    const std::string& GetName() const { return m_Name; }
    PlayerList GetPlayers();
    PlayerPtr GetBotPlayer() { return m_BotPlayer; }
    const ShipSettings& GetShipSettings(Ship ship) const { return m_ShipSettings[(int)ship]; }
};

} // ns Memory

#endif
