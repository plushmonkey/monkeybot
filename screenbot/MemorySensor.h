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

class MemorySensor : public api::MemorySensor {
public:
    typedef unsigned short PlayerID;

    struct ChatEntry {
        char message[256];
        //unsigned char type; // 0x02 == pub
        char player[24];
        char unknown[8];
        unsigned char type;
        char unknown2[3];
    };

private:
    HANDLE m_ProcessHandle;
    uintptr_t m_ContBaseAddr;
    uintptr_t m_MenuBaseAddr;
    uintptr_t m_PositionAddr;
    DWORD m_Pid;
    bool m_Initialized;
    DWORD m_SettingsTimer;
    int m_SelectedIndex;

    std::vector<ChatEntry> m_ChatLog;
    u32 m_CurrentChatEntry;

    std::map<PlayerID, api::PlayerPtr> m_Players;
    Vec2 m_Position;
    Vec2 m_Velocity;
    unsigned int m_Freq;
    std::string m_Name;

    ClientSettings m_Settings;
    
    api::PlayerPtr m_BotPlayer;

    void DetectSelected();
    void DetectSettings();
    void DetectName();
    void DetectPosition();
    void DetectFreq();
    void DetectPlayers();
    void DetectChat();

public:
    MemorySensor(Bot* bot);
    SensorError Initialize(HWND window);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetVelocity() const { return m_Velocity; }
    unsigned int GetFrequency() const { return m_Freq; }
    const std::string& GetName() const { return m_Name; }
   api::PlayerList GetPlayers();
    api::PlayerPtr GetBotPlayer() const { return m_BotPlayer; }
    const ShipSettings& GetShipSettings(api::Ship ship) const { return m_Settings.ShipSettings[(int)ship]; }
    const ClientSettings& GetClientSettings() const { return m_Settings; }
    bool OnUpdate(api::Bot* bot, unsigned long dt);
};

} // ns Memory

#endif
