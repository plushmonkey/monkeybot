#ifndef PLAYER_LIST_H_
#define PLAYER_LIST_H_

#include "Common.h"

#include <vector>
#include <string>

class Player {
private:
    std::string m_Name;
    int m_Freq;

public:
    Player() : m_Name(""), m_Freq(9999) { }
    Player(std::string name) : m_Name(name) { }
    Player(std::string name, int freq) : m_Name(name), m_Freq(freq) { }

    void SetName(const std::string& name) { m_Name = name; }
    std::string GetName() const { return m_Name; }

    void SetFreq(int freq) { m_Freq = freq; }
    int GetFreq() const { return m_Freq; }
};

typedef std::shared_ptr<Player> PlayerPtr;
typedef std::vector<PlayerPtr> PlayerList;

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

    PlayerPtr GetPlayer(int index) { return m_Players.at(index); }
    PlayerPtr Find(const std::string& name);
    PlayerList GetFrequency(int freq);

    PlayerPtr GetSelectedPlayer() { return m_Selected; }

    PlayerList GetPlayers() { return m_Players; }
};

#endif
