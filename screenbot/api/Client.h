#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <string>
#include <vector>
#include <Windows.h>
#include <memory>

enum class GunState {
    Tap,
    Constant,
    Off
};

enum class MultiState {
    None,
    On,
    Off
};

enum class Direction;
class Level;
class Vec2;
class Player;

namespace api {    

class Client {
public:
    Client() { }
    virtual ~Client() { }

    virtual void Update(DWORD dt) = 0;

    virtual void Bomb() = 0;
    virtual void Gun(GunState state, int energy_percent = 100) = 0;
    virtual void Burst() = 0;
    virtual void Repel() = 0;
    virtual void Decoy() = 0;
    virtual void SetThrust(bool on) = 0;

    virtual void SetXRadar(bool on) = 0;
    virtual void Warp() = 0;

    virtual int GetEnergy() = 0;
    virtual int GetRotation() = 0;
    virtual bool InSafe(Vec2 real_pos, const Level& level) = 0;

    virtual void Up(bool val) = 0;
    virtual void Down(bool val) = 0;
    virtual void Left(bool val) = 0;
    virtual void Right(bool val) = 0;

    virtual void Attach() = 0;

    virtual bool InShip() const = 0;
    virtual void EnterShip(int num) = 0;
    virtual void Spec() = 0;

    virtual std::vector<std::shared_ptr<Player>> GetEnemies(Vec2 real_pos, const Level& level) = 0;
    virtual std::shared_ptr<Player> GetClosestEnemy(Vec2 real_pos, Vec2 heading, const Level& level, int* dx, int* dy, double* dist) = 0;

    virtual Vec2 GetRealPosition(Vec2 bot_pos, Vec2 target, const Level& level) = 0;

    virtual void ReleaseKeys() = 0;
    virtual void ToggleKeys() = 0;

    virtual int GetFreq() = 0;

    virtual std::vector<std::shared_ptr<Player>> GetFreqPlayers(int freq) = 0;
    virtual std::vector<std::shared_ptr<Player>> GetPlayers() = 0;

    virtual bool OnSoloFreq() = 0;
    virtual std::shared_ptr<Player> GetSelectedPlayer() = 0;
    virtual void MoveTicker(Direction dir) = 0;

    virtual std::vector<Vec2> FindMines(Vec2 bot_pixel_pos) = 0;

    virtual void Scan() = 0;

    virtual bool Emped() = 0;

    virtual void SendString(const std::string& str) = 0;
    virtual void SendPM(const std::string& target, const std::string& mesg) = 0;
    virtual void UseMacro(short num) = 0;

    virtual void SelectPlayer(const std::string& name) = 0;

    virtual void SetTarget(const std::string& name) = 0;
    virtual void SetPriorityTarget(const std::string& name) = 0;

    virtual std::string GetTarget() const = 0;
    virtual std::string GetPriorityTarget() const = 0;
    virtual void EnableMulti(bool enable) = 0;

    virtual MultiState GetMultiState() const = 0;
};

} // ns

typedef std::shared_ptr<api::Client> ClientPtr;

#endif
