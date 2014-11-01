#include "Level.h"
#include <iostream>
#include <fstream>
#include <tchar.h>

Level::Level() : m_Filename(_T("")), m_Tiles(nullptr) {
    
}

bool Level::IsSolid(int id) {
    if (id == 0) return false;
    if (id < 170) return true;
    if (id >= 192 && id <= 240) return true;
    if (id >= 242 && id <= 252) return true;
    return false;
}

bool Level::Load(const tstring& filename) {
    m_Filename = filename;

    std::ifstream in(m_Filename, std::ios::in | std::ios::binary);
    
    if (!in.is_open()) return false;

    in.seekg(0, std::ios::end);
    int len = static_cast<int>(in.tellg());
    in.seekg(0, std::ios::beg);

    if (len <= 0) {
        in.close();
        return false;
    }

    m_Tiles = new int[1024 * 1024];

    char* buffer = new char[len];
    in.read(buffer, len);
    in.close();

    int pos = 0;

    if (buffer[0] == 'B' && buffer[1] == 'M') {
        pos = *(int *)(buffer + 2);
        len -= pos;
    }

    for (int y = 0; y < 1024; ++y) {
        for (int x = 0; x < 1024; ++x)
            m_Tiles[y * 1024 + x] = 0;
    }

    while (pos < len) {
        Tile tile = *reinterpret_cast<Tile *>(buffer + pos);

        m_Tiles[tile.y * 1024 + tile.x] = tile.tile;
        pos += sizeof(Tile);
    }
    return true;
}

int Level::GetTileID(unsigned short x, unsigned short y) const {
    if (!m_Tiles) return 0;
    if (x >= 1024 || y >= 1024) return 0;
    return m_Tiles[y * 1024 + x];
}

bool Level::IsSolid(unsigned short x, unsigned short y) const {
    return Level::IsSolid(GetTileID(x, y));
}