#include "MemorySensor.h"

#include "Bot.h"
#include "Memory.h"
#include "Common.h"

#include <iostream>
#include <algorithm>
#include <regex>

#pragma warning(disable : 4351)

namespace {

const std::regex ChatChannelRegex(R"::(^([0-9]+):(.+)> (.+)$)::");

} // ns

namespace Memory {

std::ostream& operator<<(std::ostream& os, SensorError error) {
    static const std::vector<std::string> strs = { "None", "DebugPrivileges", "Pid", "Handle", "BaseAddress" };
    os << strs[(int)error];
    return os;
}

template<typename Map, typename Vec>
void MapToVector(const Map& m, Vec& v) {
    for (typename Map::const_iterator it = m.begin(); it != m.end(); ++it )
        v.push_back(it->second);
}

MemorySensor::MemorySensor(Bot* bot)
    : m_ProcessHandle(nullptr),
      m_ContBaseAddr(0),
      m_MenuBaseAddr(0),
      m_PositionAddr(0),
      m_Pid(0),
      m_Initialized(false),
      m_Position(0, 0),
      m_Velocity(0, 0),
      m_Freq(9999),
      m_Name(""),
      m_SettingsTimer(0),
      m_SelectedIndex(-1),
      m_Settings(),
      m_CurrentChatEntry(0)
{
    
}

SensorError MemorySensor::Initialize(HWND window) {
    if (!Memory::GetDebugPrivileges())
        return SensorError::DebugPrivileges;

    if (GetWindowThreadProcessId(window, &m_Pid) == 0)
        return SensorError::Pid;

    if (!(m_ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_Pid)))
        return SensorError::Handle;

    m_ContBaseAddr = Memory::GetModuleBase("Continuum.exe", m_Pid);
    m_MenuBaseAddr = Memory::GetModuleBase("menu040.dll", m_Pid);

    if (m_ContBaseAddr == 0 || m_MenuBaseAddr == 0)
        return SensorError::BaseAddress;

    DetectName();
    DetectPosition();

    m_Initialized = true;

    OnUpdate(nullptr, 0);

    return SensorError::None;
}

api::PlayerList MemorySensor::GetPlayers() {
   api::PlayerList players;

    MapToVector(m_Players, players);

    return players;
}

void MemorySensor::DetectSelected() {
    uintptr_t addr = Memory::GetU32(m_ProcessHandle, m_ContBaseAddr + 0xC1AFC); // Starting address
    addr += 0x2DF44;

    m_SelectedIndex = Memory::GetU32(m_ProcessHandle, addr) & 0xFFFF;
}

void MemorySensor::DetectSettings() {
    uintptr_t addr = Memory::GetU32(m_ProcessHandle, m_ContBaseAddr + 0xC1AFC); // Starting address
    addr += 0x127EC + 0x1AE70;

    Memory::Read(m_ProcessHandle, addr, &m_Settings, sizeof(ClientSettings));
}

void MemorySensor::DetectName() {
    unsigned short pindex = Memory::GetU32(m_ProcessHandle, m_MenuBaseAddr + 0x47FA0) & 0xFFFF;
    const int ProfileStructLength = 2860;
    uintptr_t addr = m_MenuBaseAddr;

    addr = Memory::GetU32(m_ProcessHandle, addr + 0x47A38) + 0x15;
    if (addr == 0) return;

    addr += pindex * ProfileStructLength;

    std::string name = Memory::GetString(m_ProcessHandle, addr, 23);
    m_Name = name.substr(0, strlen(name.c_str()));
}

