#ifndef PLAYER_LIST_H_
#define PLAYER_LIST_H_

#include "Common.h"
#include "Player.h"

#include <vector>
#include <string>

class PlayerWindow {
private:
    ScreenAreaPtr m_Area;
    api::PlayerList m_Players;
    unsigned long m_UpdateTimer;
    api::PlayerPtr m_Selected;

    std::string GetLineText(int line);
public:
    PlayerWindow();
    PlayerWindow(ScreenAreaPtr area);

    void SetScreenArea(ScreenAreaPtr area);

    void Update(unsigned long dt);

    int GetPlayerIndex(api::PlayerPtr player);
    api::PlayerPtr GetPlayer(int index) { return m_Players.at(index); }
    api::PlayerPtr Find(const std::string& name);
    api::PlayerList GetFrequency(int freq);

    Direction GetDirection(api::PlayerPtr player);

    api::PlayerPtr GetSelectedPlayer() { return m_Selected; }

    api::PlayerList GetPlayers() { return m_Players; }
};

#endif
