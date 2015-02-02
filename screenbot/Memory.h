#ifndef MEMORY_H_
#define MEMORY_H_

#include <vector>
#include <windows.h>
#include <string>

namespace Memory {

    struct PlayerData {
        std::string name;
        unsigned int freq;
        unsigned short rot;
        unsigned short x;
        unsigned short y;
        int xspeed;
        int yspeed;
        unsigned short pid;
    };

    struct WritableArea {
        unsigned int base;
        unsigned int size;

        WritableArea(unsigned int base, unsigned int size) : base(base), size(size) { }
    };


    std::vector<unsigned int> FindRange(HANDLE handle, const unsigned int start, const unsigned int end);
    std::vector<unsigned int> FindU32(HANDLE handle, const unsigned int value);
    
    unsigned int GetU32(HANDLE handle, const unsigned int address);
    std::string GetString(HANDLE handle, const unsigned int address, size_t len);

    std::vector<WritableArea> GetWritableAreas(HANDLE handle);
    ULONG GetModuleBase(char *name, ULONG pid);
    bool GetDebugPrivileges();

    unsigned int GetPosAddress(HANDLE handle, uintptr_t base);
    std::string GetBotName(HANDLE handle, uintptr_t menu_base);
    unsigned short GetBotFreq(HANDLE handle, uintptr_t base);
    std::vector<PlayerData> GetPlayerData(HANDLE handle, uintptr_t base);
    
}

#endif
