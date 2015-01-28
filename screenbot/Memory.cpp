#include "Memory.h"

#include <tlhelp32.h>

namespace Memory {


bool GetDebugPrivileges() {
    HANDLE token = nullptr;
    bool success = false;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
        TOKEN_PRIVILEGES privileges;

        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0))
            success = true;

        CloseHandle(token);
    }

    return success;
}

std::vector<WritableArea> GetWritableAreas(HANDLE handle) {
    MEMORY_BASIC_INFORMATION meminfo;
    DWORD pos = 0;
    const int Writable = (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
    std::vector<WritableArea> areas;

    while (VirtualQueryEx(handle, (LPVOID)pos, &meminfo, sizeof(meminfo))) {
        if ((meminfo.Protect & Writable))
            areas.push_back(WritableArea(reinterpret_cast<unsigned int>(meminfo.BaseAddress), static_cast<unsigned int>(meminfo.RegionSize)));
        pos += meminfo.RegionSize;
    }

    return areas;
}

unsigned int GetPosAddress(HANDLE handle, uintptr_t base) {
    const unsigned int PosStructID = 0x4AC7C0;

    std::vector<uintptr_t> found = Memory::FindU32(handle, PosStructID);

    if (found.size() == 1) return found.at(0) + 0x1C;

    for (uintptr_t addr : found) {
        bool okay = true;
        for (int i = 1; i <= 3; ++i) {
            unsigned int value = Memory::GetU32(handle, addr + i * 4);
            if (value != 0) {
                okay = false;
                break;
            }
        }
        if (okay) return addr;
    }

    return 0;
}

std::string GetBotName(HANDLE handle, uintptr_t base) {
    std::vector<unsigned int> jumps = { 0x2B104, 0x25C, 0x32C, 0x4E8 };
    uintptr_t addr = base;

    for (unsigned int jump : jumps) {
        addr = Memory::GetU32(handle, addr + jump);
        if (base == 0) return "";
    }

    return Memory::GetString(handle, addr + 0x257, 23);
}

std::vector<unsigned int> FindU32(HANDLE handle, const unsigned int value) {
    const unsigned int upper = 0x7FFFFFFF;
    std::vector<unsigned int> found;

    std::vector<WritableArea> areas = GetWritableAreas(handle);

    for (WritableArea& area : areas) {
        if (area.size == 0) continue;

        char *buffer = new char[area.size];
        SIZE_T num_read;

        if (ReadProcessMemory(handle, (LPVOID)area.base, buffer, area.size, &num_read)) {
            for (unsigned int i = 0; i < num_read - 4; i += 4) {
                unsigned int check = *reinterpret_cast<unsigned int *>(buffer + i);

                if (check == value)
                    found.push_back(area.base + i);
            }
        }

        delete[] buffer;
    }

    return found;
}

std::vector<unsigned int> FindRange(HANDLE handle, const unsigned int start, const unsigned int end) {
    const unsigned int upper = 0x7FFFFFFF;
    std::vector<unsigned int> found;

    std::vector<WritableArea> areas = GetWritableAreas(handle);

    for (WritableArea& area : areas) {
        if (area.size == 0) continue;

        char *buffer = new char[area.size];
        SIZE_T num_read;

        if (ReadProcessMemory(handle, (LPVOID)area.base, buffer, area.size, &num_read)) {
            for (unsigned int i = 0; i < num_read - 4; i += 4) {
                unsigned int check = *reinterpret_cast<unsigned int *>(buffer + i);

                if (check >= start && check <= end)
                    found.push_back(area.base + i);
            }
        }

        delete[] buffer;
    }

    return found;
}


unsigned int GetU32(HANDLE handle, const unsigned int address) {
    unsigned int value;
    SIZE_T num_read;

    if (ReadProcessMemory(handle, (LPVOID)address, &value, sizeof(unsigned int), &num_read))
        return value;

    return 0;
}

std::string GetString(HANDLE handle, const unsigned int address, size_t len) {
    std::string value;
    SIZE_T read;

    value.resize(len);

    if (ReadProcessMemory(handle, (LPVOID)address, const_cast<char*>(value.c_str()), len, &read)) 
        return value;

    return "";
}

ULONG GetModuleBase(char *name, ULONG pid) {
    MODULEENTRY32 me = { 0 };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

    me.dwSize = sizeof(me);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    BOOL bModule = Module32First(snap, &me);
    while (bModule) {
        if (!name || strcmp(name, me.szModule) == 0) {
            CloseHandle(snap);
            return (ULONG)me.modBaseAddr;
        }
        bModule = Module32Next(snap, &me);
    }
    CloseHandle(snap);
    return 0;
}

}