// Detect the position address by searching for a unique identifier
void MemorySensor::DetectPosition() {
    const unsigned int PosStructID = 0x4AC7C0;

    std::vector<unsigned int> found = Memory::FindU32(m_ProcessHandle, PosStructID);
    uintptr_t address = 0;

    if (found.size() == 1) {
        address = found.at(0) + 0x1C;
    } else {
        // More than one address with the identifier was found, narrow down the results.
        for (uintptr_t addr : found) {
            bool okay = true;
            for (int i = 1; i <= 3; ++i) {
                unsigned int value = Memory::GetU32(m_ProcessHandle, addr + i * 4);
                if (value != 0) {
                    okay = false;
                    break;
                }
            }
            if (okay) {
                address = addr + 0x1C;
                break;
            }
        }
    }

    if (address == 0) return;

    m_PositionAddr = address;

    m_Position.x = Memory::GetU32(m_ProcessHandle, address);
    m_Position.y = Memory::GetU32(m_ProcessHandle, address + 4);
}

void MemorySensor::DetectFreq() {
    uintptr_t addr = Memory::GetU32(m_ProcessHandle, m_ContBaseAddr + 0xC1AFC) + 0x30;
    const unsigned int FreqOffset = 0x12664;
    addr += FreqOffset;
    m_Freq = Memory::GetU32(m_ProcessHandle, addr);
}

void MemorySensor::DetectPlayers() {
    uintptr_t addr = Memory::GetU32(m_ProcessHandle, m_ContBaseAddr + 0xC1AFC); // Starting address

    addr += 0x127EC;

    uintptr_t count_addr = addr + 0x1884;
    uintptr_t players_addr = addr + 0x884;

    unsigned short count = Memory::GetU32(m_ProcessHandle, count_addr) & 0xFFFF;

    const unsigned char NameOffset = 0x6D;
    const unsigned char FreqOffset = 0x58;
    const unsigned char RotOffset = 0x3C;
    const unsigned char PosOffset = 0x04;
    const unsigned char SpeedOffset = 0x10;
    const unsigned char IDOffset = 0x18;
    const unsigned char ShipOffset = 0x5C;
    const unsigned char StatusOffset = 0x60;

    for (auto kv : m_Players)
        kv.second->SetInArena(false);

    bool first_run = m_Players.empty();

    for (unsigned short i = 0; i < count; ++i) {
        uintptr_t player_addr = Memory::GetU32(m_ProcessHandle, players_addr + (i * 4));
        if (player_addr == 0) continue;

        short x = Memory::GetU32(m_ProcessHandle, player_addr + PosOffset) / 1000;
        short y = Memory::GetU32(m_ProcessHandle, player_addr + PosOffset + 4) / 1000;
        int xspeed = static_cast<int>(Memory::GetU32(m_ProcessHandle, player_addr + SpeedOffset)) / 10;
        int yspeed = static_cast<int>(Memory::GetU32(m_ProcessHandle, player_addr + SpeedOffset + 4)) / 10;
        unsigned short rot = Memory::GetU32(m_ProcessHandle, player_addr + RotOffset) / 1000;
        unsigned int freq = Memory::GetU32(m_ProcessHandle, player_addr + FreqOffset);
        unsigned short pid = Memory::GetU32(m_ProcessHandle, player_addr + IDOffset);
        unsigned short ship = Memory::GetU32(m_ProcessHandle, player_addr + ShipOffset);
        std::string name = Memory::GetString(m_ProcessHandle, player_addr + NameOffset, 23);
        name = name.substr(0, strlen(name.c_str())); // trim nulls
        unsigned char status = 0;
        Memory::Read(m_ProcessHandle, player_addr + StatusOffset, &status, 1);

        api::PlayerPtr player;

        auto found = m_Players.find(pid);
        if (found == m_Players.end()) {
            player = std::make_shared<Player>();
            m_Players[pid] = player;
        } else {
            player = found->second;
        }

        x = (short)std::max(std::min((int)x, 1023 * 16), 0);
        y = (short)std::max(std::min((int)y, 1023 * 16), 0);

        player->SetInArena(true);

        player->SetName(name);
        player->SetFreq(freq);
        player->SetRotation(rot);
        player->SetPosition(Vec2(x, y));
        player->SetVelocity(Vec2(xspeed, yspeed));
        player->SetPid(pid);
        player->SetShip(static_cast<api::Ship>(ship));
        player->SetStatus(status);

        if (name.compare(m_Name) == 0)
            m_BotPlayer = player;

        if (!first_run)
            MQueue.PushMessage(new EnterMessage(player));
    }

    for (auto iter = m_Players.begin(); iter != m_Players.end(); ) {
        if (!iter->second->InArena()) {
            MQueue.PushMessage(new LeaveMessage(iter->second));

            iter = m_Players.erase(iter);
        } else {
            ++iter;
        }
    }
}

