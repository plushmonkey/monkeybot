#ifndef API_BOT_H_
#define API_BOT_H_

#include "Client.h"
#include "State.h"
#include "Command.h"
#include "Version.h"
#include "../Pathing.h"

#include <memory>
#include <functional>

#undef SendMessage

struct TargetInfo;
class PluginManager;
class Config;

namespace api {

class Selector;
class Player;
class EnemySelectorFactory;

class Bot {
public:
    typedef std::function<bool(Bot*, unsigned long)> UpdateFunction;
    typedef unsigned int UpdateID;

    /**
     * \brief Gets the Client class for interacting with the actual game window and keyboard.
     */
    virtual std::shared_ptr<api::Client> GetClient() = 0;

    virtual Config& GetConfig() = 0;

    virtual Version& GetVersion() = 0;

    virtual CommandHandler& GetCommandHandler() = 0;
    virtual PluginManager& GetPluginManager() = 0;


    virtual UpdateID RegisterUpdater(UpdateFunction func) = 0;
    virtual void UnregisterUpdater(UpdateID id) = 0;

    /**
     * \brief Gets the bot's player name.
     */
    virtual std::string GetName() const = 0;

    virtual Ship GetShip() const = 0;


    virtual StatePtr GetState() const = 0;
    /**
     * \brief Sets the current state of the bot.
     */
    virtual void SetState(StatePtr state) = 0;

    /**
     * \brief Sets the current state of the bot using StateType.
     */
    virtual void SetState(StateType type) = 0;

    virtual bool GetPaused() const = 0;
    virtual void SetPaused(bool paused) = 0;

    /**
     * \brief Sets the bot's ship. It might have to pause and wait for energy before switching.
     */
    virtual void SetShip(Ship ship) = 0;

    /**
     * \brief Gets the current velocity of the bot in tiles.
     */
    virtual Vec2 GetVelocity() const = 0;

    /**
     * \brief Gets the direction that the bot is facing.
     */
    virtual Vec2 GetHeading() const = 0;

    /**
     * \brief Gets the position of the bot in tiles.
     */
    virtual Vec2 GetPos() const = 0;

    /**
     * \brief Gets the bot's speed in tiles/sec.
     */
    virtual double GetSpeed() const = 0;

    /**
     * \brief Sets the bot's speed in tiles/sec.
     */
    virtual void SetSpeed(double target) = 0;

    /**
     * \brief Gets the bot's current energy.
     */
    virtual int GetEnergy() const = 0;

    /**
    * \brief Gets the bot's max energy.
    */
    virtual int GetMaxEnergy() const = 0;

    /**
    * \brief Gets the bot's current energy percent.
    */
    virtual int GetEnergyPercent() const = 0;

    /**
    * \brief Determines whether or not the bot is at full energy.
    */
    virtual bool FullEnergy() const = 0;

    virtual const std::string& GetAttachTarget() const = 0;

    virtual bool IsInCenter() const = 0;
    virtual std::shared_ptr<Player> GetEnemyTarget() const = 0;
    virtual TargetInfo GetEnemyTargetInfo() const = 0;
    virtual Pathing::Grid<short>& GetGrid() = 0;
    virtual const Level& GetLevel() const = 0;

    virtual MemorySensor& GetMemorySensor() = 0;

    virtual void Follow(const std::string& name) = 0;

    virtual unsigned long GetLastEnemy() const = 0;
    virtual void SetLastEnemy(unsigned long last) = 0;

    /**
     * \brief Sends a chat message. Blocking function that waits until send buffer is cleared in update loop.
     */
    virtual void SendMessage(const std::string& str) = 0;

    virtual bool IsInSafe() const = 0;
    virtual void SetEnemySelector(std::shared_ptr<Selector> selector) = 0;
    virtual unsigned int GetFreq() const = 0;
    virtual void UpdateLog() = 0;
    virtual std::shared_ptr<api::EnemySelectorFactory> GetEnemySelectors() = 0;
};

} // ns

#endif
