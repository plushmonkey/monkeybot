#ifndef MEMORY_H_
#define MEMORY_H_

#include <vector>
#include <windows.h>
#include <string>

namespace Memory {
    struct WritableArea {
        unsigned int base;
        unsigned int size;

        WritableArea(unsigned int base, unsigned int size) : base(base), size(size) { }
    };


    std::vector<unsigned int> FindRange(HANDLE handle, const unsigned int start, const unsigned int end);
    std::vector<unsigned int> FindU32(HANDLE handle, const unsigned int value);
    
    bool Read(HANDLE handle, const uintptr_t addr, void* buffer, size_t size);
    unsigned int GetU32(HANDLE handle, const unsigned int address);
    std::string GetString(HANDLE handle, const unsigned int address, size_t len);

    std::vector<WritableArea> GetWritableAreas(HANDLE handle);
    ULONG GetModuleBase(char *name, ULONG pid);
    bool GetDebugPrivileges();
    
}

#endif