void MemorySensor::DetectChat() {
    uintptr_t addr = Memory::GetU32(m_ProcessHandle, m_ContBaseAddr + 0xC1AFC) + 0x2DD08;

    // The address where chat begins
    uintptr_t chat_addr = Memory::GetU32(m_ProcessHandle, addr);
    // The number of chat entries
    u32 entry_count = Memory::GetU32(m_ProcessHandle, addr + 8);
    // The size of each chat entry
    const unsigned int EntrySize = 0x124;

    int count = entry_count - m_CurrentChatEntry;

    if (count < 0)
        count += 64;

    bool first_run = m_ChatLog.size() == 0;

    for (int i = 0; i < count; ++i) {
        ChatEntry entry;

        if (m_CurrentChatEntry >= 1024)
            m_CurrentChatEntry -= 64;

        uintptr_t current_addr = chat_addr + (m_CurrentChatEntry * EntrySize);

        Memory::Read(m_ProcessHandle, current_addr, &entry, sizeof(ChatEntry));

        m_ChatLog.push_back(entry);

        if (!first_run) {
            // TODO: Push new chat message on message queue, delete logreader
            enum class ChatType {
                Arena = 0,
                Public = 2,
                Private = 5,
                Channel = 9
            };
            ChatMessage::Type type;

            switch (entry.type) {
            case 0:
                type = ChatMessage::Type::Arena;
                break;
            case 2:
                type = ChatMessage::Type::Public;
                break;
            case 5:
                type = ChatMessage::Type::Private;
                break;
            case 9:
                type = ChatMessage::Type::Channel;
                break;
            default:
                type = ChatMessage::Type::Other;
                break;
            }

            ChatMessage* mesg = new ChatMessage;

            mesg->SetMessage(entry.message);
            mesg->SetPlayer(entry.player);
            mesg->SetType(type);

            if (type == ChatMessage::Type::Channel) {
                std::sregex_iterator begin(mesg->GetMessage().begin(), mesg->GetMessage().end(), ChatChannelRegex);
                if (begin == std::sregex_iterator()) {
                    mesg->SetType(ChatMessage::Type::Other);
                } else {
                    std::smatch match = *begin;

                    mesg->SetChannel(atoi(std::string(match[1]).c_str()));
                    mesg->SetMessage(match[3]);
                }
            }
            
            MQueue.PushMessage(mesg);
            //std::cout << (int)entry.type << " - " << entry.player << (strlen(entry.player) > 0 ? "> " : "") << entry.message << std::endl;
        }

        ++m_CurrentChatEntry;
    }
}

bool MemorySensor::OnUpdate(api::Bot *bot, unsigned long dt) {
    if (!m_Initialized) return true;

    if (m_PositionAddr != 0) {
        m_Position.x = std::max(std::min((int)Memory::GetU32(m_ProcessHandle, m_PositionAddr), 1023 * 16), 0);
        m_Position.y = std::max(std::min((int)Memory::GetU32(m_ProcessHandle, m_PositionAddr + 4), 1023 * 16), 0);

        m_Velocity.x = (int)Memory::GetU32(m_ProcessHandle, m_PositionAddr + 8);
        m_Velocity.y = (int)Memory::GetU32(m_ProcessHandle, m_PositionAddr + 12);
    }

    m_SettingsTimer += dt;

    if (m_SettingsTimer >= 1000 || dt == 0) {
        DetectSettings();
        m_SettingsTimer = 0;
    }

    DetectFreq();
    DetectPlayers();
    DetectSelected();
    DetectChat();

    return true;
}

} // ns Memory
