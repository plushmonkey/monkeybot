#ifndef API_PLAYER_H_
#define API_PLAYER_H_

#include <string>

class Vec2;

namespace api {

enum class Ship;

class Player {
public:
    virtual ~Player() { }

    virtual const std::string& GetName() const = 0;
    virtual unsigned int GetFreq() const = 0;
    virtual unsigned short GetRotation() const = 0;
    // Pixels
    virtual Vec2 GetPosition() const = 0;
    // Pixels
    virtual Vec2 GetVelocity() const = 0;
    virtual unsigned short GetPid() const = 0;
    virtual api::Ship GetShip() const = 0;
    virtual bool InArena() const = 0;
    virtual unsigned char GetStatus() const = 0;

    virtual void SetName(const std::string& name) = 0;
    virtual void SetFreq(unsigned int freq) = 0;
    virtual void SetRotation(unsigned short rot) = 0;
    // Pixels
    virtual void SetPosition(int x, int y) = 0;
    // Pixels
    virtual void SetPosition(Vec2 pos) = 0;
    // Pixels
    virtual void SetVelocity(int xspeed, int yspeed) = 0;
    // Pixels
    virtual void SetVelocity(Vec2 vel) = 0;
    virtual void SetPid(unsigned short pid) = 0;
    virtual void SetShip(api::Ship ship) = 0;
    virtual void SetInArena(bool inarena) = 0;
    virtual void SetStatus(unsigned char status) = 0;
    virtual Vec2 GetHeading() const = 0;
};

typedef std::shared_ptr<Player> PlayerPtr;
typedef std::vector<api::PlayerPtr> PlayerList;

} // ns

#endif
