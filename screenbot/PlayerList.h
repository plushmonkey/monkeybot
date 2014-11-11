#ifndef PLAYER_LIST_H_
#define PLAYER_LIST_H_

#include "Common.h"

#include <vector>
#include <string>

class PlayerList {
private:
    ScreenAreaPtr m_Area;
    std::vector<std::string> m_Players;

public:
    PlayerList();
    PlayerList(ScreenAreaPtr area);

    void SetScreenArea(ScreenAreaPtr area);

    void Update(unsigned long dt);
};

#endif
