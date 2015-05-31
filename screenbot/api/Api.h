#ifndef API_H_
#define API_H_

#include "../ClientSettings.h"
#include <Windows.h>
#include <string>
#include <memory>
#include <vector>

namespace Memory { enum class SensorError; }
class Vec2;
namespace api {

enum class Ship { Warbird, Javelin, Spider, Leviathan, Terrier, Weasel, Lancaster, Shark, Spectator };

class Player;
class Bot;

class MemorySensor {
public:
    virtual ~MemorySensor() { }

    virtual Memory::SensorError Initialize(HWND window) = 0;

    virtual Vec2 GetPosition() const = 0;
    virtual Vec2 GetVelocity() const = 0;
    virtual unsigned int GetFrequency() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual std::vector<std::shared_ptr<api::Player>> GetPlayers() = 0;;
    virtual std::shared_ptr<api::Player> GetBotPlayer() const = 0;
    virtual const ShipSettings& GetShipSettings(api::Ship ship) const = 0;
    virtual bool OnUpdate(api::Bot* bot, unsigned long dt) = 0;
};

} // ns

#include "Version.h"
#include "Bot.h"
#include "Client.h"
#include "State.h"
#include "Command.h"
#include "Player.h"


// Get rid of the ridiculous windows defines
#ifdef GetMessage
#undef GetMessage
#undef SendMessage
#endif

namespace api {

class Selector {
public:
    virtual std::shared_ptr<Player> Select(Bot* bot) = 0;
};

typedef std::shared_ptr<Selector> SelectorPtr;

class EnemySelectorFactory {
public:
    virtual api::SelectorPtr CreateClosest() = 0;
    virtual api::SelectorPtr CreateTarget(const std::string& target) = 0;
};

/*class Config {
public:
    virtual bool Load(const std::string& jsonfile) = 0;
    virtual void LoadShip(api::Ship ship) = 0;

    virtual Json::Value GetRoot() const = 0;
    virtual Json::Value GetZoneConfig() const = 0;
};
*/
} // ns

#endif
