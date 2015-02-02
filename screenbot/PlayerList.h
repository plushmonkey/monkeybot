#ifndef PLAYER_LIST_H_
#define PLAYER_LIST_H_

#include "Common.h"
#include "Player.h"

#include <vector>
#include <string>

class PlayerWindow {
private:
    ScreenAreaPtr m_Area;
    PlayerList m_Players;
    unsigned long m_UpdateTimer;
    PlayerPtr m_Selected;

    std::string GetLineText(int line);
public:
    PlayerWindow();
    PlayerWindow(ScreenAreaPtr area);

    void SetScreenArea(ScreenAreaPtr area);

    void Update(unsigned long dt);

    int GetPlayerIndex(PlayerPtr player);
    PlayerPtr GetPlayer(int index) { return m_Players.at(index); }
    PlayerPtr Find(const std::string& name);
    PlayerList GetFrequency(int freq);

    Direction GetDirection(PlayerPtr player);

    PlayerPtr GetSelectedPlayer() { return m_Selected; }

    PlayerList GetPlayers() { return m_Players; }
};

#endif
