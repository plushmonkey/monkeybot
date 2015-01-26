#ifndef MEMORY_H_
#define MEMORY_H_

#include <vector>
#include <windows.h>

namespace Memory {
    struct WritableArea {
        unsigned int base;
        unsigned int size;

        WritableArea(unsigned int base, unsigned int size) : base(base), size(size) { }
    };


    std::vector<unsigned int> FindRange(HANDLE handle, const unsigned int start, const unsigned int end);
    unsigned int GetU32(HANDLE handle, const unsigned int address);
    std::vector<WritableArea> GetWritableAreas(HANDLE handle);

    ULONG GetModuleBase(char *name, ULONG pid);
}

#endif
