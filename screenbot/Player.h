#ifndef PLAYER_H_
#define PLAYER_H_

#include "Vector.h"

#include <string>
#include <memory>
#include <vector>

class Player {
private:
    std::string m_Name;
    unsigned short m_Freq;
    unsigned short m_Rotation;
    Vec2 m_Position;
    Vec2 m_Velocity;
    unsigned short m_Pid;

public:
    Player(const std::string& name, unsigned short freq, unsigned short rot, int x, int y, int xspeed, int yspeed, unsigned short pid)
        : m_Name(name), m_Freq(freq), m_Rotation(rot), m_Position(x, y), m_Velocity(xspeed, yspeed), m_Pid(pid)
    {

    }

    Player() { }
    Player(const std::string& name) : m_Name(name) { }
    Player(const std::string& name, unsigned short freq) : m_Name(name), m_Freq(freq) { }

    const std::string& GetName() const { return m_Name; }
    unsigned short GetFreq() const { return m_Freq; }
    unsigned short GetRotation() const { return m_Rotation; }
    // Pixels
    Vec2 GetPosition() const { return m_Position; }
    // Pixels
    Vec2 GetVelocity() const { return m_Velocity; }
    unsigned short GetPid() const { return m_Pid; }

    void SetName(const std::string& name) { m_Name = name; }
    void SetFreq(unsigned short freq) { m_Freq = freq; }
    void SetRotation(unsigned short rot) { m_Rotation = rot; }
    // Pixels
    void SetPosition(int x, int y) { m_Position = Vec2(x, y); }
    // Pixels
    void SetPosition(Vec2 pos) { m_Position = pos; }
    // Pixels
    void SetVelocity(int xspeed, int yspeed) { m_Velocity = Vec2(xspeed, yspeed); }
    // Pixels
    void SetVelocity(Vec2 vel) { m_Velocity = vel; }
    void SetPid(unsigned short pid) { m_Pid = pid; }
};

typedef std::shared_ptr<Player> PlayerPtr;
typedef std::vector<PlayerPtr> PlayerList;

#endif
